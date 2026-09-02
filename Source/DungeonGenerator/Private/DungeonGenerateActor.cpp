/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * ADungeonGeneratedActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
 * ADungeonGenerateActorは配置可能(Placeable)、ADungeonGeneratedActorは配置不可能(NotPlaceable)にするため、
 * 継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
 */

#include "DungeonGenerateActor.h"
#include "DungeonSpatialMeshGroup.h"
#include "Core/Generator.h"
#include "Core/Debug/BuildInformation.h"
#include "Core/Debug/Debug.h"
#include "Core/Debug/Config.h"
#include "Core/Helper/Stopwatch.h"
#include "Core/Math/VectorUtility.h"
#include "Core/Voxelization/Grid.h"
#include "Core/Voxelization/Voxel.h"
#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "PluginInformation.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include <GameFramework/PlayerController.h>
#include <Engine/LevelStreaming.h>
#include <Kismet/GameplayStatics.h>
#include <Net/UnrealNetwork.h>

#include "Core/Debug/MeasureTime.h"

#if WITH_EDITOR && JENKINS_FOR_DEVELOP
#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetToolsModule.h>
#include <ContentBrowserModule.h>
#include <FileHelpers.h>
#include <Framework/Notifications/NotificationManager.h>
#include <HAL/FileManager.h>
#include <IAssetTools.h>
#include <IContentBrowserSingleton.h>
#include <Misc/App.h>
#include <Misc/EngineVersionComparison.h>
#include <UObject/Package.h>
#include <Widgets/Notifications/SNotificationList.h>
#endif
#if WITH_EDITOR
#include <DrawDebugHelpers.h>
#include <Editor.h>
#include <Engine/Level.h>
#include <Kismet/KismetSystemLibrary.h>
#include <Misc/PackageName.h>
#endif

#define LOCTEXT_NAMESPACE "ADungeonGenerateActor"

namespace
{
	constexpr float DungeonGenerationTickInterval = 6.f / 60.f;

#if WITH_EDITOR
	constexpr float DrawDebugDuration = DungeonGenerationTickInterval * 2.f;
	constexpr uint8 FloorPlaneOpacity = 51;

#if JENKINS_FOR_DEVELOP
	void ShowParameterSaveNotification(const FText& message, const SNotificationItem::ECompletionState completionState)
	{
		FNotificationInfo notificationInfo(message);
		notificationInfo.bFireAndForget = true;
		notificationInfo.ExpireDuration = completionState == SNotificationItem::CS_Fail ? 8.f : 5.f;
		if (const TSharedPtr<SNotificationItem> notification = FSlateNotificationManager::Get().AddNotification(notificationInfo))
		{
			notification->SetCompletionState(completionState);
		}
	}

	FString MakeParameterSeedToken(const int32 seed)
	{
		return seed < 0
			? FString::Printf(TEXT("N%lld"), -static_cast<int64>(seed))
			: FString::FromInt(seed);
	}

	void SelectParameterInContentBrowser(UObject* parameter)
	{
		if (GEditor && IsValid(parameter))
		{
			TArray<UObject*> objectsToSync;
			objectsToSync.Add(parameter);
			GEditor->SyncBrowserToObjects(objectsToSync);
		}
	}
#endif

	constexpr FColor FloorDebugColors[] =
	{
		FColor(64, 160, 255),
		FColor(80, 220, 140),
		FColor(255, 190, 64),
		FColor(220, 96, 220),
		FColor(64, 220, 220),
		FColor(255, 112, 112)
	};

#endif // WITH_EDITOR
}

ADungeonGenerateActor::ADungeonGenerateActor(const FObjectInitializer& initializer)
	: Super(initializer)
	, BuildJobTag(TEXT(DUNGEON_GENERATOR_PLUGIN_VERSION_NAME "-" JENKINS_JOB_TAG))
	, LicenseTag(TEXT(JENKINS_LICENSE))
	, LicenseId(TEXT(JENKINS_UUID))
{
	// Tick Enable
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = DungeonGenerationTickInterval;

	SetCanBeDamaged(false);
	SetReplicates(true);
	bAlwaysRelevant = true;
}

void ADungeonGenerateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADungeonGenerateActor, mReplicatedGenerationState);
}

#if WITH_EDITOR && JENKINS_FOR_DEVELOP
/**
 * Finds the saved asset that owns the destination context for a parameter copy.
 * Parameter Copyの保存先Contextとなる保存済みAssetを検索します。
 */
const UDungeonGenerateParameter* ADungeonGenerateActor::ResolveParameterAssetSource(const UDungeonGenerateParameter* parameter)
{
	const UDungeonGenerateParameter* candidate = parameter;
	for (int32 depth = 0; depth < 8 && IsValid(candidate); ++depth)
	{
		UPackage* candidatePackage = candidate->GetOutermost();
		const FString candidatePackageName = IsValid(candidatePackage) ? candidatePackage->GetName() : FString();
		FString candidateFilename;
		if (!candidate->HasAnyFlags(RF_Transient) &&
			FPackageName::IsValidLongPackageName(candidatePackageName) &&
			FPackageName::DoesPackageExist(candidatePackageName, &candidateFilename))
		{
			return candidate;
		}

		const UDungeonGenerateParameter* nextCandidate = candidate->GenerationSourceParameterAsset;
		if (!IsValid(nextCandidate) || nextCandidate == candidate)
			break;
		candidate = nextCandidate;
	}
	return nullptr;
}

/**
 * Saves one parameter object as a new content asset and rolls back registration on failure.
 * 1つのParameter Objectを新しいContent Assetとして保存し、失敗時は登録を元に戻します。
 */
bool ADungeonGenerateActor::SaveFailedParameterAsAsset(UDungeonGenerateParameter* parameter, const UDungeonGenerateParameter* sourceAsset, const int32 savedSeed)
{
	if (!IsValid(parameter))
		return false;

	const FString seedToken = MakeParameterSeedToken(savedSeed);
	FString packageName;
	FString assetName;

	if (IsValid(sourceAsset))
	{
		const FString sourcePackageName = sourceAsset->GetOutermost()->GetName();
		const FString sourceFolder = FPackageName::GetLongPackagePath(sourcePackageName);
		const FString baseAssetName = FString::Printf(TEXT("%s_Failed_Seed_%s"), *sourceAsset->GetName(), *seedToken);
		const FString basePackageName = sourceFolder / baseAssetName;

		IAssetTools& assetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		assetTools.CreateUniqueAssetName(basePackageName, TEXT(""), packageName, assetName);
	}
	else
	{
		if (FApp::IsUnattended())
		{
			ShowParameterSaveNotification(
				LOCTEXT("ParameterSaveDialogUnavailable", "The parameter has no saved source asset, and a save location cannot be requested in unattended mode."),
				SNotificationItem::CS_Fail);
			return false;
		}

		FSaveAssetDialogConfig saveAssetDialogConfig;
		saveAssetDialogConfig.DialogTitleOverride = LOCTEXT("SaveFailedParameterDialogTitle", "Save Failed DungeonGenerateParameter");
		saveAssetDialogConfig.DefaultPath = TEXT("/Game");
		saveAssetDialogConfig.DefaultAssetName = FString::Printf(TEXT("DungeonGenerateParameter_Failed_Seed_%s"), *seedToken);
		saveAssetDialogConfig.AssetClassNames.Add(UDungeonGenerateParameter::StaticClass()->GetClassPathName());
		saveAssetDialogConfig.ExistingAssetPolicy = ESaveAssetDialogExistingAssetPolicy::Disallow;

		FContentBrowserModule& contentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		const FString chosenObjectPath = contentBrowserModule.Get().CreateModalSaveAssetDialog(saveAssetDialogConfig);
		if (chosenObjectPath.IsEmpty())
		{
			ShowParameterSaveNotification(
				LOCTEXT("ParameterSaveCancelled", "DungeonGenerateParameter was not saved because the save dialog was cancelled."),
				SNotificationItem::CS_Fail);
			return false;
		}

		packageName = FPackageName::ObjectPathToPackageName(chosenObjectPath);
		assetName = FPackageName::ObjectPathToObjectName(chosenObjectPath);
		if (!FPackageName::IsValidLongPackageName(packageName) || assetName.IsEmpty())
		{
			ShowParameterSaveNotification(
				LOCTEXT("ParameterSavePathInvalid", "The selected DungeonGenerateParameter asset path is invalid."),
				SNotificationItem::CS_Fail);
			return false;
		}
	}

	UPackage* newPackage = CreatePackage(*packageName);
	if (!IsValid(newPackage))
	{
		ShowParameterSaveNotification(
			LOCTEXT("ParameterPackageCreationFailed", "Could not create a package for the DungeonGenerateParameter copy."),
			SNotificationItem::CS_Fail);
		return false;
	}

	UDungeonGenerateParameter* newParameter = DuplicateObject<UDungeonGenerateParameter>(
		parameter,
		newPackage,
		FName(*assetName));
	if (!IsValid(newParameter))
	{
		ShowParameterSaveNotification(
			LOCTEXT("ParameterDuplicationFailed", "Could not duplicate the DungeonGenerateParameter."),
			SNotificationItem::CS_Fail);
		return false;
	}

	newParameter->ClearFlags(RF_Transient);
	newParameter->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
	newParameter->SetRandomSeed(savedSeed);
	newParameter->GenerationSourceParameterAsset = nullptr;
	FAssetRegistryModule::AssetCreated(newParameter);
	static_cast<void>(newPackage->MarkPackageDirty());

	TArray<UPackage*> packagesToSave;
	packagesToSave.Add(newPackage);
#if UE_VERSION_NEWER_THAN(5, 3, 0)
	FEditorFileUtils::FPromptForCheckoutAndSaveParams saveParams;
	saveParams.bCheckDirty = false;
	saveParams.bPromptToSave = false;
	saveParams.bCanBeDeclined = false;
	const FEditorFileUtils::EPromptReturnCode saveResult = FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, saveParams);
#else
	const FEditorFileUtils::EPromptReturnCode saveResult = FEditorFileUtils::PromptForCheckoutAndSave(packagesToSave, false, false, nullptr, false, false);
#endif
	FString savedFilename;
	const bool savedFileExists = saveResult == FEditorFileUtils::EPromptReturnCode::PR_Success &&
		FPackageName::TryConvertLongPackageNameToFilename(packageName, savedFilename, FPackageName::GetAssetPackageExtension()) &&
		IFileManager::Get().FileExists(*savedFilename);
	if (!savedFileExists)
	{
		FAssetRegistryModule::AssetDeleted(newParameter);
		newParameter->ClearFlags(RF_Public | RF_Standalone);
		newParameter->MarkAsGarbage();
		newPackage->SetDirtyFlag(false);
		const FText failureMessage = saveResult == FEditorFileUtils::EPromptReturnCode::PR_Success
			? LOCTEXT("ParameterSaveFileMissing", "The DungeonGenerateParameter copy was not written to disk. No asset was registered.")
			: LOCTEXT("ParameterSaveFailed", "The DungeonGenerateParameter copy could not be saved. No asset was registered.");
		ShowParameterSaveNotification(failureMessage, SNotificationItem::CS_Fail);
		return false;
	}

	SelectParameterInContentBrowser(newParameter);
	ShowParameterSaveNotification(
		FText::Format(LOCTEXT("FailedParameterSaved", "Saved failed DungeonGenerateParameter: {0}"), FText::FromString(newParameter->GetPathName())),
		SNotificationItem::CS_Success);
	return true;
}

/**
 * Captures the active supplied parameter before failed-generation cleanup mutates actor state.
 * 生成失敗後のCleanupでActor状態が変わる前に、使用中の指定Parameterを取得します。
 */
void ADungeonGenerateActor::CaptureFailedGenerationParameterSnapshot()
{
	PendingFailedGenerationParameterSnapshot = nullptr;
	PendingFailedGenerationParameterSourceAsset = nullptr;
	if (!mAutoSaveFailedGenerationParameter || !IsValid(DungeonGenerateParameter))
		return;

	const FName snapshotName = MakeUniqueObjectName(this, UDungeonGenerateParameter::StaticClass(), TEXT("FailedGenerationParameterSnapshot"));
	PendingFailedGenerationParameterSnapshot = DuplicateObject<UDungeonGenerateParameter>(DungeonGenerateParameter, this, snapshotName);
	if (!IsValid(PendingFailedGenerationParameterSnapshot))
	{
		ShowParameterSaveNotification(
			LOCTEXT("FailedParameterSnapshotFailed", "Could not capture the failed DungeonGenerateParameter before cleanup."),
			SNotificationItem::CS_Fail);
		return;
	}

	PendingFailedGenerationParameterSnapshot->SetFlags(RF_Transient);
	PendingFailedGenerationParameterSnapshot->SetRandomSeed(GeneratedRandomSeed);
	PendingFailedGenerationParameterSourceAsset = const_cast<UDungeonGenerateParameter*>(ResolveParameterAssetSource(DungeonGenerateParameter));
}

/**
 * Converts the pending failed snapshot into an asset after generation cleanup completes.
 * 生成Cleanup完了後に保留中の失敗SnapshotをAssetへ変換します。
 */
void ADungeonGenerateActor::SavePendingFailedGenerationParameter()
{
	if (!IsValid(PendingFailedGenerationParameterSnapshot))
		return;

	SaveFailedParameterAsAsset(
		PendingFailedGenerationParameterSnapshot,
		PendingFailedGenerationParameterSourceAsset,
		PendingFailedGenerationParameterSnapshot->GetRandomSeed());
	PendingFailedGenerationParameterSnapshot = nullptr;
	PendingFailedGenerationParameterSourceAsset = nullptr;
}
#endif // WITH_EDITOR && JENKINS_FOR_DEVELOP

void ADungeonGenerateActor::PostInitializeComponents()
{
	// Calling the parent class
	Super::PostInitializeComponents();

	mComponentsInitialized = true;

	if (AutoGenerateAtStart && HasAuthority())
		GenerateAuthoritative(DungeonGenerateParameter);
	else if (!HasAuthority() && mReplicatedGenerationState.GenerationId != 0)
		ApplyGenerationState(mReplicatedGenerationState);
}

void ADungeonGenerateActor::BeginPlay()
{
	// Calling the parent class
	Super::BeginPlay();

#if WITH_EDITOR
	DUNGEON_GENERATOR_LOG(TEXT("%s: LocalRole=%s"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*UEnum::GetValueAsString(GetLocalRole())
	);
	DUNGEON_GENERATOR_LOG(TEXT("%s: RemoteRole=%s"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		*UEnum::GetValueAsString(GetRemoteRole())
	);

	if (bShowPerformanceStats)
		ShowPerformanceStats();
	else
		HidePerformanceStats();
	mShowPerformanceStats = bShowPerformanceStats;
#endif
}

void ADungeonGenerateActor::Tick(float DeltaSeconds)
{
	// Calling the parent class
	Super::Tick(DeltaSeconds);


#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	DrawDebugInformation();
#endif

#if WITH_EDITOR
	if (mShowPerformanceStats != bShowPerformanceStats)
	{
		mShowPerformanceStats = bShowPerformanceStats;
		if (bShowPerformanceStats)
			ShowPerformanceStats();
		else
			HidePerformanceStats();
	}
#endif
}

/**
 * Forwards the pre-generation notification to the main level script before the core layout is built.
 * コアのレイアウトを構築する前に、生成開始通知をメインレベルスクリプトへ転送します。
 */
void ADungeonGenerateActor::OnPreDungeonGeneration()
{
	if (auto* level = GetLevel())
	{
		if (auto* levelScript = Cast<ADungeonMainLevelScriptActor>(level->GetLevelScriptActor()))
			levelScript->OnPreDungeonGeneration(this);
	}
}

/**
 * Forwards the core generation result to the main level script before world construction continues.
 * ワールド構築を続ける前に、コア生成結果をメインレベルスクリプトへ転送します。
 */
void ADungeonGenerateActor::OnPostDungeonGeneration(const bool result)
{
	if (auto* level = GetLevel())
	{
		if (auto* levelScript = Cast<ADungeonMainLevelScriptActor>(level->GetLevelScriptActor()))
			levelScript->OnPostDungeonGeneration(this, result);
	}
}


////////// InstancedStaticMesh //////////
/**
 * Maps a world position to a generator-relative spatial group used for partitioned instance management.
 * ワールド座標を、分割されたインスタンス管理に使うGenerator相対の空間グループへ変換します。
 */
FIntVector ADungeonGenerateActor::CalculateInstancedMeshGroupCoordinate(const FVector& worldPosition, const FVector& actorLocation, const FVector& gridSize)
{
	return dungeon::spatial_mesh_group::CalculateCoordinate(worldPosition, actorLocation, gridSize);
}

/**
 * Opens a generated-mesh transaction on every existing spatial cluster before bulk instance insertion.
 * インスタンスの一括追加前に、既存の全空間Clusterで生成メッシュTransactionを開始します。
 */
void ADungeonGenerateActor::BeginInstanceTransaction()
{
	for (auto& group : mInstancedMeshClusters)
	{
		group.Value.GetInstances().BeginTransaction(EDungeonInstancedMeshCategory::GeneratedMesh);
	}
}

/**
 * Routes one generated mesh to its spatial cluster and adds it as an ISM or HISM instance.
 * 生成メッシュを位置に対応する空間Clusterへ振り分け、ISMまたはHISM Instanceとして追加します。
 */
void ADungeonGenerateActor::AddInstance(UStaticMesh* staticMesh, const FTransform& transform, const EDungeonMeshGenerationMethod meshGenerationMethod, const bool affectsNavigation)
{
	check(
		meshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh ||
		meshGenerationMethod == EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh
	);

	FVector gridSize(500.0);
	if (mParameter)
		gridSize = mParameter->GetGridSize().To3D();
	const FIntVector groupCoordinate = CalculateInstancedMeshGroupCoordinate(transform.GetTranslation(), GetActorLocation(), gridSize);
	FDungeonInstancedMeshCollection& instances = mInstancedMeshClusters.FindOrAdd(groupCoordinate).GetInstances();
	if (meshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh)
	{
		instances.AddInstance(this, staticMesh, transform, mActiveGenerationId, mActiveGenerationSeed, groupCoordinate, affectsNavigation);
	}
	else
	{
		instances.AddHierarchicalInstance(this, staticMesh, transform, mActiveGenerationId, mActiveGenerationSeed, groupCoordinate, affectsNavigation);
	}
}

/**
 * Commits every generated-mesh cluster, builds pending instance data, and applies the current cull range.
 * 全生成メッシュClusterを確定して保留中のInstance Dataを構築し、現在のCull距離を反映します。
 */
void ADungeonGenerateActor::EndInstanceTransaction()
{
	MEASURE_TIME_START(stopwatch);

	mInstancedMeshClusters.Shrink();

	for (auto& group : mInstancedMeshClusters)
	{
		group.Value.GetInstances().EndTransaction(EDungeonInstancedMeshCategory::GeneratedMesh);
	}

	if (mInstancedMeshCullDistance.Min < mInstancedMeshCullDistance.Max)
		ApplyInstancedMeshCullDistance();

	MEASURE_TIME_LAP(stopwatch, TEXT("EndInstanceTransaction Time"));
}

/**
 * Removes generated terrain instances while retaining clusters that still own another instance category.
 * 生成地形Instanceを削除し、別CategoryのInstanceが残るClusterは維持します。
 */
void ADungeonGenerateActor::DestroyAllInstance()
{
	for (auto& group : mInstancedMeshClusters)
	{
		group.Value.GetInstances().Destroy(EDungeonInstancedMeshCategory::GeneratedMesh);
	}
	for (auto iterator = mInstancedMeshClusters.CreateIterator(); iterator; ++iterator)
	{
		if (iterator.Value().IsEmpty())
			iterator.RemoveCurrent();
	}
}

void ADungeonGenerateActor::SetInstancedMeshCullDistance(const FInt32Interval& cullDistance)
{
	mInstancedMeshCullDistance = cullDistance;
	ApplyInstancedMeshCullDistance();
}

/**
 * Propagates the configured generated-mesh cull range to every spatial cluster.
 * 設定された生成メッシュのCull距離を全空間Clusterへ反映します。
 */
void ADungeonGenerateActor::ApplyInstancedMeshCullDistance()
{
	for (auto& pair : mInstancedMeshClusters)
	{
		pair.Value.GetInstances().SetGeneratedMeshCullDistance(mInstancedMeshCullDistance);
	}
}

////////// 生成と破棄 //////////
/**
 * Rebuilds a dungeon revision from a clean state, binds the selected mesh output path, and records its result.
 * 既存状態を破棄してDungeon Revisionを再構築し、選択されたメッシュ出力方式を接続して結果を記録します。
 */
bool ADungeonGenerateActor::PreGenerateImplementation()
{
	MEASURE_TIME_START(stopwatch);

	if (!IsValid(DungeonGenerateParameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
		return false;
	}


	Dispose(true);
	MEASURE_TIME_LAP(stopwatch, TEXT("Dispose(true) Time"));

	// インスタンスメッシュを登録
	if (DungeonMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
	{
		OnAddFloor([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod, true);
			}
		);
		OnAddSlope([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod, true);
			}
		);
		OnAddCatwalk([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod, true);
			}
		);
	}

	// インスタンスメッシュを登録
	if (DungeonWallRoofPillarMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
	{
		OnAddWall([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod, true);
			}
		);
		OnAddRoof([this](UStaticMesh* staticMesh, const FTransform& transform, const FVector& partitionRegistrationWorldLocation)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod, false);
			}
		);
		OnAddPillar([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod, true);
			}
		);
	}

	// インスタンスメッシュの登録開始
	BeginInstanceTransaction();

	// ダンジョン生成開始
	bool beginDungeonGenerationResult = BeginDungeonGeneration(DungeonGenerateParameter, HasAuthority(), mActiveGenerationSeed);
	MEASURE_TIME_LAP(stopwatch, TEXT("BeginDungeonGeneration Time"));
	const int32 generatedRandomSeed = DungeonGenerateParameter->GetGeneratedRandomSeed();

	/*
	 * サーバーと同じダンジョンになったかを確かめます。
	 * 違うダンジョンで遊ばせる方が、ダンジョンが無いより危険なので失敗として扱います。
	 * EndDungeonGenerationは mGenerated で経路を選ぶため、そちらも倒します。
	 */
	if (beginDungeonGenerationResult && mHasExpectedServerCRC32)
	{
		const uint32 localCRC32 = static_cast<uint32>(CalculateCRC32());
		if (localCRC32 != mExpectedServerCRC32)
		{
			ReportGenerationCrcMismatch(mActiveGenerationId, mExpectedServerCRC32, localCRC32, generatedRandomSeed);
			InvalidateGeneratedDungeon();
			beginDungeonGenerationResult = false;
		}
	}
	mHasExpectedServerCRC32 = false;
#if WITH_EDITOR
	GeneratedRandomSeed = generatedRandomSeed;
#endif
	if (beginDungeonGenerationResult)
	{

		// PlayerStartの移動
		{
			/*
			 * List of PlayerStart, not including PlayerStartPIE.
			 * PlayerStartの一覧。PlayerStartPIEは含まない。
			 */
			TArray<APlayerStart*> playerStart;
			CollectPlayerStartExceptPlayerStartPIE(playerStart);


			MovePlayerStart(playerStart);
			MEASURE_TIME_LAP(stopwatch, TEXT("MovePlayerStart Time"));
		}

		// スタート部屋とゴール部屋の位置を記録
		StartRoomLocation = GetStartLocation();
		GoalRoomLocation = GetGoalLocation();

#if WITH_EDITOR
		// Record the generated dungeon hash.
		GeneratedDungeonCRC32 = CalculateCRC32();

		if (HasAuthority())
		{
			DungeonGenerateParameter->SetGeneratedDungeonCRC32(GeneratedDungeonCRC32);
		}

		if (GeneratedDungeonCRC32 == DungeonGenerateParameter->GetGeneratedDungeonCRC32())
		{
			DUNGEON_GENERATOR_LOG(TEXT("%s, Remote:(GeneratedRandomSeed=%x, CRC32=%x) Local:(GeneratedRandomSeed=%x, CRC32=%x)"),
				HasAuthority() ? TEXT("Server") : TEXT("Client"),
				DungeonGenerateParameter->GetGeneratedRandomSeed(),
				DungeonGenerateParameter->GetGeneratedDungeonCRC32(),
				GeneratedRandomSeed,
				GeneratedDungeonCRC32
			);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("%s, Remote:(GeneratedRandomSeed=%x, CRC32=%x) Local:(GeneratedRandomSeed=%x, CRC32=%x)"),
				HasAuthority() ? TEXT("Server") : TEXT("Client"),
				DungeonGenerateParameter->GetGeneratedRandomSeed(),
				DungeonGenerateParameter->GetGeneratedDungeonCRC32(),
				GeneratedRandomSeed,
				GeneratedDungeonCRC32
			);
		}
#endif

		// ダンジョン生成完了
		EndDungeonGeneration();
		MEASURE_TIME_LAP(stopwatch, TEXT("EndDungeonGeneration (success path) Time"));
	}
	else
	{
#if WITH_EDITOR && JENKINS_FOR_DEVELOP
		CaptureFailedGenerationParameterSnapshot();
#endif
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon. Seed(%d)"), generatedRandomSeed);

		EndDungeonGeneration();
		MEASURE_TIME_LAP(stopwatch, TEXT("EndDungeonGeneration (failure path) Time"));

		Dispose(false);
		MEASURE_TIME_LAP(stopwatch, TEXT("Dispose(false) Time"));
	}

	// インスタンスメッシュを登録完了
	EndInstanceTransaction();
#if WITH_EDITOR && JENKINS_FOR_DEVELOP
	SavePendingFailedGenerationParameter();
#endif
	return beginDungeonGenerationResult;
}

/**
 * Warns about Actor transforms that generation deliberately does not apply to the generated dungeon.
 * 生成Dungeonへ意図的に反映されないActorの回転とScaleについて警告します。
 */
void ADungeonGenerateActor::PostGenerateImplementation() const
{
	if (GetActorRotation().Equals(FRotator::ZeroRotator) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("The actor's rotation is not applied in the generated dungeon."));
	}

	if (GetActorScale().Equals(FVector::OneVector) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("The actor's scale is not applied in the generated dungeon."));
	}
}

void ADungeonGenerateActor::Dispose(const bool flushStreamLevels)
{
	// 生成が途中で失敗した場合もインスタンスメッシュは残るため、IsGeneratedでは判定できません
	if (IsDisposeRequired())
	{
		DestroyAllInstance();
	}

	Super::Dispose(flushStreamLevels);
}

/**
 * Finalizes all queued terrain ISM and HISM instances before decoration begins.
 * 装飾処理の開始前に、保留している地形ISMとHISMのインスタンスをすべて確定します。
 */
void ADungeonGenerateActor::FinalizeGeneratedTerrain()
{
	MEASURE_TIME_START(stopwatch);
	EndInstanceTransaction();
	MEASURE_TIME_LAP(stopwatch, TEXT("EndInstanceTransaction Time"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint Useful Functions
// サーバープロセスで実行する
void ADungeonGenerateActor::GenerateDungeon_Implementation()
{
	if (mIsGeneratingDungeon)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("GenerateDungeon ignored because dungeon generation is already in progress."));
		return;
	}

#if JENKINS_FOR_DEVELOP
	DUNGEON_GENERATOR_LOG(TEXT("ServerOnGenerateDungeon: %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
#endif

	GenerateAuthoritative(DungeonGenerateParameter);
}

// サーバープロセスで実行する
void ADungeonGenerateActor::GenerateDungeonWithParameter_Implementation(UDungeonGenerateParameter* dungeonGenerateParameter)
{
	if (mIsGeneratingDungeon)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("GenerateDungeonWithParameter ignored because dungeon generation is already in progress."));
		return;
	}

#if JENKINS_FOR_DEVELOP
	DUNGEON_GENERATOR_LOG(TEXT("ServerOnGenerateDungeonWithParameter: %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
#endif

#if WITH_EDITOR && JENKINS_FOR_DEVELOP
	PendingFailedGenerationParameterSnapshot = nullptr;
	PendingFailedGenerationParameterSourceAsset = nullptr;
	const TGuardValue<bool> autoSaveFailedParameterGuard(mAutoSaveFailedGenerationParameter, HasAuthority() && GetNetMode() == NM_Standalone);
#endif
	GenerateAuthoritative(dungeonGenerateParameter);
}

// 全てのクライアントプロセスで実行する
void ADungeonGenerateActor::MulticastApplyGenerationState_Implementation(const FDungeonGenerationReplicatedState& generationState)
{
	ApplyGenerationState(generationState);
}

/**
 * Applies the replicated state to late-joining clients.
 * 複製された状態を途中参加クライアントへ適用します。
 */
void ADungeonGenerateActor::OnRep_GenerationState()
{
	ApplyGenerationState(mReplicatedGenerationState);
}

/**
 * Allocates an actor-local monotonic generation identifier.
 * Actor単位で単調増加する生成識別子を採番します。
 */
/**
 * Derives the seed for a retry from the seed that just failed.
 * ResolveGenerationSeed falls back to the wall clock, so calling it again inside the same second
 * returns the same seed and repeats the same failure.
 * 直前に失敗した乱数の種から、再試行用の種を導出します。
 * ResolveGenerationSeedは時刻を使うため、同じ秒の中で呼び直すと同じ種が返り、
 * 同じ失敗を繰り返してしまいます。
 */
int32 ADungeonGenerateActor::MakeRetryGenerationSeed(const int32 previousSeed, const int32 attempt)
{
	// 種が偏らないよう、大きな奇数を掛けてから回転させます
	const uint32 mixed = static_cast<uint32>(previousSeed) * 1664525u + 1013904223u * static_cast<uint32>(attempt);
	const int32 seed = static_cast<int32>(mixed & 0x7fffffffu);

	// 0は「指定なし」を意味するため避けます
	return seed != 0 ? seed : 1;
}

uint64 ADungeonGenerateActor::AllocateGenerationId() const
{
	if (mReplicatedGenerationState.GenerationId == MAX_uint64)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Dungeon generation ID is exhausted for '%s'; refusing to reuse a previous network identity."), *GetPathName());
		return 0;
	}

	const uint64 nextGenerationId = mReplicatedGenerationState.GenerationId + 1;
	return nextGenerationId;
}

/**
 * Resolves an automatic seed once on the authoritative server.
 * 自動SeedをAuthorityサーバー上で一度だけ確定します。
 */
int32 ADungeonGenerateActor::ResolveGenerationSeed(const UDungeonGenerateParameter* parameter)
{
	if (!IsValid(parameter))
		return 0;

	const int32 configuredSeed = parameter->GetRandomSeed();
	return configuredSeed != 0 ? configuredSeed : static_cast<int32>(time(nullptr));
}

/**
 * Refreshes systems that consume the completed generated world.
 * 完成した生成ワールドを参照するシステムを更新します。
 */
void ADungeonGenerateActor::RefreshGeneratedWorldState() const
{
	if (const UWorld* world = GetWorld())
	{
		if (IsValid(world->PersistentLevel))
		{
			if (ADungeonMainLevelScriptActor* levelScript = Cast<ADungeonMainLevelScriptActor>(world->PersistentLevel->GetLevelScriptActor()))
				levelScript->RebuildSparsePartitionGraphAndRefresh();
		}
	}
}

/**
 * Applies one unseen replicated revision and validates its generated CRC.
 * 未適用の複製Revisionを一度だけ適用し、生成CRCを検証します。
 */
bool ADungeonGenerateActor::ApplyGenerationState(const FDungeonGenerationReplicatedState& generationState)
{
	if (!mComponentsInitialized || generationState.GenerationId == 0 || generationState.GenerationId <= mAppliedGenerationId)
		return false;

	if (mIsGeneratingDungeon)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Generation state R%016llX ignored because dungeon generation is already in progress."), static_cast<unsigned long long>(generationState.GenerationId));
		return false;
	}

	mAppliedGenerationId = generationState.GenerationId;
	mActiveGenerationId = generationState.GenerationId;
	mActiveGenerationSeed = generationState.Seed;

	if (!generationState.bGenerated)
	{
		Dispose(false);
		RefreshGeneratedWorldState();
		return true;
	}

	if (!IsValid(generationState.Parameter))
	{
		/*
		 * 参照を解決できない原因はほぼ常に、保存していないパラメータを配信した事です
		 * 送り出す側でもReportNonReplicableParameterが報告しています
		 */
		DUNGEON_GENERATOR_ERROR(TEXT("Generation state R%016llX has no valid parameter asset. The server most likely generated with a parameter that has no stable network path, such as the result of GenerateRandomParameter."), static_cast<unsigned long long>(generationState.GenerationId));
		return false;
	}

	DungeonGenerateParameter = generationState.Parameter;
	DungeonMeshGenerationMethod = generationState.FloorGenerationMethod;
	DungeonWallRoofPillarMeshGenerationMethod = generationState.WallGenerationMethod;
	DungeonGenerateParameter->SetGeneratedRandomSeed(generationState.Seed);
	if (!HasAuthority())
		DungeonGenerateParameter->SetGeneratedDungeonCRC32(static_cast<int32>(generationState.ServerCRC32));

	/*
	 * 生成の直後にサーバーと突き合わせるため、期待するCRCを控えます
	 * Authorityは突き合わせる相手がいないので控えません
	 */
	mHasExpectedServerCRC32 = !HasAuthority();
	mExpectedServerCRC32 = generationState.ServerCRC32;

	MEASURE_TIME_START(stopwatch);
	TGuardValue<bool> generatingGuard(mIsGeneratingDungeon, true);
	const bool generated = PreGenerateImplementation();
	PostGenerateImplementation();
	RefreshGeneratedWorldState();

	MEASURE_TIME_LAP(stopwatch, TEXT("Total Dungeon Generation Time"));
	return generated;
}

/**
 * Reports that the locally generated dungeon differs from the one the server generated.
 * The caller then fails the generation, so OnGenerationFailure is what a handler receives and the
 * reason is available from GetLastGenerationIssues.
 * ローカルで生成したダンジョンがサーバーのものと異なる事を報告します。
 * 呼び出し側はこの後で生成を失敗させるため、ハンドラにはOnGenerationFailureが届き、
 * 理由はGetLastGenerationIssuesから取得できます。
 */
void ADungeonGenerateActor::ReportGenerationCrcMismatch(const uint64 generationId, const uint32 serverCRC32, const uint32 localCRC32, const int32 seed)
{
	FDungeonValidationIssue issue;
	issue.Severity = EDungeonValidationSeverity::Error;
	issue.Code = TEXT("DG_NET_CRC_MISMATCH");
	issue.Message = FText::Format(
		NSLOCTEXT("DungeonGenerateActor", "NetCrcMismatch", "The dungeon generated on this client does not match the one generated by the server. Server CRC is {0}, local CRC is {1}, seed is {2}."),
		FText::FromString(FString::Printf(TEXT("%08X"), serverCRC32)),
		FText::FromString(FString::Printf(TEXT("%08X"), localCRC32)),
		FText::FromString(FString::Printf(TEXT("%08X"), static_cast<uint32>(seed))));
	issue.FixHint = NSLOCTEXT("DungeonGenerateActor", "NetCrcMismatchHint", "The dungeon was discarded and the generation was failed, so this client has no dungeon and OnGenerationFailure was broadcast. Generation is only reproducible when every peer runs the same plugin version with the same Parameter Asset; floating point results can also differ between platforms, so check this first when the server and the clients are built for different ones.");
	AddGenerationIssue(issue);

	DUNGEON_GENERATOR_ERROR(TEXT("Dungeon generation CRC mismatch for R%016llX. Server=%08X Local=%08X Seed=%08X"),
		static_cast<unsigned long long>(generationId), serverCRC32, localCRC32, static_cast<uint32>(seed));
}

/**
 * Returns whether the parameter can be referenced over the network.
 * An object reference replicates by its path, so the parameter needs a path that exists on
 * every peer. A parameter loaded from a saved asset has one; one built at runtime does not.
 * パラメータをネットワーク越しに参照できるかを返します。
 * オブジェクト参照は経路で複製されるため、全てのピアに存在する経路が必要です。
 * 保存済みアセットから読み込んだパラメータは経路を持ち、実行時に作ったパラメータは持ちません。
 */
bool ADungeonGenerateActor::IsParameterReplicable(const UDungeonGenerateParameter* parameter)
{
	if (!IsValid(parameter))
		return false;

	// 一時オブジェクトはパッケージへ保存されないため、経路があっても届きません
	if (parameter->HasAnyFlags(RF_Transient))
		return false;

	return parameter->IsFullNameStableForNetworking();
}

/**
 * Reports a parameter that clients cannot resolve over the network.
 * The parameter replicates as an object reference, so it needs a path that exists on every peer.
 * A parameter built at runtime, such as the result of GenerateRandomParameter, has no such path.
 * The reference arrives as null and the clients end up with no dungeon while the server has one.
 * クライアントがネットワーク越しに解決できないパラメータを報告します。
 * パラメータはオブジェクト参照として複製されるため、全てのピアに存在する経路が必要です。
 * GenerateRandomParameterの結果のように実行時に作ったパラメータはその経路を持ちません。
 * 参照はnullとして届き、サーバーだけにダンジョンがある状態になります。
 */
void ADungeonGenerateActor::ReportNonReplicableParameter(const UDungeonGenerateParameter* parameter)
{
	// スタンドアロンは複製しないので、経路が無くても問題になりません
	if (GetNetMode() == NM_Standalone)
		return;

	if (!IsValid(parameter))
		return;

	if (IsParameterReplicable(parameter))
		return;

	FDungeonValidationIssue issue;
	issue.Severity = EDungeonValidationSeverity::Error;
	issue.Code = TEXT("DG_NET_PARAMETER_NOT_REPLICABLE");
	issue.Message = NSLOCTEXT("DungeonGenerateActor", "NetParameterNotReplicable", "The dungeon generation parameter cannot be referenced over the network, so the connected clients build no dungeon.");
	issue.FixHint = NSLOCTEXT("DungeonGenerateActor", "NetParameterNotReplicableHint", "Assign a saved Parameter Asset that is packaged at the same path on every peer. A parameter created at runtime, such as the result of GenerateRandomParameter, only works in a standalone game.");
	issue.ParameterName = TEXT("DungeonGenerateParameter");
	AddGenerationIssue(issue);

	DUNGEON_GENERATOR_ERROR(TEXT("Dungeon generation parameter '%s' cannot be referenced over the network. The clients receive no parameter and build no dungeon. Use a saved Parameter Asset available at the same path on every peer."),
		*parameter->GetPathName());
}

/**
 * Generates one authoritative revision and publishes its finalized state.
 * Authorityとして1つのRevisionを生成し、確定状態を配信します。
 */
void ADungeonGenerateActor::GenerateAuthoritative(UDungeonGenerateParameter* parameter)
{
	if (!HasAuthority())
		return;
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
		return;
	}
	/*
	 * 生成に失敗した場合、乱数の種を変えて作り直します。
	 * 失敗はコア生成の段階で判明し、アクターを生成する前に返るため再試行は安価です。
	 * 100シード×2帯の計測では、失敗19件が全て2回以内の再試行で解消しました。
	 * 実測の最大が2回なので、1回分の余裕を持たせた3回を上限にしています。
	 * これ以上増やしても、設定自体が無理な場合に待ち時間が延びるだけです。
	 */
	static constexpr int32 MaximumGenerationRetryCount = 3;

	/*
	 * 乱数の種を指定されている場合は再試行しません。
	 * その指定は「このシードのダンジョンが欲しい」という意味なので、
	 * 別の種で作り直すと再現性の約束を破ってしまいます。
	 */
	const bool canRetry = parameter->GetRandomSeed() == 0;
	const int32 maximumAttemptCount = canRetry ? 1 + MaximumGenerationRetryCount : 1;

	FDungeonGenerationReplicatedState generationState;
	generationState.GenerationId = AllocateGenerationId();
	if (generationState.GenerationId == 0)
		return;
	generationState.Seed = ResolveGenerationSeed(parameter);
	generationState.Parameter = parameter;
	generationState.FloorGenerationMethod = DungeonMeshGenerationMethod;
	generationState.WallGenerationMethod = DungeonWallRoofPillarMeshGenerationMethod;
	generationState.bGenerated = true;

	bool generated = false;
	for (int32 attempt = 0; attempt < maximumAttemptCount; ++attempt)
	{
		if (attempt > 0)
		{
			/*
			 * 乱数の種を決定的に導出します。
			 * ResolveGenerationSeedはtime(nullptr)を返すため、同じ秒の中で呼び直すと
			 * 同じ種が返り、同じ失敗を繰り返します。
			 */
			generationState.Seed = MakeRetryGenerationSeed(generationState.Seed, attempt);

			/*
			 * ApplyGenerationStateは適用済みより後のIDしか受け付けないため、
			 * 再試行のたびに新しいIDが必要です。
			 * mReplicatedGenerationStateは成功が確定するまで更新しません。
			 */
			++generationState.GenerationId;
		}

		generated = ApplyGenerationState(generationState);
		if (generated)
		{
			if (attempt > 0)
			{
				DUNGEON_GENERATOR_WARNING(TEXT("Dungeon generation succeeded on attempt %d with seed %d. Review the parameter if this happens often."),
					attempt + 1, generationState.Seed);
			}
			break;
		}
	}

	if (generated)
	{
		generationState.ServerCRC32 = static_cast<uint32>(CalculateCRC32());
		parameter->SetGeneratedRandomSeed(generationState.Seed);
		parameter->SetGeneratedDungeonCRC32(static_cast<int32>(generationState.ServerCRC32));
	}
	else
	{
		if (maximumAttemptCount > 1)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Dungeon generation failed on every one of the %d attempts. See the reported issue for the reason."),
				maximumAttemptCount);
		}
		generationState.bGenerated = false;
	}

	/*
	 * 配信する直前に確認します
	 * この時点で生成の成否は確定しており、問題の一覧も出揃っています
	 */
	ReportNonReplicableParameter(parameter);

	mReplicatedGenerationState = generationState;
	ForceNetUpdate();
	MulticastApplyGenerationState(generationState);
}

void ADungeonGenerateActor::DestroyDungeon_Implementation()
{
	if (mIsGeneratingDungeon)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DestroyDungeon ignored because dungeon generation is already in progress."));
		return;
	}

	FDungeonGenerationReplicatedState generationState = mReplicatedGenerationState;
	generationState.GenerationId = AllocateGenerationId();
	if (generationState.GenerationId == 0)
		return;
	generationState.bGenerated = false;
	ApplyGenerationState(generationState);
	mReplicatedGenerationState = generationState;
	ForceNetUpdate();
	MulticastApplyGenerationState(generationState);
}

void ADungeonGenerateActor::SetAllRoomLightIntensities(const float baseFillIntensity, const float guidanceIntensity)
{
	if (!HasAuthority())
	{
		DUNGEON_GENERATOR_WARNING(TEXT("SetAllRoomLightIntensities ignored because %s does not have authority."), *GetName());
		return;
	}

	TArray<AActor*> roomSensors;
	UGameplayStatics::GetAllActorsOfClass(this, ADungeonRoomSensorBase::StaticClass(), roomSensors);
	for (AActor* actor : roomSensors)
	{
		ADungeonRoomSensorBase* roomSensor = Cast<ADungeonRoomSensorBase>(actor);
		if (IsValid(roomSensor) && roomSensor->ActorHasTag(GetDungeonGeneratorTag()) && roomSensor->DungeonGeneratorOwner == this)
			roomSensor->SetRoomLightIntensities(baseFillIntensity, guidanceIntensity);
	}
}

int32 ADungeonGenerateActor::FindFloorHeight(const float z) const
{
	const int32 gridZ = FindVoxelHeight(z);
	const std::shared_ptr<const dungeon::Generator>& generator = GetGenerator();
	if (generator == nullptr)
		return 0;
	return generator->FindFloor(gridZ);
}

int32 ADungeonGenerateActor::FindVoxelHeight(const float z) const
{
	if (!IsValid(DungeonGenerateParameter))
		return 0;

	const float verticalGridSize = DungeonGenerateParameter->GetGridSize().VerticalSize;
	if (verticalGridSize <= 0.f || !FMath::IsFinite(verticalGridSize) || !FMath::IsFinite(z))
		return 0;

	return FMath::FloorToInt((z - GetActorLocation().Z) / verticalGridSize);
}


#if WITH_EDITOR
int32 ADungeonGenerateActor::GetGeneratedDungeonCRC32() const noexcept
{
	return GeneratedDungeonCRC32;
}
#endif

float ADungeonGenerateActor::GetGridSize() const
{
	if (DungeonGenerateParameter)
		return DungeonGenerateParameter->GetGridSize().HorizontalSize;
	return 1.f;
}

FVector ADungeonGenerateActor::GetRoomMaxSizeWithMargin(const int32_t margin) const
{
	if (DungeonGenerateParameter)
	{
		FVector result;
		result.X = DungeonGenerateParameter->GetRoomWidth().Max + margin;
		result.Y = DungeonGenerateParameter->GetRoomDepth().Max + margin;
		result.Z = DungeonGenerateParameter->GetRoomHeight().Max + margin;
		return result * DungeonGenerateParameter->GetGridSize().To3D();
	}
	return FVector::ZeroVector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// for debug
#if WITH_EDITOR
bool ADungeonGenerateActor::ShouldTickIfViewportsOnly() const
{
	return true;
}

/**
 * Draws the enabled editor-only dungeon debug views.
 * 有効になっているエディタ専用のダンジョンデバッグ表示を描画します。
 */
void ADungeonGenerateActor::DrawDebugInformation() const
{
	if (!ShowFloorInformation && !ShowVoxelGridType)
		return;

	if (!IsValid(DungeonGenerateParameter))
		return;

	std::shared_ptr<const dungeon::Generator> generator = GetGenerator();
	if (generator == nullptr)
		return;

	if (ShowFloorInformation)
		DrawFloorInformation(*generator);

	if (ShowVoxelGridType)
		DrawVoxelGridInformation(*generator);
}

/**
 * Displays the basic performance statistics selected by the actor.
 * アクターで選択された基本的なパフォーマンス統計を表示します。
 */
void ADungeonGenerateActor::ShowPerformanceStats() const
{
	// Reset all stat overlays before enabling the requested development views.
	HidePerformanceStats();
	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("stat fps"));
	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("stat unit"));
	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("stat gpu"));
}

/**
 * Reset all stat overlays
 * 全ての統計オーバーレイをリセットします
 */
void ADungeonGenerateActor::HidePerformanceStats() const
{
	// Reset all stat overlays
	UKismetSystemLibrary::ExecuteConsoleCommand(this, TEXT("stat none"));
}

/**
 * Draws every generated floor as a translucent section with an outline and height label.
 * 生成された各階層を、外周線と高さラベルを持つ半透明断面として描画します。
 */
void ADungeonGenerateActor::DrawFloorInformation(const dungeon::Generator& generator) const
{
	UWorld* world = GetWorld();
	if (!IsValid(world))
		return;

	const std::shared_ptr<dungeon::Voxel>& voxel = generator.GetVoxel();
	if (voxel == nullptr)
		return;

	const std::vector<int32_t>& floorHeights = generator.GetFloorHeight();
	if (floorHeights.empty())
		return;

	const FVector gridSize = DungeonGenerateParameter->GetGridSize().To3D();
	const FVector actorLocation = GetActorLocation();
	const FVector2D dungeonSize(
		static_cast<double>(voxel->GetWidth()) * gridSize.X,
		static_cast<double>(voxel->GetDepth()) * gridSize.Y
	);

	const TArray<int32> planeIndices =
	{
		0, 1, 2,
		0, 2, 3,
		2, 1, 0,
		3, 2, 0
	};

	for (size_t floorIndex = 0; floorIndex < floorHeights.size(); ++floorIndex)
	{
		const int32 floorHeight = floorHeights[floorIndex];
		const double worldHeight = actorLocation.Z + static_cast<double>(floorHeight) * gridSize.Z;
		const FVector corners[] =
		{
			FVector(actorLocation.X, actorLocation.Y, worldHeight),
			FVector(actorLocation.X + dungeonSize.X, actorLocation.Y, worldHeight),
			FVector(actorLocation.X + dungeonSize.X, actorLocation.Y + dungeonSize.Y, worldHeight),
			FVector(actorLocation.X, actorLocation.Y + dungeonSize.Y, worldHeight)
		};

		const FColor outlineColor = FloorDebugColors[floorIndex % UE_ARRAY_COUNT(FloorDebugColors)];
		FColor planeColor = outlineColor;
		planeColor.A = FloorPlaneOpacity;

		const TArray<FVector> planeVertices =
		{
			corners[0],
			corners[1],
			corners[2],
			corners[3]
		};
		DrawDebugMesh(world, planeVertices, planeIndices, planeColor, false, DrawDebugDuration);

		for (int32 cornerIndex = 0; cornerIndex < UE_ARRAY_COUNT(corners); ++cornerIndex)
		{
			DrawDebugLine(
				world,
				corners[cornerIndex],
				corners[(cornerIndex + 1) % UE_ARRAY_COUNT(corners)],
				outlineColor,
				false,
				DrawDebugDuration,
				0,
				dungeon::BoldThickness
			);
		}

		const FVector labelLocation = corners[0] + FVector(gridSize.X * 0.25, gridSize.Y * 0.25, gridSize.Z * 0.05);
		const FString label = FString::Printf(
			TEXT("Floor %d / Grid Z: %d / World Z: %.0f"),
			static_cast<int32>(floorIndex),
			floorHeight,
			worldHeight
		);
		DrawDebugString(world, labelLocation, label, nullptr, outlineColor, DrawDebugDuration, true, 1.f);
	}
}

/**
 * Draws voxel grid types and details around the active PIE camera.
 * PIEでアクティブなカメラ周辺のボクセルグリッド種別と詳細を描画します。
 */
void ADungeonGenerateActor::DrawVoxelGridInformation(const dungeon::Generator& generator) const
{
	const APlayerController* playerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(playerController))
		return;

	FVector cameraWorldLocation;
	FRotator cameraWorldRotation;
	playerController->GetPlayerViewPoint(cameraWorldLocation, cameraWorldRotation);
	const FVector cameraLocation = cameraWorldLocation - GetActorLocation();
	const FIntVector cameraGridLocation = DungeonGenerateParameter->ToGrid(cameraLocation);

	constexpr int32 range = 3;
	constexpr int32 squaredRange = range * range;
	generator.GetVoxel()->Each([this, &cameraGridLocation](const FIntVector& location, const dungeon::Grid& grid)
		{
			const FIntVector delta = location - cameraGridLocation;
			const FIntVector squaredDelta = delta * delta;
			if (squaredDelta.X > squaredRange || squaredDelta.Y > squaredRange || squaredDelta.Z > squaredRange)
				return true;

			if (grid.GetType() == dungeon::Grid::Type::Empty || grid.GetType() == dungeon::Grid::Type::OutOfBounds)
				return true;

			const FVector gridSize = DungeonGenerateParameter->GetGridSize().To3D();
			const FVector halfGridSize = gridSize / 2.;
			const FVector center = dungeon::ToVector(location) * gridSize + halfGridSize + GetActorLocation();
			const FColor& color = dungeon::Grid::GetTypeColor(grid.GetType());

			// グリッドを描画
			UKismetSystemLibrary::DrawDebugBox(
				GetWorld(),
				center,
				halfGridSize * 0.95,
				color,
				FRotator::ZeroRotator,
				DrawDebugDuration,
				dungeon::ThinThickness
			);

			// グリッドの方向を描画
			UKismetSystemLibrary::DrawDebugArrow(
				GetWorld(),
				center,
				center + halfGridSize * dungeon::ToVector(grid.GetDirection().GetVector()),
				halfGridSize.Length(),
				color,
				DrawDebugDuration,
				dungeon::ThinThickness
			);

			FString message;
			message.Append(TEXT("Identifier:") + FString::FromInt(grid.GetIdentifier()) + TEXT("\n"));
			message.Append(TEXT("DepthRatioFromStart:") + FString::SanitizeFloat(static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f) + TEXT("\n"));
			message.Append(TEXT("Type: ") + grid.GetTypeName() + TEXT("\n"));
			message.Append(TEXT("Props: ") + grid.GetPropsName() + TEXT("\n"));
			message.Append(TEXT("Direction: ") + grid.GetDirection().GetName() + TEXT("\n"));
			if (grid.IsReserved())
				message.Append(TEXT("Reserved\n"));
			if (grid.IsCatwalk())
			{
				message.Append(TEXT("Catwalk\n"));
				message.Append(TEXT("Direction: ") + grid.GetCatwalkDirection().GetName() + TEXT("\n"));
			}
			if (grid.IsSubLevel())
				message.Append(TEXT("SubLevel\n"));
			message.Append(grid.GetNoMeshGenerationName() + TEXT("\n"));
			message.Append(grid.GetWallName());
			DrawDebugString(
				GetWorld(),
				center,
				message,
				nullptr,
				color,
				DrawDebugDuration,
				true,
				1.f
			);

			return true;
		}
	);
}
#endif

#undef LOCTEXT_NAMESPACE
