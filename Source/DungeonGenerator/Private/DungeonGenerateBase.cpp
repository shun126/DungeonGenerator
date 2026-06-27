/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 *
 * ADungeonGeneratedActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
 * ADungeonGenerateActorは配置可能(Placeable)、ADungeonGeneratedActorは配置不可能(NotPlaceable)にするため、
 * 継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
 */

#include "DungeonGenerateBase.h"


#include "Core/Generator.h"
#include "Core/Debug/Debug.h"
#include "Core/Debug/Config.h"
#include "Core/Debug/MeasureTime.h"
#include "Core/Helper/Direction.h"
#include "Core/Helper/Identifier.h"
#include "Core/Helper/Stopwatch.h"
#include "Core/Math/Math.h"
#include "Core/Math/Random.h"
#include "Core/PathGeneration/StartLocationPolicy.h"
#include "Core/RoomGeneration/Room.h"
#include "Core/Voxelization/Voxel.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "Mission/DungeonRoomProps.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "Parameter/Selector/DungeonGridIndexPartsSelector.h"
#include "SubActor/DungeonDoorBase.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include "Validation/DungeonParameterValidator.h"


#include "PluginInformation.h"

#include "Helper/DungeonAisleGridMap.h"
#include "Helper/DungeonDirection.h"

#include <Camera/PlayerCameraManager.h>
#include <Model.h>
#include <FoliageInstancedStaticMeshComponent.h>
#include <TextureResource.h>
#include <Components/BrushComponent.h>
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Components/InstancedStaticMeshComponent.h>
#include <Components/PointLightComponent.h>
#include <Components/StaticMeshComponent.h>
#include <GameFramework/PlayerStart.h>
#include <Engine/Level.h>
#include <Engine/LevelStreamingDynamic.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/StaticMeshActor.h>
#include <Engine/Texture2D.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/PackageName.h>
#include <NavMesh/NavMeshBoundsVolume.h>
#include <NavMesh/RecastNavMesh.h>
#include <Engine/Polys.h>
#include <UObject/Package.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <unordered_map>


#if WITH_EDITOR
// UnrealEd
#include <Editor.h>
#include <EditorActorFolders.h>
#include <EditorLevelUtils.h>
#include <Builders/CubeBuilder.h>
#endif

namespace
{
	/*
	 * Converts the public start-room policy to the internal generation policy.
	 * 公開開始部屋方針を内部生成方針へ変換します。
	 */
	dungeon::StartLocationPolicy ToInternalStartLocationPolicy(const EDungeonStartLocationPolicy policy) noexcept
	{
		switch (policy)
		{
		case EDungeonStartLocationPolicy::UseNorthernMost:
			return dungeon::StartLocationPolicy::UseNorthernMost;
		case EDungeonStartLocationPolicy::UseEasternMost:
			return dungeon::StartLocationPolicy::UseEasternMost;
		case EDungeonStartLocationPolicy::UseWesternMost:
			return dungeon::StartLocationPolicy::UseWesternMost;
		case EDungeonStartLocationPolicy::UseHighestPoint:
			return dungeon::StartLocationPolicy::UseHighestPoint;
		case EDungeonStartLocationPolicy::UseLowestPoint:
			return dungeon::StartLocationPolicy::UseLowestPoint;
		case EDungeonStartLocationPolicy::UseCentralPoint:
			return dungeon::StartLocationPolicy::UseCentralPoint;
		case EDungeonStartLocationPolicy::UseMultiStart:
			return dungeon::StartLocationPolicy::UseMultiStart;
		case EDungeonStartLocationPolicy::UseSouthernMost:
		default:
			return dungeon::StartLocationPolicy::UseSouthernMost;
		}
	}
}

namespace
{
	ADungeonMainLevelScriptActor* FindDungeonMainLevelScriptActor(UWorld* world)
	{
		if (!IsValid(world) || !world->PersistentLevel)
		{
			return nullptr;
		}

		return Cast<ADungeonMainLevelScriptActor>(world->PersistentLevel->GetLevelScriptActor());
	}

	const FString ActorsFolderPath = TEXT("Actors");
	const FString DoorsFolderPath = TEXT("Actors/Doors");
	const FString TorchesFolderPath = TEXT("Actors/Torches");
	const FString ChandeliersFolderPath = TEXT("Actors/Chandeliers");
	const FString SensorsFolderPath = TEXT("Actors/Sensors");
	const FString LevelsFolderPath = TEXT("/Levels/");
	const FString InteriorsFolderPath = TEXT("Interiors");

	constexpr bool operator==(const EDungeonRoomItem left, const dungeon::Room::Item right)
	{
		return static_cast<uint8_t>(left) == static_cast<uint8_t>(right);
	}

	constexpr EDungeonRoomItem Cast(const dungeon::Room::Item item)
	{
		return static_cast<EDungeonRoomItem>(item);
	}

	constexpr EDungeonRoomLocatorParts Cast(const dungeon::Room::Parts parts)
	{
		return static_cast<EDungeonRoomLocatorParts>(parts);
	}
	uint8 MakeNeighborMask6(const dungeon::Grid& grid)
	{
		uint8 mask = 0;
		if (grid.HasNorthWall() == false)
			mask |= 1 << 0; // North
		if (grid.HasEastWall() == false)
			mask |= 1 << 1; // East
		if (grid.HasSouthWall() == false)
			mask |= 1 << 2; // South
		if (grid.HasWestWall() == false)
			mask |= 1 << 3; // West
		if (grid.HasFloor())
			mask |= 1 << 4; // Floor
		if (grid.HasCeiling())
			mask |= 1 << 5; // Ceiling
		return mask;
	}

	bool IsVisibleGeneratedGrid(const dungeon::Grid& grid) noexcept
	{
		if (grid.Is(dungeon::Grid::Type::UpSpace) || grid.Is(dungeon::Grid::Type::Stairwell))
			return false;
		return grid.Is(dungeon::Grid::Type::Slope) || grid.Is(dungeon::Grid::Type::DownSpace) || grid.CanBuildFloor(false);
	}

#if WITH_EDITOR
	FString NormalizeEditorPackageNameForComparison(const FString& packageName)
	{
		FString normalized = packageName;
		int32 slashIndex = INDEX_NONE;
		if (!normalized.FindLastChar(TEXT('/'), slashIndex) || slashIndex + 1 >= normalized.Len())
			return normalized;

		FString leafName = normalized.Mid(slashIndex + 1);
		if (!leafName.StartsWith(TEXT("UEDPIE_")))
			return normalized;

		const int32 piePrefixEnd = leafName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FCString::Strlen(TEXT("UEDPIE_")));
		if (piePrefixEnd == INDEX_NONE || piePrefixEnd + 1 >= leafName.Len())
			return normalized;

		leafName = leafName.Mid(piePrefixEnd + 1);
		normalized = normalized.Left(slashIndex + 1) + leafName;
		return normalized;
	}

	FSoftObjectPath MakeEditorWorldAssetPathFromLevel(const ULevel* level)
	{
		if (!IsValid(level) || !IsValid(level->GetOutermost()))
			return FSoftObjectPath();

		const FString packageName = NormalizeEditorPackageNameForComparison(level->GetOutermost()->GetName());
		if (packageName.IsEmpty())
			return FSoftObjectPath();

		return FSoftObjectPath(packageName + TEXT(".") + FPackageName::GetShortName(packageName));
	}
#endif
}

ADungeonGenerateBase::ADungeonGenerateBase(const FObjectInitializer& initializer)
	: Super(initializer)
{
	// Create root scene component
	RootComponent = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"), true);
	check(RootComponent);

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	const AddStaticMeshEvent addFloorStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Floor"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, EStaticMeshPartitionRegistrationFace::PositiveZ);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addFloorStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};
	const AddStaticMeshEvent addSlopeStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Slope"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, EStaticMeshPartitionRegistrationFace::PositiveZ);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addSlopeStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};
	const AddStaticMeshEvent addWallStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Wall"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, EStaticMeshPartitionRegistrationFace::PositiveY);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addWallStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};
	const AddStaticMeshEvent addRoofStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Roof"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, EStaticMeshPartitionRegistrationFace::NegativeZ);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addRoofStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};
	const AddPillarStaticMeshEvent addPillarStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Pillars"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addPillarStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};
	const AddStaticMeshEvent addCatwalkStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Catwalk"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_VERBOSE(TEXT("addFloorStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#else
			(void)(actor);
#endif
		};

	mOnAddFloor = addFloorStaticMeshEvent;
	mOnAddSlope = addSlopeStaticMeshEvent;
	mOnAddWall = addWallStaticMeshEvent;
	mOnAddRoof = addRoofStaticMeshEvent;
	mOnAddPillar = addPillarStaticMeshEvent;
	mOnAddCatwalk = addCatwalkStaticMeshEvent;
}

void ADungeonGenerateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Dispose(false);

	// Calling the parent class
	Super::EndPlay(EndPlayReason);
}

void ADungeonGenerateBase::Tick(float DeltaSeconds)
{
	// Calling the parent class
	Super::Tick(DeltaSeconds);

	const ADungeonMainLevelScriptActor* mainLevelScriptActor = FindDungeonMainLevelScriptActor(GetWorld());
	mDungeonDeferredActorSpawnManager.Update([mainLevelScriptActor](const int32 partitionIndex)
	{
		return IsValid(mainLevelScriptActor) && mainLevelScriptActor->IsPartitionActive(partitionIndex);
	});

}

/*
 * 同期用の乱数を取得します。必ずサーバーとクライアントが同じ回数呼び出すようにして下さい。
 */
std::shared_ptr<dungeon::Random> ADungeonGenerateBase::GetSynchronizedRandom() const noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}

/*
 * ローカル用の乱数を取得します。レプリケーションで同期する事を想定しています。
 */
const std::shared_ptr<dungeon::Random>& ADungeonGenerateBase::GetRandom() const noexcept
{
	return mLocalRandom;
}

UDungeonComponentActivatorComponent* ADungeonGenerateBase::FindOrAddComponentActivatorComponent(AActor* actor)
{
	UDungeonComponentActivatorComponent* component = nullptr;
	if (IsValid(actor))
	{
		component = actor->FindComponentByClass<UDungeonComponentActivatorComponent>();
		if (component == nullptr)
		{
			component = NewObject<UDungeonComponentActivatorComponent>(actor);
			actor->AddInstanceComponent(component);
			component->RegisterComponent();
		}
	}
	return component;
}

void ADungeonGenerateBase::DeferredSpawnActorWithFolderPath(UClass* actorClass, const FString& folderPath,
	const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters,
	const TFunction<void(AActor*)>& onSpawned)
{
	DeferredSpawnActorWithFolderPath(GetWorld(), actorClass, folderPath, transform, actorSpawnParameters, onSpawned);
}

void ADungeonGenerateBase::DeferredSpawnActorWithFolderPath(UWorld* world, UClass* actorClass,
	const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters,
	const TFunction<void(AActor*)>& onSpawned)
{
	int32 partitionIndex = INDEX_NONE;
	if (const ADungeonMainLevelScriptActor* mainLevelScriptActor = FindDungeonMainLevelScriptActor(world))
	{
		partitionIndex = mainLevelScriptActor->FindPartitionIndex(transform.GetLocation());
	}

	mDungeonDeferredActorSpawnManager.RequestSpawn(
		world,
		actorClass,
		folderPath,
		transform,
		actorSpawnParameters,
		partitionIndex,
		onSpawned
	);

	SetActorTickEnabled(true);
}

AActor* ADungeonGenerateBase::SpawnActorWithFolderPath(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters)
{
	if (!IsValid(world))
		return nullptr;

	AActor* actor = world->SpawnActor(actorClass, &transform, actorSpawnParameters);
	if (actor)
	{
#if WITH_EDITOR
		actor->SetFolderPath(FName(dungeon::GetBaseDirectoryName() + TEXT("/") + folderPath));
#endif

		actor->Tags.Reserve(1);
		actor->Tags.Emplace(GetDungeonGeneratorTag());
	}

	return actor;
}

/*
 * アクターをスポーンします。
 * DungeonGeneratorというタグを追加します。
 * スポーンしたアクターはDestroySpawnedActorsで破棄されます。
 */
AActor* ADungeonGenerateBase::SpawnActorWithFolderPath(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const
{
	UWorld* world = GetWorld();
	if (IsValid(world) == false)
		return nullptr;

	return SpawnActorWithFolderPath(world, actorClass, folderPath, transform, actorSpawnParameters);
}

/*
 * スポーンしたアクターを全て破棄します
 * DungeonGeneratorというタグが付いたアクターが対象です。
 */
void ADungeonGenerateBase::DestroySpawnedActors() const
{
	DestroySpawnedActors(GetWorld());
}

/*
 * スポーンしたアクターを全て破棄します
 * DungeonGeneratorというタグが付いたアクターが対象です。
 */
void ADungeonGenerateBase::DestroySpawnedActors(UWorld* world)
{
	if (!IsValid(world))
		return;

	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsWithTag(world, GetDungeonGeneratorTag(), actors);

#if WITH_EDITOR
	// Standaloneモードによる起動ではGEditorやGEngineが無効になる
	if (IsValid(GEditor))
	{
		TArray<FFolder> deleteFolders;

		// TagにDungeonGeneratorTagがついているアクターのフォルダを回収
		for (const AActor* actor : actors)
		{
			if (IsValid(actor))
			{
				const FFolder& folder = actor->GetFolder();

				if (!folder.IsValid())
					continue;
				if (folder.GetPath() == folder.GetEmptyPath())
					continue;

				if (const auto* actorFolder = folder.GetActorFolder())
				{
					if (!actorFolder->IsValid())
						continue;
				}

				deleteFolders.AddUnique(folder);
			}
		}

		// パスが長い順に並べ替え
		deleteFolders.Sort([](const FFolder& l, const FFolder& r)
			{
				return l.GetPath().GetStringLength() > r.GetPath().GetStringLength();
			}
		);

		// フォルダを削除
		for (FFolder& folder : deleteFolders)
		{
			FActorFolders::Get().DeleteFolder(*world, folder);
		}
	}
#endif

	// アクターの削除
	for (AActor* actor : actors)
	{
		if (IsValid(actor))
		{
			actor->Destroy();
		}
	}
}

std::shared_ptr<const dungeon::Generator> ADungeonGenerateBase::GetGenerator() const
{
	return mGenerator;
}

bool ADungeonGenerateBase::GetVisibleGridHeightRange(int32& minZ, int32& maxZ) const noexcept
{
	if (mVisibleGridHeightRangeValid == false)
		return false;

	minZ = mMinVisibleGridZ;
	maxZ = mMaxVisibleGridZ;
	return true;
}

/*
 * Clears the cached visible grid height range when the generated dungeon changes.
 * 生成済みダンジョンが変わるときに、表示可能なグリッド高さ範囲のキャッシュをクリアします。
 */
void ADungeonGenerateBase::InvalidateVisibleGridHeightRange() noexcept
{
	mVisibleGridHeightRangeValid = false;
	mMinVisibleGridZ = 0;
	mMaxVisibleGridZ = 0;
}

/*
 * Caches the Z range that contains visible generated grid cells.
 * 表示可能な生成済みグリッドセルを含むZ範囲をキャッシュします。
 */
void ADungeonGenerateBase::CacheVisibleGridHeightRange() noexcept
{
	InvalidateVisibleGridHeightRange();

	if (mGenerator == nullptr || mGenerator->GetLastError() != dungeon::Generator::Error::Success)
		return;

	const std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel();
	if (voxel == nullptr)
		return;

	voxel->Each([this](const FIntVector& location, const dungeon::Grid& grid) -> bool
		{
			if (IsVisibleGeneratedGrid(grid) == false)
				return true;

			if (mVisibleGridHeightRangeValid == false)
			{
				mMinVisibleGridZ = location.Z;
				mMaxVisibleGridZ = location.Z;
				mVisibleGridHeightRangeValid = true;
			}
			else
			{
				mMinVisibleGridZ = FMath::Min(mMinVisibleGridZ, location.Z);
				mMaxVisibleGridZ = FMath::Max(mMaxVisibleGridZ, location.Z);
			}

			return true;
		}
	);
}

void ADungeonGenerateBase::OnPreDungeonGeneration()
{
}

void ADungeonGenerateBase::OnPostDungeonGeneration(const bool result)
{
}














bool ADungeonGenerateBase::IsGenerated() const noexcept
{
	return mGenerated;
}

void ADungeonGenerateBase::Dispose(const bool flushStreamLevels)
{
	mDungeonDeferredActorSpawnManager.CancelAll(/*bNotifyCallbacks=*/false);

	// 生成済みなら破棄する
	if (mGenerated == true)
	{

		// スポーン済みアクターを破棄
		DestroySpawnedActors();

		// ジェネレータを解放
		mGenerator.reset();
		InvalidateVisibleGridHeightRange();

		// 生成したパラメータを解放
		mParameter = nullptr;

		// 生成済みフラグをリセットする
		mGenerated = false;
	}
}

/*
 * hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
 * 例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
 */

bool ADungeonGenerateBase::BeginDungeonGeneration(const UDungeonGenerateParameter* parameter, const bool hasAuthority)
{
	MEASURE_TIME_START(stopwatch);
	InvalidateVisibleGridHeightRange();
	dungeon::GenerateParameter generateParameter;
	if (!BeginDungeonGenerationPhase_Prepare(parameter, hasAuthority, generateParameter))
	{
		return false;
	}
	MEASURE_TIME_LAP(stopwatch, TEXT(" BeginDungeonGenerationPhase_Prepare"));

	if (!BeginDungeonGenerationPhase_InitializeCore(generateParameter))
	{
		return false;
	}
	MEASURE_TIME_LAP(stopwatch, TEXT(" BeginDungeonGenerationPhase_InitializeCore"));

	if (!BeginDungeonGenerationPhase_RunGenerator(generateParameter, hasAuthority))
	{
		return false;
	}
	MEASURE_TIME_LAP(stopwatch, TEXT(" BeginDungeonGenerationPhase_RunGenerator"));

	BeginDungeonGenerationPhase_BuildWorld(generateParameter, hasAuthority);
	MEASURE_TIME_LAP(stopwatch, TEXT(" BeginDungeonGenerationPhase_BuildWorld"));

	return true;
}

bool ADungeonGenerateBase::BeginDungeonGenerationPhase_Prepare(const UDungeonGenerateParameter* parameter, const bool hasAuthority, dungeon::GenerateParameter& generateParameter)
{
	check(mGenerated == false);

#if WITH_EDITOR
	dungeon::CreateDebugDirectory();
#endif

	DUNGEON_GENERATOR_LOG(TEXT("version '%s', license '%s', build '%s', uuid '%s', commit '%s', HasAuthority '%s'"),
		TEXT(DUNGEON_GENERATOR_PLUGIN_VERSION_NAME),
		TEXT(JENKINS_LICENSE),
		TEXT(JENKINS_BUILD_TAG),
		TEXT(JENKINS_UUID),
		TEXT(JENKINS_GIT_COMMIT),
		hasAuthority ? TEXT("Yes") : TEXT("No")
	);

	// CRC32の値を初期化
	mCrc32AtCreation = ~0;

	// Conversion from UDungeonGenerateParameter to dungeon::GenerateParameter
	if (IsValid(parameter) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the dungeon generation parameters"));
		return false;
	}

	mDungeonDeferredActorSpawnManager.CancelAll(/*bNotifyCallbacks=*/false);


	TArray<FDungeonValidationIssue> validationIssues;
	FDungeonParameterValidator::Validate(parameter, validationIssues, false);
	for (const FDungeonValidationIssue& issue : validationIssues)
	{
		if (issue.Severity == EDungeonValidationSeverity::Error)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Validation Error [%s] %s | Hint: %s"), *issue.Code.ToString(), *issue.Message.ToString(), *issue.FixHint.ToString());
		}
		else if (issue.Severity == EDungeonValidationSeverity::Warning)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("Validation Warning [%s] %s | Hint: %s"), *issue.Code.ToString(), *issue.Message.ToString(), *issue.FixHint.ToString());
		}
		else
		{
			DUNGEON_GENERATOR_LOG(TEXT("Validation Info [%s] %s | Hint: %s"), *issue.Code.ToString(), *issue.Message.ToString(), *issue.FixHint.ToString());
		}
	}
	if (validationIssues.ContainsByPredicate([](const FDungeonValidationIssue& issue) { return issue.Severity == EDungeonValidationSeverity::Error; }))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Dungeon generation aborted due to validation errors."));
		return false;
	}

	// UDungeonGenerateParameterを保存
	mParameter = parameter;


	// ダンジョン生成パラメータを生成
	{
		int32 randomSeed;
		if (hasAuthority)
		{
			// Server
			randomSeed = mParameter->GetRandomSeed();
			if (randomSeed == 0)
				randomSeed = static_cast<int32>(time(nullptr));
			const_cast<UDungeonGenerateParameter*>(mParameter)->SetGeneratedRandomSeed(randomSeed);
		}
		else
		{
			// Client
			randomSeed = mParameter->GetGeneratedRandomSeed();
		}
		generateParameter.GetRandom()->SetSeed(randomSeed);
		const auto& structure = mParameter->GetStructureSettings();
		const auto& path = mParameter->GetPathSettings();
		generateParameter.SetNumberOfCandidateRooms(static_cast<uint8>(FMath::Clamp(structure.RoomCountRange.Max, 3, 255)));
		generateParameter.SetMinRoomWidth(structure.RoomWidth.Min);
		generateParameter.SetMaxRoomWidth(structure.RoomWidth.Max);
		generateParameter.SetMinRoomDepth(structure.RoomDepth.Min);
		generateParameter.SetMaxRoomDepth(structure.RoomDepth.Max);
		generateParameter.SetMinRoomHeight(structure.RoomHeight.Min);
		generateParameter.SetMaxRoomHeight(structure.RoomHeight.Max);
		generateParameter.SetMissionGraph(mParameter->IsUseMissionGraph());
		generateParameter.SetAisleComplexity(mParameter->GetAisleComplexity());
		generateParameter.SetAisleCeilingHeightPolicy(static_cast<dungeon::AisleCeilingHeightPolicy>(mParameter->GetAisleCeilingHeightPolicy()));
		generateParameter.SetGenerateSlopeInRoom(true);
		generateParameter.SetGenerateStructuralColumn(true);
		generateParameter.SetSkylightChancePercent(8);
		generateParameter.SetPathSettings(path);
		generateParameter.SetRoomRoleSettings(mParameter->GetRoomRoleSettings());
		generateParameter.SetZoneSettings(mParameter->GetZoneSettings());
		generateParameter.SetLayoutCandidateCount(path.LayoutCandidateCount);
		EDungeonStartLocationPolicy startLocationPolicy = path.StartRoomPolicy;
		if (mParameter->IsUseMissionGraph())
		{
			if (startLocationPolicy == EDungeonStartLocationPolicy::UseMultiStart)
			{
				DUNGEON_GENERATOR_WARNING(TEXT("Path.StartRoomPolicy=UseMultiStart requires Keys And Locks disabled. Falling back to UseSouthernMost."));
				startLocationPolicy = EDungeonStartLocationPolicy::UseSouthernMost;
			}
		}
		generateParameter.SetStartLocationPolicy(ToInternalStartLocationPolicy(startLocationPolicy));
		uint8 startRoomCount = 1;
		if (startLocationPolicy == EDungeonStartLocationPolicy::UseMultiStart)
		{
			TArray<APlayerStart*> startPoints;
			CollectPlayerStartExceptPlayerStartPIE(startPoints);
			if (startPoints.IsEmpty())
			{
				DUNGEON_GENERATOR_WARNING(TEXT("UseMultiStart requires at least one PlayerStart. Falling back to a single start room."));
			}
			startRoomCount = static_cast<uint8>(FMath::Clamp(startPoints.Num(), 1, 255));
		}
		generateParameter.SetStartRoomCount(startRoomCount);

		generateParameter.SetHorizontalRoomMargin(structure.HorizontalRoomMargin);
		if (structure.FloorMode == EDungeonFloorMode::Flat)
		{
			generateParameter.SetVerticalRoomMargin(0);
			generateParameter.SetExpansionPolicy(dungeon::ExpansionPolicy::Flat);
		}
		else
		{
			generateParameter.SetVerticalRoomMargin(structure.VerticalRoomMargin);
			const auto autoFloorCount = structure.FloorMode == EDungeonFloorMode::Vertical ?
				FMath::Max(0, structure.RoomCountRange.Max - 1) :
				3;
			switch (structure.FloorMode)
			{
			case EDungeonFloorMode::Vertical:
				generateParameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandVertically);
				break;
			case EDungeonFloorMode::Free:
			default:
				generateParameter.SetExpansionPolicy(dungeon::ExpansionPolicy::ExpandAnyDirection);
				break;
			}
		}

		check(generateParameter.GetMinRoomWidth() <= generateParameter.GetMaxRoomWidth());
		check(generateParameter.GetMinRoomDepth() <= generateParameter.GetMaxRoomDepth());
		check(generateParameter.GetMinRoomHeight() <= generateParameter.GetMaxRoomHeight());
	}

	return true;
}

bool ADungeonGenerateBase::BeginDungeonGenerationPhase_InitializeCore(const dungeon::GenerateParameter& generateParameter)
{
	// クライアント用乱数生成器を初期化
	mLocalRandom = std::make_shared<dungeon::Random>(generateParameter.GetRandom()->Get<uint32_t>());

	// ダンジョン生成コアの初期設定
	mGenerator = std::make_shared<dungeon::Generator>();
	if (mGenerator == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("System initialization failed."));
		return false;
	}

	// 生成開始イベントの通知
	MEASURE_TIME_START(stopwatch);
	BeginGeneration();
	OnBeginGeneration.Broadcast();
	MEASURE_TIME_LAP(stopwatch, TEXT("  On start generation event"));

	// 通路グリッド記録クラスを生成
	mAisleGridMap = NewObject<UDungeonAisleGridMap>(this);


	return true;
}

bool ADungeonGenerateBase::BeginDungeonGenerationPhase_RunGenerator(dungeon::GenerateParameter& generateParameter, const bool hasAuthority)
{
#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	{
		uint32_t x, y, z, w;
		generateParameter.GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation start: Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, %s"),
			x, y, z, w, mCrc32AtCreation, hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation start:       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, %s"),
			x, y, z, w, mCrc32AtCreation, hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif

	// ダンジョンを生成
	OnPreDungeonGeneration();
	mGenerator->Generate(generateParameter);
	const dungeon::Generator::Error generatorError = mGenerator->GetLastError();
	OnPostDungeonGeneration(dungeon::Generator::Error::Success == generatorError);

	// 生成エラーを確認する
	if (dungeon::Generator::Error::Success != generatorError)
	{
		InvalidateVisibleGridHeightRange();
#if WITH_EDITOR
		// デバッグに必要な情報（デバッグ生成パラメータ）を出力する
		//mParameter->DumpToJson();
#endif
		return false;
	}

	CacheVisibleGridHeightRange();

	{
		const FDungeonLayoutMetrics& metrics = mGenerator->GetLastLayoutMetrics();
		const FDungeonLayoutScore& score = mGenerator->GetLastLayoutScore();
		DUNGEON_GENERATOR_LOG(TEXT("Layout diagnostics: Candidate=%d Score=%f Rooms=%d Aisles=%d CriticalPath=%d Branches=%d Loops=%d SpecialDeadEndCoverage=%f VerticalTransitions=%d StartGoalDistance=%f MissionSolvable=%s"),
			score.CandidateIndex,
			score.TotalScore,
			metrics.RoomCount,
			metrics.AisleCount,
			metrics.CriticalPathLength,
			metrics.BranchCount,
			metrics.LoopCount,
			metrics.SpecialDeadEndCoverage,
			metrics.VerticalTransitionCount,
			metrics.StartGoalDistance,
			metrics.bMissionSolvable ? TEXT("true") : TEXT("false")
		);
	}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		generateParameter.GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p0   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif

	return true;
}

void ADungeonGenerateBase::BeginDungeonGenerationPhase_BuildWorld(const dungeon::GenerateParameter& generateParameter, const bool hasAuthority)
{
	MEASURE_TIME_START(stopwatch);


	// メッシュの生成
	{
		RoomAndRoomSensorMap roomSensorCache;
		CreateImplement_PrepareSpawnRoomSensor(roomSensorCache, hasAuthority);
		CreateImplement_QueryAisleGeneration(hasAuthority);
		CreateImplement_AddTerrain(roomSensorCache, hasAuthority);
		/*
		 * 壁を生成します。
		 * CreateImplement_FinishSpawnInteriorよりも前にする事で内装物を壁にめり込まないようにします
		 * しかし、壁に穴や突起物がある場合、内装物が壁に引っかかる現象が発生します
		 */
		CreateImplement_AddWall();
		CreateImplement_AddChandelier(roomSensorCache, hasAuthority);
		CreateImplement_Navigation(hasAuthority);
		// DungeonRoomSensor::OnInitializeを呼び出す
		CreateImplement_FinishSpawnRoomSensor(roomSensorCache);
	}

	// Blueprintから使用できる乱数を生成します
	UDungeonRandom* random = NewObject<UDungeonRandom>(this);
	random->SetOwner(GetSynchronizedRandom());
	EndGeneration(random, mAisleGridMap);
	OnEndGeneration.Broadcast(random, mAisleGridMap);

	mParameter->OnEndGeneration(random, mAisleGridMap, [this, hasAuthority](const FSoftObjectPath& spawnPath, const FTransform& transform)
		{
			if (hasAuthority)
			{
				const FSoftObjectPath path(spawnPath.ToString() + "_C");
				const TSoftClassPtr<AActor> softClassPointer(path);
				auto* actorClass = softClassPointer.LoadSynchronous();

				FActorSpawnParameters actorSpawnParameters;
				actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
				SpawnActorWithFolderPath(actorClass, ActorsFolderPath, transform, actorSpawnParameters);
			}
		}
	);
	MEASURE_TIME_LAP(stopwatch, TEXT("  On end generation event"));

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		generateParameter.GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation end  : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation end  :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif

	// 通路グリッド記録クラスを解放
	mAisleGridMap = nullptr;
	mGenerated = true;
}

void ADungeonGenerateBase::EndDungeonGeneration()
{
	if (mGenerated)
	{
		// 成功を通知
		OnGenerationSuccess.Broadcast();
	}
	else
	{
		// 失敗を通知
		OnGenerationFailure.Broadcast();
	}

}

void ADungeonGenerateBase::BeginGeneration_Implementation()
{
}

bool ADungeonGenerateBase::OnQueryAisleGeneration_Implementation(const FVector& center, const int32 identifier, const EDungeonDirection direction)
{
	return false;
}

void ADungeonGenerateBase::EndGeneration_Implementation(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap)
{
}

/*
 * ボクセル情報に従って通路生成のイベントを発生させます
 */
void ADungeonGenerateBase::CreateImplement_QueryAisleGeneration(const bool hasAuthority)
{
	check(IsValid(mParameter));
	MEASURE_TIME_START(stopwatch);

	// 通路生成のイベントを発生させます
	mGenerator->GetVoxel()->Each([this](const FIntVector& location, dungeon::Grid& grid)
		{
			if (grid.GetType() == dungeon::Grid::Type::Aisle)
			{
				const FVector position = mParameter->ToWorld(location) + GetActorLocation();
				const FVector gridSize = mParameter->GetGridSize().To3D();
				const FVector gridHalfSize = gridSize / 2.;
				const FVector centerPosition = position + FVector(gridHalfSize.X, gridHalfSize.Y, 0);
				const EDungeonDirection direction = static_cast<EDungeonDirection>(grid.GetDirection().Get());
				if (OnQueryAisleGeneration(centerPosition, grid.GetIdentifier(), direction))
				{
					// 各メッシュの生成禁止フラグを立てます
					grid.NoFloorMeshGeneration(true);
					grid.NoRoofMeshGeneration(true);
					//grid.NoNorthWallMeshGeneration(true);
					//grid.NoEastWallMeshGeneration(true);
					//grid.NoSouthWallMeshGeneration(true);
					//grid.NoWestWallMeshGeneration(true);
				}

				// 通路グリッドを登録
				check(mAisleGridMap);
				mAisleGridMap->Register(grid.GetIdentifier(), direction, centerPosition, grid.GetDepthRatioFromStart(), grid.GetZoneIndex());
			}
			return true;
		}
	);

	MEASURE_TIME_LAP(stopwatch, TEXT("  Query aisle generation"));
}

/*
ボクセル情報にしたがってメッシュを生成します

hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
*/
void ADungeonGenerateBase::CreateImplement_AddTerrain(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority)
{
	check(IsValid(mParameter));

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p1   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p1   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif

	mReservedWallInfo.clear();

	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		double floorAndSlopeStopwatch = 0;
		double wallStopwatch = 0;
		double pillarAndTorchStopwatch = 0;
		double doorStopwatch = 0;
		double roofStopwatch = 0;
		dungeon::Stopwatch stopwatch;
		mGenerator->GetVoxel()->Each([this, &roomSensorCache, &floorAndSlopeStopwatch, &wallStopwatch, &pillarAndTorchStopwatch, &doorStopwatch, &roofStopwatch, hasAuthority]
#else
		mGenerator->GetVoxel()->Each([this, &roomSensorCache, hasAuthority]
#endif
			(const FIntVector & location, const dungeon::Grid & grid)
			{
				ADungeonRoomSensorBase* dungeonRoomSensorBase = nullptr;
				for (const auto& roomSensor : roomSensorCache)
				{
					if (roomSensor.first->Contain(location))
					{
						dungeonRoomSensorBase = roomSensor.second;
					}
				}

				const FVector position = mParameter->ToWorld(location) + GetActorLocation();
				const FVector gridSize = mParameter->GetGridSize().To3D();
				const FVector gridHalfSize = gridSize / 2.;
				const FVector centerPosition = position + FVector(gridHalfSize.X, gridHalfSize.Y, 0);
				const CreateImplementParameter createImplementParameter =
				{
					location,
					mGenerator->GetVoxel()->Index(location),
					grid,
					position,
					gridSize,
					gridHalfSize,
					centerPosition
				};

				// Generate floor and slope meshes
				{
					MEASURE_TIME_START(stopwatch);
					CreateImplement_AddFloorAndSlope(createImplementParameter);
					MEASURE_TIME_ADD(stopwatch, floorAndSlopeStopwatch);
				}

				// Generate wall mesh
				{
					MEASURE_TIME_START(stopwatch);
					CreateImplement_ReserveWall(createImplementParameter);
					MEASURE_TIME_ADD(stopwatch, wallStopwatch);
				}

				// Generate mesh for pillars and torches
				{
					MEASURE_TIME_START(stopwatch);
					CreateImplement_AddPillarAndTorch(createImplementParameter, dungeonRoomSensorBase, hasAuthority);
					MEASURE_TIME_ADD(stopwatch, pillarAndTorchStopwatch);
				}

				// Generate door mesh
				{
					MEASURE_TIME_START(stopwatch);
					CreateImplement_AddDoor(createImplementParameter, dungeonRoomSensorBase, hasAuthority);
					MEASURE_TIME_ADD(stopwatch, doorStopwatch);
				}

				// Generate roof mesh
				{
					MEASURE_TIME_START(stopwatch);
					CreateImplement_AddRoof(createImplementParameter);
					MEASURE_TIME_ADD(stopwatch, roofStopwatch);
				}

				// Reserve Vegetation Generation Aisle Bounds
				{
					CreateImplement_ReserveVegetationGenerationAisleBounds(createImplementParameter);
				}

				return true;
			}
		);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("Spawn meshes and actors: %lf seconds"), stopwatch.Lap());
		DUNGEON_GENERATOR_LOG(TEXT(" - floor and slope meshes: %lf seconds"), floorAndSlopeStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT(" - wall meshes: %lf seconds"), wallStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT(" - pillar and torch actors: %lf seconds"), pillarAndTorchStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT(" - roof meshes: %lf seconds"), roofStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT(" - door actors: %lf seconds"), doorStopwatch);
#endif
	}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p2   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p2   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif
}

/*
床とスロープはレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void ADungeonGenerateBase::CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp) const
{
	if (mOnAddSlope && cp.mGrid.CanBuildSlope())
	{
		const uint8 neighborMask6 = MakeNeighborMask6(mGenerator->GetGrid(cp.mGridLocation));
		/*
		スロープのメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase = mParameter->GetDungeonMeshPartsDatabase(cp.mGridLocation, cp.mGrid);
		if (dungeonMeshSetDatabase)
		{
			if (const FDungeonMeshParts* parts = mParameter->SelectSlopeParts(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom(), neighborMask6))
			{
				mOnAddSlope(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
			}
		}
	}
	else if (mOnAddFloor && cp.mGrid.CanBuildFloor(true))
	{
		const uint8 neighborMask6 = MakeNeighborMask6(mGenerator->GetGrid(cp.mGridLocation));
		/*
		床のメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase = mParameter->GetDungeonMeshPartsDatabase(cp.mGridLocation, cp.mGrid);
		if (dungeonMeshSetDatabase)
		{
			if (cp.mGrid.IsCatwalk())
			{
				if (const FDungeonMeshParts* parts = mParameter->SelectCatwalkParts(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom(), neighborMask6))
				{
					mOnAddCatwalk(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetCatwalkDirection()));
				}
			}
			else
			{
				if (const FDungeonMeshParts* parts = mParameter->SelectFloorParts(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom(), neighborMask6))
				{
					mOnAddFloor(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
				}
			}
		}
	}
}

/*
壁はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void ADungeonGenerateBase::CreateImplement_ReserveWall(const CreateImplementParameter& cp)
{
	const uint8 neighborMask6 = MakeNeighborMask6(mGenerator->GetGrid(cp.mGridLocation));
	/*
	壁のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面（北側の壁）になっています。
	*/
	const FDungeonMeshSet* meshSet = nullptr;
	const FDungeonMeshParts* parts = nullptr;
	bool bSelectWallPartsByFace = false;
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase = mParameter->GetDungeonMeshPartsDatabase(cp.mGridLocation, cp.mGrid);
		if (dungeonMeshSetDatabase)
		{
			meshSet = mParameter->SelectMeshSet(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom());
			if (meshSet != nullptr)
				bSelectWallPartsByFace = IsValid(meshSet->GetWallPartsSelector()) && meshSet->GetWallPartsSelector()->IsA<UDungeonGridIndexPartsSelector>();

			// グリッドによるパーツ選択を行う場合はここで抽選する
			if (!bSelectWallPartsByFace)
				parts = mParameter->SelectWallPartsByGrid(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom(), neighborMask6);
		}
	}

	if (bSelectWallPartsByFace || parts != nullptr)
	{
		// 北側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetGrid(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z), dungeon::Direction::North, false, false))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (bSelectWallPartsByFace)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::North));
			}
			if (parts != nullptr)
			{
				FVector wallPosition = cp.mCenterPosition;
				wallPosition.Y -= cp.mGridHalfSize.Y;
				mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 0.f));

			}
		}
		// 南側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetGrid(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z), dungeon::Direction::South, false, false))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (bSelectWallPartsByFace)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::South));
			}
			if (parts != nullptr)
			{
				FVector wallPosition = cp.mCenterPosition;
				wallPosition.Y += cp.mGridHalfSize.Y;
				mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 180.f));

			}
		}
		// 東側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetGrid(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::East, false, false))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (bSelectWallPartsByFace)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::East));
			}
			if (parts != nullptr)
			{
				FVector wallPosition = cp.mCenterPosition;
				wallPosition.X += cp.mGridHalfSize.X;
				mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 90.f));

			}
		}
		// 西側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetGrid(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::West, false, false))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (bSelectWallPartsByFace)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::West));
			}
			if (parts != nullptr)
			{
				FVector wallPosition = cp.mCenterPosition;
				wallPosition.X -= cp.mGridHalfSize.X;
				mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, -90.f));

			}
		}
	}
}

/**
 * この関数はGridに対して壁フラグの設定(CreateImplement_ReserveWall)
 * が完了してから呼び出してください
 */
void ADungeonGenerateBase::CreateImplement_ReserveVegetationGenerationAisleBounds(const CreateImplementParameter& cp) const
{
}

void ADungeonGenerateBase::CreateImplement_AddWall()
{
	if (mOnAddWall)
	{
		for (const auto& reservedWallInfo : mReservedWallInfo)
		{
			mOnAddWall(reservedWallInfo.mStaticMesh, reservedWallInfo.mTransform);
		}
		mReservedWallInfo.clear();
	}
}

/*
天井はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void ADungeonGenerateBase::CreateImplement_AddRoof(const CreateImplementParameter& cp) const
{
	if (mOnAddRoof == nullptr)
		return;

	if (cp.mGrid.CanBuildRoof(mGenerator->GetGrid(cp.mGridLocation.X, cp.mGridLocation.Y, cp.mGridLocation.Z + 1), true))
	{
		const uint8 neighborMask6 = MakeNeighborMask6(mGenerator->GetGrid(cp.mGridLocation));
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase = mParameter->GetDungeonMeshPartsDatabase(cp.mGridLocation, cp.mGrid);
		if (dungeonMeshSetDatabase)
		{
			/*
			壁のメッシュを生成
			メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
			*/
			const FTransform transform(cp.mCenterPosition);
			if (const FDungeonMeshPartsWithDirection* parts = mParameter->SelectRoofParts(dungeonMeshSetDatabase, cp.mGridLocation, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom(), neighborMask6))
			{
				mOnAddRoof(
					parts->StaticMesh,
					parts->CalculateWorldTransform(GetSynchronizedRandom(), transform)
				);
			}
		}
	}
}

/*
ADungeonDoorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
/*
 * Builds a fixture-selection candidate from a voxel location.
 * ボクセル位置から Fixture 選択候補を作成します。
 */
ADungeonGenerateBase::FixtureGridCandidate ADungeonGenerateBase::MakeFixtureGridCandidate(const FIntVector& location) const
{
	const dungeon::Grid& grid = mGenerator->GetGrid(location);
	return FixtureGridCandidate(location, mGenerator->GetVoxel()->Index(location), grid);
}

/*
 * Adds a valid fixture-selection candidate, ignoring duplicates and empty space.
 * 有効な Fixture 選択候補を追加し、重複と空間グリッドは無視します。
 */
void ADungeonGenerateBase::AddFixtureGridCandidate(std::vector<FixtureGridCandidate>& candidates, const FIntVector& location) const
{
	const FixtureGridCandidate candidate = MakeFixtureGridCandidate(location);
	if (candidate.mGrid == nullptr)
		return;

	if (GetFixtureGridCandidatePriority(*candidate.mGrid) < 0)
		return;

	const auto i = std::find_if(candidates.begin(), candidates.end(), [&candidate](const FixtureGridCandidate& existing)
		{
			return existing.mGridLocation == candidate.mGridLocation;
		}
	);
	if (i == candidates.end())
	{
		candidates.emplace_back(candidate);
	}
}

/*
 * Selects the strongest fixture context from adjacent candidates.
 * 隣接候補から最も強い Fixture コンテキストを選択します。
 */
ADungeonGenerateBase::FixtureGridCandidate ADungeonGenerateBase::SelectFixtureGridCandidate(const std::vector<FixtureGridCandidate>& candidates, const FixtureGridCandidate& fallback) const
{
	FixtureGridCandidate bestCandidate = fallback;
	int32 bestPriority = (fallback.mGrid != nullptr) ? GetFixtureGridCandidatePriority(*fallback.mGrid) : MIN_int32;

	for (const FixtureGridCandidate& candidate : candidates)
	{
		if (candidate.mGrid == nullptr)
			continue;

		const int32 priority = GetFixtureGridCandidatePriority(*candidate.mGrid);
		if (priority < 0)
			continue;

		if (priority > bestPriority || (priority == bestPriority && candidate.mGridIndex < bestCandidate.mGridIndex))
		{
			bestCandidate = candidate;
			bestPriority = priority;
		}
	}

	return bestCandidate;
}

/*
 * Scores a candidate grid by fixture override strength and room/aisle ownership.
 * Fixture override の強さと部屋/通路の所属に基づいて候補グリッドを評価します。
 */
int32 ADungeonGenerateBase::GetFixtureGridCandidatePriority(const dungeon::Grid& grid) const
{
	if (grid.IsInvalidIdentifier() || grid.IsKindOfSpatialType())
		return -1;

	const bool bRoomGrid = dungeon::Identifier(grid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	if (bRoomGrid)
	{
		for (const FDungeonRoomRoleProfile& profile : mParameter->Gameplay.RoomRoles.Roles)
		{
			if (profile.Role != grid.GetRoomGameplayRole())
			{
				continue;
			}
			if (profile.ThemeOverride.bOverrideFixtures)
			{
				return 400;
			}
			break;
		}
	}

	if (mParameter->Zones.Zones.IsValidIndex(grid.GetZoneIndex()) &&
		mParameter->Zones.Zones[grid.GetZoneIndex()].ThemeOverride.bOverrideFixtures)
	{
		return 300;
	}

	if (bRoomGrid)
		return 200;

	if (grid.IsKindOfAisleType())
		return 100;

	return 50;
}

void ADungeonGenerateBase::CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const
{
	if (hasAuthority == false)
		return;

	if (CanAddDoor(dungeonRoomSensorBase, cp.mGridLocation, cp.mGrid))
	{
		const EDungeonRoomProps props = static_cast<EDungeonRoomProps>(cp.mGrid.GetProps());
		const FixtureGridCandidate fallbackCandidate(cp.mGridLocation, cp.mGridIndex, cp.mGrid);
		const auto selectDoorParts = [this, &cp, &fallbackCandidate, props](const FIntVector& neighborLocation) -> const FDungeonDoorActorParts*
			{
				std::vector<FixtureGridCandidate> candidates;
				candidates.reserve(2);
				AddFixtureGridCandidate(candidates, cp.mGridLocation);
				AddFixtureGridCandidate(candidates, neighborLocation);

				const FixtureGridCandidate selectedCandidate = SelectFixtureGridCandidate(candidates, fallbackCandidate);
				return (selectedCandidate.mGrid != nullptr) ? mParameter->SelectDoorParts(selectedCandidate.mGridLocation, selectedCandidate.mGridIndex, *selectedCandidate.mGrid, props, GetRandom()) : nullptr;
			};

		const FIntVector northLocation(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z);
		const dungeon::Grid& northGrid = mGenerator->GetGrid(northLocation);
		if (!northGrid.IsNoDoorGeneration() && cp.mGrid.CanBuildGate(northGrid, dungeon::Direction::North, false))
		{
			if (const FDungeonDoorActorParts* parts = selectDoorParts(northLocation))
			{
				// 北側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 0.f), dungeonRoomSensorBase, props);
			}
		}
		const FIntVector southLocation(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z);
		const dungeon::Grid& southGrid = mGenerator->GetGrid(southLocation);
		if (!southGrid.IsNoDoorGeneration() && cp.mGrid.CanBuildGate(southGrid, dungeon::Direction::South, false))
		{
			if (const FDungeonDoorActorParts* parts = selectDoorParts(southLocation))
			{
				// 南側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize * 0.5f;
				doorPosition.Y += mParameter->GetGridSize().HorizontalSize;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 180.f), dungeonRoomSensorBase, props);
			}
		}
		const FIntVector eastLocation(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z);
		const dungeon::Grid& eastGrid = mGenerator->GetGrid(eastLocation);
		if (!eastGrid.IsNoDoorGeneration() && cp.mGrid.CanBuildGate(eastGrid, dungeon::Direction::East, false))
		{
			if (const FDungeonDoorActorParts* parts = selectDoorParts(eastLocation))
			{
				// 東側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize;
				doorPosition.Y += mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 90.f), dungeonRoomSensorBase, props);
			}
		}
		const FIntVector westLocation(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z);
		const dungeon::Grid& westGrid = mGenerator->GetGrid(westLocation);
		if (!westGrid.IsNoDoorGeneration() && cp.mGrid.CanBuildGate(westGrid, dungeon::Direction::West, false))
		{
			if (const FDungeonDoorActorParts* parts = selectDoorParts(westLocation))
			{
				// 西側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.Y += mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, -90.f), dungeonRoomSensorBase, props);
			}
		}
	}
}

/*
ADungeonDoorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
bool ADungeonGenerateBase::CanAddDoor(const ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const
{
	if (grid.Is(dungeon::Grid::Type::Gate) == false)
		return false;

	if (grid.IsNoDoorGeneration())
		return false;

	if (dungeonRoomSensorBase == nullptr)
		return true;

	const uint8 addingProbability = dungeonRoomSensorBase->GetDoorAddingProbability();
	if (addingProbability >= 100)
		return true;

	const uint8 ratio = GetRandom()->Get<uint8>(100);
	if (ratio > addingProbability)
		return false;

	if (grid.GetDirection().IsNorthSouth())
	{
		const FIntVector e(location.X + 1, location.Y, location.Z);
		if (mGenerator->GetGrid(e).Is(dungeon::Grid::Type::Gate))
			return false;

		const FIntVector w(location.X - 1, location.Y, location.Z);
		if (mGenerator->GetGrid(w).Is(dungeon::Grid::Type::Gate))
			return false;
	}
	else
	{
		const FIntVector n(location.X, location.Y - 1, location.Z);
		if (mGenerator->GetGrid(n).Is(dungeon::Grid::Type::Gate))
			return false;

		const FIntVector s(location.X, location.Y + 1, location.Z);
		if (mGenerator->GetGrid(s).Is(dungeon::Grid::Type::Gate))
			return false;
	}

	return true;
}

void ADungeonGenerateBase::CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const
{
	const bool bRoomGrid = dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	const FDungeonFixtureSettings& fixtures = mParameter->ResolveFixtureSettings(cp.mGrid.GetRoomGameplayRole(), cp.mGrid.GetZoneIndex(), bRoomGrid);
	const EDungeonFrequencyOfGeneration initialTorchlightFrequency = fixtures.FrequencyOfTorchlightGeneration;

	struct TorchChecker final
	{
		uint16_t mIdentifier;
		uint16_t mCount;
		FIntVector mNormal;

		TorchChecker(const uint16_t identifier, const FIntVector& normal) noexcept
			: mIdentifier(identifier)
			, mCount(1)
			, mNormal(normal) { }
		bool IsValid() const noexcept
		{
			return 2 <= mCount && mCount <= 3;
		}
	};
	struct WallChecker final
	{
		dungeon::Direction::Index mDirection;

		constexpr explicit WallChecker(dungeon::Direction::Index direction)
			: mDirection(direction) { }
	};
	static constexpr std::array<WallChecker, 8> WallCheckers = { {
			// 左回り
			WallChecker(dungeon::Direction::North),
			WallChecker(dungeon::Direction::West),
			WallChecker(dungeon::Direction::South),
			WallChecker(dungeon::Direction::East),
			// 右回り
			WallChecker(dungeon::Direction::West),
			WallChecker(dungeon::Direction::North),
			WallChecker(dungeon::Direction::East),
			WallChecker(dungeon::Direction::South),
		} };

	// 燭台の方向や設定を求める
	std::vector<TorchChecker> torchCheckers;
	std::vector<FixtureGridCandidate> fixtureCandidates;
	fixtureCandidates.reserve(4);
	const FixtureGridCandidate fallbackCandidate(cp.mGridLocation, cp.mGridIndex, cp.mGrid);
	AddFixtureGridCandidate(fixtureCandidates, cp.mGridLocation);
	uint8_t validGridCount = 0;

	// 間引きパラメータ
	bool suppressGenerationThrottling;
	{
		const int32 column = cp.mGridLocation.X & 1;
		const int32 row = cp.mGridLocation.Y & 1;
		suppressGenerationThrottling = (cp.mGridLocation.Z & 1) ? row == column : row != column;
	}

	bool castTorchLightShadow = suppressGenerationThrottling;
	{
		FIntVector checkLocation = cp.mGridLocation;
		uint8_t validCastTorchLightShadowCount = 0;

		// 燭台の正面を求める
		for (const auto& wallChecker : WallCheckers)
		{
			const auto& fromGrid = mGenerator->GetGrid(checkLocation);
			const FIntVector& direction = dungeon::Direction::GetVector(wallChecker.mDirection);
			checkLocation += direction;
			const auto& toGrid = mGenerator->GetGrid(checkLocation);

			// 床が無いグリッドか？
			if (fromGrid.IsKindOfSpatialType() || fromGrid.Is(dungeon::Grid::Type::Floor) || fromGrid.Is(dungeon::Grid::Type::DownSpace))
				++validCastTorchLightShadowCount;

			// 柱必要か調べます
			if (fromGrid.CanBuildWall(toGrid, wallChecker.mDirection, false, false) == true)
			{
				++validGridCount;
				AddFixtureGridCandidate(fixtureCandidates, checkLocation - direction);

				// 燭台の生成が必要か判定します
				bool generationPermit =
					fromGrid.IsKindOfSpatialType() == false &&
					fromGrid.Is(dungeon::Grid::Type::DownSpace) == false &&
					fromGrid.Is(dungeon::Grid::Type::Slope) == false;
				// Occasionally以上は燭台を二階以上の場所に生成しない
				if (generationPermit && initialTorchlightFrequency >= EDungeonFrequencyOfGeneration::Occasionally)
				{
					generationPermit = fromGrid.Is(dungeon::Grid::Type::Floor) == false;
				}
				// 燭台の方向を計算
				if (generationPermit)
				{
					static const auto Registerer = [](std::vector<TorchChecker>& torchCheckers, const uint16_t identifier, const FIntVector& normal)
						{
							auto i = std::find_if(torchCheckers.begin(), torchCheckers.end(), [identifier](const TorchChecker& torch)
								{
									return torch.mIdentifier == identifier;
								}
							);
							if (i == torchCheckers.end())
							{
								torchCheckers.emplace_back(identifier, normal);
							}
							else
							{
								++i->mCount;
								i->mNormal += normal;
							}
						};
					Registerer(torchCheckers, fromGrid.GetIdentifier(), direction * -1);
				}
			}
		}

		// 地面が遠いなら影を落とさない
		if (validCastTorchLightShadowCount >= WallCheckers.size())
			castTorchLightShadow = false;
	}

	/*
	柱のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
	*/
	if (1 <= validGridCount)
	{
		const FTransform rootTransform(cp.mPosition);
		const FixtureGridCandidate selectedFixtureCandidate = SelectFixtureGridCandidate(fixtureCandidates, fallbackCandidate);
		const FIntVector fixtureGridLocation = (selectedFixtureCandidate.mGrid != nullptr) ? selectedFixtureCandidate.mGridLocation : cp.mGridLocation;
		const dungeon::Grid& fixtureGrid = (selectedFixtureCandidate.mGrid != nullptr) ? *selectedFixtureCandidate.mGrid : cp.mGrid;
		const size_t fixtureGridIndex = (selectedFixtureCandidate.mGrid != nullptr) ? selectedFixtureCandidate.mGridIndex : cp.mGridIndex;
		const bool bFixtureRoomGrid = dungeon::Identifier(fixtureGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
		const FDungeonFixtureSettings& selectedFixtures = mParameter->ResolveFixtureSettings(fixtureGrid.GetRoomGameplayRole(), fixtureGrid.GetZoneIndex(), bFixtureRoomGrid);
		const EDungeonFrequencyOfGeneration torchlightFrequency = selectedFixtures.FrequencyOfTorchlightGeneration;

		// 柱を生成
		if (mOnAddPillar)
		{
			if (const FDungeonMeshParts* pillarParts = mParameter->SelectPillarParts(fixtureGridLocation, fixtureGridIndex, fixtureGrid, GetSynchronizedRandom()))
			{
				mOnAddPillar(pillarParts->StaticMesh, pillarParts->CalculateWorldTransform(rootTransform));
			}
		}

		// 燭台を生成（サーバーのみアクターをスポーンする）
		bool spawnTorchActor = false;
		switch (torchlightFrequency)
		{
		case EDungeonFrequencyOfGeneration::Normally:
			spawnTorchActor = true;
			break;
		case EDungeonFrequencyOfGeneration::Sometime:
		case EDungeonFrequencyOfGeneration::Occasionally:
			spawnTorchActor = suppressGenerationThrottling;
			break;
		case EDungeonFrequencyOfGeneration::Rarely:
			if (GetRandom()->Get<bool>())
				spawnTorchActor = suppressGenerationThrottling;
			break;
		case EDungeonFrequencyOfGeneration::AlmostNever:
			if ((GetRandom()->Get<uint32_t>() & 7) == 0)
				spawnTorchActor = suppressGenerationThrottling;
			break;
		case EDungeonFrequencyOfGeneration::Never:
		default:
			break;
		}

		// 燭台をスポーン
		if (spawnTorchActor)
		{
			for (auto& torchChecker : torchCheckers)
			{
				if (!torchChecker.IsValid())
					continue;

				if (const FDungeonActorParts* torchParts = mParameter->SelectTorchParts(fixtureGridLocation, fixtureGridIndex, fixtureGrid, GetRandom()))
				{
					/*
					hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
					例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
					*/
					if (hasAuthority)
					{
						FVector normal(torchChecker.mNormal);
						normal.Normalize();
						FTransform relativeTransform(normal.Rotation());
						const FTransform worldTransform = torchParts->RelativeTransform * relativeTransform * rootTransform;
						SpawnTorchActor(
							torchParts->ActorClass,
							worldTransform,
							dungeonRoomSensorBase,
							ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
							castTorchLightShadow
						);
					}
				}

			}
		}
	}
}

/*
ADungeonRoomSensorBaseはリプリケートされない前提のアクターなので
必ず同期乱数(GetSynchronizedRandom)を使ってください。
*/
void ADungeonGenerateBase::CreateImplement_PrepareSpawnRoomSensor(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const
{
	check(IsValid(mParameter));
	MEASURE_TIME_START(stopwatch);

	mGenerator->ForEach([this, &roomSensorCache, hasAuthority](const std::shared_ptr<const dungeon::Room>& room)
		{
			const uint8 depthFromStart = room->GetDepthFromStart();
			const uint8 deepestDepthFromStart = mGenerator->GetDeepestDepthFromStart();
			const float depthFromStartRatio = deepestDepthFromStart > 0 ?
				static_cast<float>(depthFromStart) / static_cast<float>(deepestDepthFromStart) :
				0.f;
			FDungeonGeneratedRoomInfo roomInfo;
			roomInfo.RoomStructuralRole = room->GetStructuralRole();
			roomInfo.RoomGameplayRole = room->GetGameplayRole();
			roomInfo.Parts = static_cast<EDungeonRoomParts>(room->GetParts());
			roomInfo.Item = static_cast<EDungeonRoomItem>(room->GetItem());
			roomInfo.Identifier = room->GetIdentifier();
			roomInfo.bSecretRoom = roomInfo.RoomGameplayRole == EDungeonRoomGameplayRole::Secret;
			roomInfo.bDeadEndRoom = roomInfo.RoomStructuralRole == EDungeonRoomStructuralRole::DeadEnd;
			roomInfo.bMainPathRoom = room->IsMainPathRoom();
			roomInfo.bLockedRouteRoom = room->IsLockedRouteRoom();
			roomInfo.ZoneIndex = room->GetZoneIndex();
			if (mParameter->GetZoneSettings().Zones.IsValidIndex(roomInfo.ZoneIndex))
			{
				roomInfo.ZoneName = mParameter->GetZoneSettings().Zones[roomInfo.ZoneIndex].Name;
			}
			roomInfo.BranchId = room->GetBranchId();
			roomInfo.DepthFromStart = depthFromStart;
			roomInfo.DeepestDepthFromStart = deepestDepthFromStart;
			roomInfo.DepthFromStartRatio = depthFromStartRatio;

			// 乱数の同期のため、部屋ごとに生成するRoomSensorクラスを選択する
			auto* roomSensorClass = mParameter->ResolveRoomSensorClass(roomInfo.RoomGameplayRole, roomInfo.ZoneIndex);

			// サーバーならRoomSensorActorを生成
			if (hasAuthority)
			{
				if (roomSensorClass)
				{
					auto* roomSensorActor = SpawnRoomSensorActorDeferred(
						roomSensorClass,
						room->GetIdentifier(),
						room->GetCenter() * mParameter->GetGridSize().To3D() + GetActorLocation(),
						room->GetExtent() * mParameter->GetGridSize().To3D(),
						roomInfo.Parts,
						roomInfo.Item,
						roomInfo,
						room->GetBranchId(),
						depthFromStart,
						deepestDepthFromStart
					);
					roomSensorCache[room.get()] = roomSensorActor;
				}
			}
		}
	);

	MEASURE_TIME_LAP(stopwatch, TEXT("  Prepare spawn DungeonRoomSensor actors"));
}

/*
ADungeonRoomSensorBaseはリプリケートされない前提のアクターなので
必ず同期乱数(GetSynchronizedRandom)を使ってください。
*/
void ADungeonGenerateBase::CreateImplement_FinishSpawnRoomSensor(const RoomAndRoomSensorMap& roomSensorCache)
{
	MEASURE_TIME_START(stopwatch);

	for (const auto& roomSensorActor : roomSensorCache)
	{
		if (IsValid(roomSensorActor.second))
		{
			FinishRoomSensorActorSpawning(roomSensorActor.second);
		}
	}

	MEASURE_TIME_LAP(stopwatch, TEXT("  Finish spawn DungeonRoomSensor actors"));
}


void ADungeonGenerateBase::CreateImplement_AddChandelier(const RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const
{
	MEASURE_TIME_START(stopwatch);

	if (!hasAuthority)
		return;

	const std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel();
	if (voxel == nullptr)
		return;

	struct FCandidate final
	{
		FVector Location;
		float Score;
		float MinSpacing;
		float MinCeilingHeight;
		float Radius;
		size_t GridIndex;
		FIntVector GridLocation;
		dungeon::Grid Grid;
	};

	const float horizontalGridSize = mParameter->GetGridSize().HorizontalSize;
	const float verticalGridSize = mParameter->GetGridSize().VerticalSize;
	const FVector actorLocation = GetActorLocation();

	const auto spawnChandeliersInRegion =
		[this, &voxel, horizontalGridSize, verticalGridSize, actorLocation]
		(
			const FBox& worldBox,
			const FBox& gridBox,
			const int32 areaVoxel,
			ADungeonRoomSensorBase* ownerSensor,
			const std::function<bool(const FIntVector&, const dungeon::Grid&)>& isCandidateGrid,
			const bool checkOuterBoundary
			)
		{
			if (areaVoxel <= 0)
				return;

			const FVector regionCenter = worldBox.GetCenter();
			const FVector regionExtent = worldBox.GetExtent();
			const int32 targetCount = FMath::Max(1, (areaVoxel + 3) / 4);

			std::vector<FCandidate> candidates;
			voxel->Each(gridBox, [&](const FIntVector& location, const dungeon::Grid& grid)
				{
					if (!isCandidateGrid(location, grid))
						return true;

					const FVector position = mParameter->ToWorld(location) + actorLocation;
					const FVector candidate = position + FVector(horizontalGridSize * 0.5f, horizontalGridSize * 0.5f, 0.f);
					if (!worldBox.IsInsideXY(candidate))
						return true;

					const float distXToWall = FMath::Min(FMath::Abs((regionCenter.X - regionExtent.X) - candidate.X), FMath::Abs((regionCenter.X + regionExtent.X) - candidate.X));
					const float distYToWall = FMath::Min(FMath::Abs((regionCenter.Y - regionExtent.Y) - candidate.Y), FMath::Abs((regionCenter.Y + regionExtent.Y) - candidate.Y));
					const float distToWall = FMath::Min(distXToWall, distYToWall) / FMath::Max(1.f, horizontalGridSize);
					const float distToRoomCenter = FVector::Dist2D(candidate, regionCenter) / FMath::Max(1.f, horizontalGridSize);
					const float distToEntrance = distToWall;
					const float distToCombatCenter = FMath::Max(0.f, 1.f - distToRoomCenter / 8.f);

					const UDungeonMeshSetDatabase* dungeonMeshSetDatabase = mParameter->GetDungeonMeshPartsDatabase(location, grid);
					const FDungeonMeshSet* meshSet = mParameter->SelectMeshSet(dungeonMeshSetDatabase, location, voxel->Index(location), grid, GetRandom());
					if (meshSet == nullptr)
						return true;

					const float score =
						distToWall * meshSet->GetChandelierWallWeight() +
						distToEntrance * 0.3f -
						distToRoomCenter * 0.2f +
						distToCombatCenter * meshSet->GetChandelierCombatWeight();

					candidates.push_back({
						candidate,
						score,
						FMath::Max(1.f, meshSet->GetChandelierMinSpacing()),
						FMath::Max(1.f, meshSet->GetChandelierMinCeilingHeight()),
						FMath::Max(1.f, meshSet->GetChandelierRadius()),
						voxel->Index(location),
						location,
						grid
					});
					return true;
				}
			);

			std::vector<FCandidate> selected;
			selected.reserve(static_cast<size_t>(targetCount));

			std::vector<bool> used(candidates.size(), false);
			const float maxRegionRadius = FMath::Max(1.f, FMath::Max(regionExtent.X, regionExtent.Y));

			for (int32 pickCount = 0; pickCount < targetCount; ++pickCount)
			{
				int32 bestIndex = INDEX_NONE;
				float bestScore = -std::numeric_limits<float>::max();

				for (int32 i = 0; i < static_cast<int32>(candidates.size()); ++i)
				{
					constexpr float spreadWeight = 1.25f;
					constexpr float centerWeight = 0.35f;

					if (used[static_cast<size_t>(i)])
						continue;

					const FCandidate& candidate = candidates[static_cast<size_t>(i)];
					float minDistToSelected = std::numeric_limits<float>::max();
					for (const auto& picked : selected)
					{
						minDistToSelected = FMath::Min(minDistToSelected, FVector::Dist2D(candidate.Location, picked.Location));
					}

					if (minDistToSelected < candidate.MinSpacing)
						continue;

					const float spreadScore = selected.empty() ? 0.f : FMath::Clamp(minDistToSelected / maxRegionRadius, 0.f, 2.f);
					const float centerCoverageScore = FMath::Clamp(1.f - FVector::Dist2D(candidate.Location, regionCenter) / maxRegionRadius, 0.f, 1.f);
					const float randomJitter = GetRandom()->Get<float>(0.05f);

					const float combinedScore =
						candidate.Score +
						spreadScore * spreadWeight +
						centerCoverageScore * centerWeight +
						randomJitter;

					if (combinedScore > bestScore)
					{
						bestScore = combinedScore;
						bestIndex = i;
					}
				}

				if (bestIndex == INDEX_NONE)
					break;

				used[static_cast<size_t>(bestIndex)] = true;
				selected.push_back(candidates[static_cast<size_t>(bestIndex)]);
			}
			for (const auto& picked : selected)
			{
				if (checkOuterBoundary && FVector::Dist2D(picked.Location, regionCenter) > FMath::Max(regionExtent.X, regionExtent.Y) * 0.95f)
					continue;

				FHitResult ceilingHit;
				FCollisionQueryParams queryParams(SCENE_QUERY_STAT(DungeonChandelierCeilingTrace), false);
				const auto traceStart = picked.Location + FVector(0, 0, verticalGridSize * 0.5);
				const auto traceEnd = traceStart + FVector(0, 0, verticalGridSize * 4);
				if (!GetWorld()->LineTraceSingleByChannel(ceilingHit, traceStart, traceEnd, ECC_WorldStatic, queryParams))
					continue;

				const auto ceilingHeight = ceilingHit.Location.Z - picked.Location.Z;
				if (ceilingHeight < picked.MinCeilingHeight)
					continue;

				const auto overlapCenter = picked.Location + FVector(0, 0, ceilingHeight * 0.5);
				if (GetWorld()->OverlapBlockingTestByChannel(overlapCenter, FQuat::Identity, ECC_WorldStatic, FCollisionShape::MakeSphere(picked.Radius), queryParams))
					continue;

				if (const auto* parts = mParameter->SelectChandelierParts(mParameter->GetDungeonMeshPartsDatabase(picked.GridLocation, picked.Grid), picked.GridLocation, picked.GridIndex, picked.Grid, GetRandom(), 0))
				{
					const FRotator yawOnlyRotation(0.f, GetRandom()->Get<float>(360.f), 0.f);
					const FTransform rootTransform(yawOnlyRotation, ceilingHit.Location);
					const FTransform worldTransform = parts->RelativeTransform * rootTransform;
					SpawnChandelierActor(
						parts->ActorClass,
						worldTransform,
						ownerSensor,
						ESpawnActorCollisionHandlingMethod::AlwaysSpawn
					);
				}
			}
		};

	for (const auto& roomSensorPair : roomSensorCache)
	{
		const dungeon::Room* room = roomSensorPair.first;
		auto* roomSensor = roomSensorPair.second;
		if (room == nullptr || !IsValid(roomSensor))
			continue;

		const FBox roomBox = roomSensor->GetRoomSize();
		const FBox roomGridBox(room->GetMin(), room->GetMax());
		const int32 roomAreaVoxel = FMath::Max(1, room->GetWidth() * room->GetDepth());

		spawnChandeliersInRegion(
			roomBox,
			roomGridBox,
			roomAreaVoxel,
			roomSensor,
			[room](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (!room->Contain(location))
					return false;
				if (grid.IsKindOfSpatialType())
					return false;
				return grid.Is(dungeon::Grid::Type::Floor);
			},
			true
		);
	}

	struct FAisleRegion final
	{
		FBox GridBox;
		int32 AreaVoxel;
	};

	const size_t voxelCount = static_cast<size_t>(voxel->GetWidth()) * voxel->GetDepth() * voxel->GetHeight();
	std::vector<int32> aisleRegionByGridIndex(voxelCount, -1);
	std::vector<FAisleRegion> aisleRegions;
	int32 aisleRegionId = 0;

	voxel->Each([&](const FIntVector& location, const dungeon::Grid& grid)
		{
			if (!grid.IsKindOfAisleType())
				return true;

			const size_t startIndex = voxel->Index(location);
			if (aisleRegionByGridIndex[startIndex] >= 0)
				return true;

			std::queue<FIntVector> queue;
			queue.push(location);
			aisleRegionByGridIndex[startIndex] = aisleRegionId;

			int32 minX = location.X;
			int32 minY = location.Y;
			int32 minZ = location.Z;
			int32 maxX = location.X + 1;
			int32 maxY = location.Y + 1;
			int32 maxZ = location.Z + 1;
			int32 areaVoxel = 0;

			while (!queue.empty())
			{
				const FIntVector current = queue.front();
				queue.pop();
				++areaVoxel;

				minX = FMath::Min(minX, current.X);
				minY = FMath::Min(minY, current.Y);
				minZ = FMath::Min(minZ, current.Z);
				maxX = FMath::Max(maxX, current.X + 1);
				maxY = FMath::Max(maxY, current.Y + 1);
				maxZ = FMath::Max(maxZ, current.Z + 1);

				static const FIntVector NeighborOffsets[] =
				{
					FIntVector(1, 0, 0),
					FIntVector(-1, 0, 0),
					FIntVector(0, 1, 0),
					FIntVector(0, -1, 0),
				};

				for (const FIntVector& offset : NeighborOffsets)
				{
					const FIntVector neighbor = current + offset;
					if (!voxel->Contain(neighbor))
						continue;

					const size_t neighborIndex = voxel->Index(neighbor);
					if (aisleRegionByGridIndex[neighborIndex] >= 0)
						continue;

					if (!voxel->Get(neighbor).IsKindOfAisleType())
						continue;

					aisleRegionByGridIndex[neighborIndex] = aisleRegionId;
					queue.push(neighbor);
				}
			}

			aisleRegions.push_back(
				{
					FBox(
						FVector(static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(minZ)),
						FVector(static_cast<float>(maxX), static_cast<float>(maxY), static_cast<float>(maxZ))
					),
					areaVoxel
				}
			);

			++aisleRegionId;
			return true;
		}
	);

	for (int32 currentRegionId = 0; currentRegionId < static_cast<int32>(aisleRegions.size()); ++currentRegionId)
	{
		const FAisleRegion& region = aisleRegions[currentRegionId];

		const FIntVector minGrid(
			FMath::FloorToInt(region.GridBox.Min.X),
			FMath::FloorToInt(region.GridBox.Min.Y),
			FMath::FloorToInt(region.GridBox.Min.Z)
		);
		const FIntVector maxGrid(
			FMath::CeilToInt(region.GridBox.Max.X),
			FMath::CeilToInt(region.GridBox.Max.Y),
			FMath::CeilToInt(region.GridBox.Max.Z)
		);

		const FBox worldBox(
			mParameter->ToWorld(minGrid) + actorLocation,
			mParameter->ToWorld(maxGrid) + actorLocation
		);

		spawnChandeliersInRegion(
			worldBox,
			region.GridBox,
			region.AreaVoxel,
			nullptr,
			[&voxel, &aisleRegionByGridIndex, currentRegionId](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (!grid.IsKindOfAisleType())
					return false;

				const size_t index = voxel->Index(location);
				return aisleRegionByGridIndex[index] == currentRegionId;
			},
			false
		);
	}

	MEASURE_TIME_LAP(stopwatch, TEXT("  CreateImplement_AddChandelier Time"));
}

void ADungeonGenerateBase::CreateImplement_Navigation(const bool hasAuthority)
{
	MEASURE_TIME_START(stopwatch);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p3   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p3   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
	}
#endif

	// RecastNavMeshを調べる
	CheckRecastNavMesh();
	MEASURE_TIME_LAP(stopwatch, TEXT("  CheckRecastNavMesh Time"));

	// NavMeshBoundsVolumeをフィットさせる
	FitNavMeshBoundsVolume();
	MEASURE_TIME_LAP(stopwatch, TEXT("  FitNavMeshBoundsVolume Time"));
}

/*
RecastNavMeshアクターを検索して、無ければRecastNavMeshアクターをスポーンします
*/
void ADungeonGenerateBase::CheckRecastNavMesh() const
{
	if (const auto* recastNavMesh = FindActor<ARecastNavMesh>())
	{
		if (recastNavMesh->GetRuntimeGenerationMode() != ERuntimeGenerationType::Dynamic)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Set RuntimeGenerationMode of RecastNavMesh to Dynamic"));
		}
	}
}

void ADungeonGenerateBase::FitNavMeshBoundsVolume()
{
	if (ANavMeshBoundsVolume* navMeshBoundsVolume = FindActor<ANavMeshBoundsVolume>())
	{
		const FBox& bounding = CalculateBoundingBox();
		const FVector& boundingCenter = bounding.GetCenter();
		const FVector& boundingExtent = bounding.GetExtent();

		if (USceneComponent* rootComponent = navMeshBoundsVolume->GetRootComponent())
		{
			const EComponentMobility::Type mobility = rootComponent->Mobility;
			rootComponent->SetMobility(EComponentMobility::Movable);

			navMeshBoundsVolume->SetActorLocation(boundingCenter);
			navMeshBoundsVolume->SetActorScale3D(FVector::OneVector);

#if WITH_EDITOR
			// ブラシビルダーを生成 (UnrealEd)
			if (UCubeBuilder* cubeBuilder = NewObject<UCubeBuilder>(this))
			{
				cubeBuilder->X = boundingExtent.X * 2.0;
				cubeBuilder->Y = boundingExtent.Y * 2.0;
				cubeBuilder->Z = boundingExtent.Z * 2.0;

				// ブラシ生成開始
				navMeshBoundsVolume->PreEditChange(nullptr);

				const EObjectFlags objectFlags = navMeshBoundsVolume->GetFlags() & (RF_Transient | RF_Transactional);
				navMeshBoundsVolume->Brush = NewObject<UModel>(navMeshBoundsVolume, NAME_None, objectFlags);
				navMeshBoundsVolume->Brush->Initialize(nullptr, true);
				navMeshBoundsVolume->Brush->Polys = NewObject<UPolys>(navMeshBoundsVolume->Brush, NAME_None, objectFlags);
				navMeshBoundsVolume->GetBrushComponent()->Brush = navMeshBoundsVolume->Brush;
				navMeshBoundsVolume->BrushBuilder = DuplicateObject<UBrushBuilder>(cubeBuilder, navMeshBoundsVolume);

				// ブラシビルダーを使ってブラシを生成
				cubeBuilder->Build(navMeshBoundsVolume->GetWorld(), navMeshBoundsVolume);

				// ブラシ生成終了
				navMeshBoundsVolume->PostEditChange();

				// 登録
				navMeshBoundsVolume->PostRegisterAllComponents();
			}
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("CubeBuilder generation failed in ADungeonGenerateBase"));
			}
#else
			/*
			UCubeBuilderはエディタでのみ使用可能なので
			スケールによるサイズの変更をスケールで代用します。
			*/
			const FBoxSphereBounds boxSphereBounds = navMeshBoundsVolume->GetBounds();
			const FVector boundingScale = boundingExtent / boxSphereBounds.BoxExtent;
			navMeshBoundsVolume->SetActorScale3D(boundingScale);
#endif

			rootComponent->SetMobility(mobility);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Set the RootComponent of the NavMeshBoundsVolume"));
		}
	}
}

void ADungeonGenerateBase::CollectPlayerStartExceptPlayerStartPIE(TArray<APlayerStart*>& startPoints)
{
	EachActors<APlayerStart>([&startPoints](APlayerStart* playerStart)
		{
			// "Play from Here" PlayerStart, if we find one while in PIE mode
			if (Cast<APlayerStartPIE>(playerStart))
				return true;

			if (nullptr != playerStart->GetRootComponent())
				startPoints.Add(playerStart);

			return true;
		}
	);
}

/*
プレイヤーの位置はAGameModeBase::ChoosePlayerStart_Implementation
内で選択されているので、これよりも前に設定する必要があります。

AGameModeBase::ChoosePlayerStart_Implementation
AGameModeBase::FindPlayerStart_Implementation
*/
void ADungeonGenerateBase::MovePlayerStart(const TArray<APlayerStart*>& startPoints)
{
	if (startPoints.Num() <= 0)
	{
		// APlayerStartが無いなら、サブレベル内のAPlayerStartの姿勢をAPlayerStartPIEに設定する
		TArray<APlayerStart*> startPointsInSubLevels;
		EachActors<APlayerStart>([&startPointsInSubLevels](APlayerStart* playerStart)
			{
				if (Cast<APlayerStartPIE>(playerStart))
					return true;

				if (nullptr != playerStart->GetRootComponent())
					startPointsInSubLevels.Add(playerStart);

				return true;
			}
		);
		if (startPointsInSubLevels.Num() > 0)
		{
			EachActors<APlayerStartPIE>([&startPointsInSubLevels](APlayerStartPIE* playerStartPIE)
				{
					APlayerStart* playerStart = startPointsInSubLevels[FMath::RandHelper(startPointsInSubLevels.Num())];
					if (IsValid(playerStart))
					{
						const auto& transform = playerStart->GetActorTransform();
						playerStartPIE->SetActorTransform(transform);
					}
					return true;
				}
			);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Unable to determine the starting position. Please place a PlayerStart."));
		}
		return;
	}

	// APlayerStartPIEの位置を調整
	// TODO: メニュー内の「ここから開始」で問題が起きるかもしれません
	EachActors<APlayerStartPIE>([&startPoints](APlayerStartPIE* playerStartPIE)
		{
			APlayerStart* playerStart = startPoints[FMath::RandHelper(startPoints.Num())];
			if (IsValid(playerStart))
			{
				const auto& transform = playerStart->GetActorTransform();
				playerStartPIE->SetActorTransform(transform);
			}
			return true;
		}
	);

	if (IsValid(mParameter) == false)
		return;

	// PlayerStartをスタート位置へ移動しないならここで終了します
	if (mParameter->IsMovePlayerStartToStartingPoint() == false)
		return;

	// Calculate the position to shift APlayerStart
	const double halfHorizontalSize = mParameter->GetGridSize().HorizontalSize / 2;
	const double halfVerticalSize = mParameter->GetGridSize().VerticalSize / 2;
	TArray<FBox> startRoomBoundingBoxes;
	if (mParameter->GetPathSettings().StartRoomPolicy == EDungeonStartLocationPolicy::UseMultiStart)
	{
		if (mGenerator)
		{
			mGenerator->ForEach([this, &startRoomBoundingBoxes](const std::shared_ptr<const dungeon::Room>& room)
				{
					if (room->GetParts() != dungeon::Room::Parts::Start)
						return;

					FBox boundingBox(
						room->GetMin() * mParameter->GetGridSize().To3D(),
						room->GetMax() * mParameter->GetGridSize().To3D()
					);
					startRoomBoundingBoxes.Add(boundingBox.ShiftBy(GetActorLocation()));
				}
			);
		}

		if (startRoomBoundingBoxes.IsEmpty())
		{
			startRoomBoundingBoxes.Add(GetStartBoundingBox());
		}
	}
	else
	{
		startRoomBoundingBoxes.Add(GetStartBoundingBox());
	}

	if (mParameter->GetPathSettings().StartRoomPolicy == EDungeonStartLocationPolicy::UseMultiStart &&
		startRoomBoundingBoxes.Num() != startPoints.Num())
	{
		DUNGEON_GENERATOR_WARNING(TEXT("Start room count (%d) does not match PlayerStart count (%d)."),
			startRoomBoundingBoxes.Num(),
			startPoints.Num());
	}

	auto movePlayerStartToRoom = [&](APlayerStart* playerStart, const FBox& roomBoundingBox)
	{
		const auto& startRoomCenterLocation = roomBoundingBox.GetCenter();

		// APlayerStart cannot use GetSimpleCollisionCylinder because collision is disabled.
		if (USceneComponent* rootComponent = playerStart->GetRootComponent())
		{
			// Create a small margin to avoid grounding.
			static constexpr float heightMargin = 10.f;

			// Temporarily set EComponentMobility to Movable
			const EComponentMobility::Type mobility = rootComponent->Mobility;
			rootComponent->SetMobility(EComponentMobility::Movable);

			float cylinderRadius, cylinderHalfHeight;
			rootComponent->CalcBoundingCylinder(cylinderRadius, cylinderHalfHeight);

			FHitResult hitResult;
			do {
				const FVector location(
					FMath::RandRange(roomBoundingBox.Min.X + halfHorizontalSize, roomBoundingBox.Max.X - halfHorizontalSize),
					FMath::RandRange(roomBoundingBox.Min.Y + halfHorizontalSize, roomBoundingBox.Max.Y - halfHorizontalSize),
					roomBoundingBox.Min.Z
				);

				const FVector startLocation = location + FVector(0, 0, halfVerticalSize);
				const FVector endLocation = location - FVector(0, 0, halfVerticalSize);
				if (playerStart->GetWorld()->LineTraceSingleByChannel(hitResult, startLocation, endLocation, ECollisionChannel::ECC_Pawn))
				{
					hitResult.ImpactPoint.Z += cylinderHalfHeight + heightMargin;

					// 部屋の中心を向く
					auto rotator = playerStart->GetActorRotation();
					rotator.Yaw = dungeon::math::ToDegree(
						std::atan2(
							startRoomCenterLocation.Y - location.Y,
							startRoomCenterLocation.X - location.X
						)
					);

					// 位置を設定
					playerStart->SetActorTransform(FTransform(rotator, hitResult.ImpactPoint));
				}
			} while (hitResult.bBlockingHit == false);

			// Undo EComponentMobility
			rootComponent->SetMobility(mobility);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("PlayerStart's RootComponent was not set and could not be moved (%s)"), *playerStart->GetName());
		}
	};

	for (int32 index = 0; index < startPoints.Num(); ++index)
	{
		APlayerStart* playerStart = startPoints[index];
		const int32 boundingBoxIndex = startRoomBoundingBoxes.Num() == 0
			? 0
			: (index % startRoomBoundingBoxes.Num());
		movePlayerStartToRoom(playerStart, startRoomBoundingBoxes[boundingBoxIndex]);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*
StaticMeshActorを使って地形をスポーンします。
生成したアクターにDungeonComponentActivatorComponentを追加して処理負荷制御を行います。
CRC32の計算を行うのでサーバーとクライアントの同期ずれを検出する事ができます。
*/
AStaticMeshActor* ADungeonGenerateBase::SpawnStaticMeshActor(UStaticMesh* staticMesh, const FString& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod, const EStaticMeshPartitionRegistrationFace registrationFace) const
{
	AStaticMeshActor* actor = SpawnActorDeferredImpl<AStaticMeshActor>(folderPath, transform, nullptr, spawnActorCollisionHandlingMethod);
	if (IsValid(actor) == false)
		return nullptr;

	UStaticMeshComponent* staticMeshComponent = GetValid(actor->GetStaticMeshComponent());
	const auto updateFixedPartitionRegistrationLocation = [registrationFace, staticMeshComponent](UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
	{
		if (!IsValid(dungeonComponentActivatorComponent))
			return;
		if (registrationFace == EStaticMeshPartitionRegistrationFace::None || staticMeshComponent == nullptr)
			return;

		FVector localBoundsMin = FVector::ZeroVector;
		FVector localBoundsMax = FVector::ZeroVector;
		staticMeshComponent->GetLocalBounds(localBoundsMin, localBoundsMax);

		constexpr float PartitionRegistrationInsetCm = 5.0f;
		FVector localRegistrationLocation = (localBoundsMin + localBoundsMax) * 0.5f;
		switch (registrationFace)
		{
		case EStaticMeshPartitionRegistrationFace::PositiveY:
			localRegistrationLocation.Y = FMath::Clamp(localBoundsMax.Y - PartitionRegistrationInsetCm, localBoundsMin.Y, localBoundsMax.Y);
			break;
		case EStaticMeshPartitionRegistrationFace::PositiveZ:
			localRegistrationLocation.Z = FMath::Clamp(localBoundsMax.Z - PartitionRegistrationInsetCm, localBoundsMin.Z, localBoundsMax.Z);
			break;
		case EStaticMeshPartitionRegistrationFace::NegativeZ:
			localRegistrationLocation.Z = FMath::Clamp(localBoundsMin.Z + PartitionRegistrationInsetCm, localBoundsMin.Z, localBoundsMax.Z);
			break;
		case EStaticMeshPartitionRegistrationFace::None:
		default:
			break;
		}

		const FVector registrationWorldLocation = staticMeshComponent->GetComponentTransform().TransformPosition(localRegistrationLocation);
		dungeonComponentActivatorComponent->SetFixedPartitionRegistrationWorldLocation(registrationWorldLocation);
	};
	if (staticMeshComponent != nullptr)
	{
		if (staticMeshComponent->Mobility != EComponentMobility::Movable)
			staticMeshComponent->SetMobility(EComponentMobility::Movable);

		staticMeshComponent->SetStaticMesh(staticMesh);
		staticMeshComponent->ComponentTags.AddUnique(GetDungeonGeneratorTerrainTag());
	}

	// 負荷制御コンポーネントを追加する
	if (auto* dungeonComponentActivatorComponent = FindOrAddComponentActivatorComponent(actor))
	{
		dungeonComponentActivatorComponent->SetEnableCollisionEnableControl(false);
		updateFixedPartitionRegistrationLocation(dungeonComponentActivatorComponent);
	}

	actor->FinishSpawning(transform, true);

	if (auto* dungeonComponentActivatorComponent = actor->FindComponentByClass<UDungeonComponentActivatorComponent>())
		updateFixedPartitionRegistrationLocation(dungeonComponentActivatorComponent);

	// CRC32を記録（必ずサーバーとクライアント両方で計算しないとCRC32が一致しなくなる）
	mCrc32AtCreation = ADungeonVerifiableActor::GenerateCrc32(transform, mCrc32AtCreation);

	return actor;
}

/*
DungeonDoorBaseをスポーンします。
*/
ADungeonDoorBase* ADungeonGenerateBase::SpawnDoorActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, EDungeonRoomProps props) const
{
	ADungeonDoorBase* actor = SpawnActorDeferredImpl<ADungeonDoorBase>(actorClass, DoorsFolderPath, transform, ownerActor, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (IsValid(actor))
	{
		actor->InvokeInitialize(GetRandom(), props);
		actor->FinishSpawning(transform, true);

		// 負荷制御コンポーネントを追加する
		FindOrAddComponentActivatorComponent(actor);

		// ドアアクターを登録する
		if (IsValid(ownerActor))
			ownerActor->AddDungeonDoor(actor);
	}
	return actor;
}

/*
燭台アクターをスポーンします。
*/
AActor* ADungeonGenerateBase::SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod, const bool castShadow) const
{
	ADungeonGenerateBase* nonConstThis = const_cast<ADungeonGenerateBase*>(this);
	if (!IsValid(nonConstThis))
		return nullptr;

	TWeakObjectPtr<ADungeonRoomSensorBase> weakOwnerActor(ownerActor);
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = nullptr;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	nonConstThis->DeferredSpawnActorWithFolderPath(
		actorClass,
		TorchesFolderPath,
		transform,
		actorSpawnParameters,
		[weakOwnerActor, castShadow](AActor* actor)
		{
			if (IsValid(actor) == false)
				return;

			// 雋闕ｷ蛻ｶ蠕｡繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ霑ｽ蜉縺吶ｋ
			ADungeonGenerateBase::FindOrAddComponentActivatorComponent(actor);

			// 繝昴う繝ｳ繝医Λ繧､繝医∪縺溘・繧ｹ繝昴ャ繝医Λ繧､繝医・CastShadow繧貞宛蠕｡縺吶ｋ
			if (castShadow == false)
			{
				for (auto* component : actor->GetComponents())
				{
					if (auto* pointLightComponent = Cast<UPointLightComponent>(component))
						pointLightComponent->SetCastShadows(false);
				}
			}

			// 隕ｪ繧｢繧ｯ繧ｿ繝ｼ縺ｫ辯ｭ蜿ｰ繧｢繧ｯ繧ｿ繝ｼ繧堤匳骭ｲ縺吶ｋ
			if (ADungeonRoomSensorBase* validOwnerActor = weakOwnerActor.Get())
			{
				if (actor->GetOwner() != validOwnerActor)
				{
					actor->SetOwner(validOwnerActor);
				}
				validOwnerActor->AddDungeonTorch(actor);
			}
		}
	);

	return nullptr;
}
AActor* ADungeonGenerateBase::SpawnChandelierActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	ADungeonGenerateBase* nonConstThis = const_cast<ADungeonGenerateBase*>(this);
	if (!IsValid(nonConstThis))
		return nullptr;

	TWeakObjectPtr<ADungeonRoomSensorBase> weakOwnerActor(ownerActor);
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = nullptr;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	nonConstThis->DeferredSpawnActorWithFolderPath(
		actorClass,
		ChandeliersFolderPath,
		transform,
		actorSpawnParameters,
		[weakOwnerActor](AActor* actor)
		{
			if (IsValid(actor) == false)
				return;

			ADungeonGenerateBase::FindOrAddComponentActivatorComponent(actor);
			if (ADungeonRoomSensorBase* validOwnerActor = weakOwnerActor.Get())
			{
				if (actor->GetOwner() != validOwnerActor)
				{
					actor->SetOwner(validOwnerActor);
				}
				validOwnerActor->AddDungeonChandelier(actor);
			}
		}
	);
	return nullptr;
}

/*
DungeonRoomSensorBaseをスポーンします

ADungeonRoomSensorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
ADungeonRoomSensorBase* ADungeonGenerateBase::SpawnRoomSensorActorDeferred(UClass* actorClass, const dungeon::Identifier& identifier, const FVector& center, const FVector& extents, EDungeonRoomParts parts, EDungeonRoomItem item, const FDungeonGeneratedRoomInfo& roomInfo, uint8 branchId, const uint8 depthFromStart, const uint8 deepestDepthFromStart) const
{
	const FTransform transform(center);
	ADungeonRoomSensorBase* actor = SpawnActorDeferredImpl<ADungeonRoomSensorBase>(actorClass, SensorsFolderPath, transform, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		const bool success = actor->InvokePrepare(
			GetRandom(),
			identifier,
			center,
			extents,
			mParameter->GetGridSize().HorizontalSize,
			parts,
			item,
			roomInfo,
			branchId,
			depthFromStart,
			deepestDepthFromStart);
		if (success == false)
		{
			actor->Destroy();
			actor = nullptr;
		}
	}
	return actor;
};

/*
ADungeonRoomSensorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
void ADungeonGenerateBase::FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor)
{
	if (IsValid(dungeonRoomSensor))
	{
		dungeonRoomSensor->InvokeInitialize();
		dungeonRoomSensor->FinishSpawning(FTransform::Identity, true);
	}
};

FVector ADungeonGenerateBase::GetStartLocation() const
{
	if (IsValid(mParameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		FVector location = *mGenerator->GetStartPoint() * mParameter->GetGridSize().To3D();
		location += GetActorLocation();
		return location;
	}
	return FVector::ZeroVector;
}

FBox ADungeonGenerateBase::GetStartBoundingBox() const
{
	if (IsValid(mParameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		if (const auto& startRoom = mGenerator->GetStartPoint()->GetOwnerRoom())
		{
			FBox boundingBox(
				startRoom->GetMin() * mParameter->GetGridSize().To3D(),
				startRoom->GetMax() * mParameter->GetGridSize().To3D()
			);
			return boundingBox.ShiftBy(GetActorLocation());
		}
	}
	return FBox();
}


bool ADungeonGenerateBase::ShiftGeneratedDungeonWorldOffset(const FVector& delta)
{
	if (delta.IsNearlyZero())
		return true;

	const auto shiftFixedPartitionRegistrationLocation = [&delta](AActor* actor)
	{
		if (!IsValid(actor))
			return;

		TInlineComponentArray<UDungeonComponentActivatorComponent*> activatorComponents(actor);
		actor->GetComponents<UDungeonComponentActivatorComponent>(activatorComponents);
		for (UDungeonComponentActivatorComponent* activatorComponent : activatorComponents)
		{
			if (IsValid(activatorComponent))
				activatorComponent->ShiftFixedPartitionRegistrationWorldLocation(delta);
		}
	};

	if (auto* rootComponent = GetRootComponent())
	{
		if (rootComponent->Mobility != EComponentMobility::Movable)
			rootComponent->SetMobility(EComponentMobility::Movable);
	}
	SetActorLocation(GetActorLocation() + delta);
	shiftFixedPartitionRegistrationLocation(this);

	// ISM/HISM are component-managed terrain and may not move with owner actor translation.
	TInlineComponentArray<UInstancedStaticMeshComponent*> instancedMeshComponents(this);
	GetComponents(instancedMeshComponents);
	for (auto* instancedMeshComponent : instancedMeshComponents)
	{
		if (!IsValid(instancedMeshComponent))
			continue;

		if (instancedMeshComponent->GetAttachParent() != nullptr &&
			instancedMeshComponent->GetAttachmentRootActor() == this)
		{
			continue;
		}

		if (instancedMeshComponent->Mobility != EComponentMobility::Movable)
			instancedMeshComponent->SetMobility(EComponentMobility::Movable);

		const int32 instanceCount = instancedMeshComponent->GetInstanceCount();
		for (int32 index = 0; index < instanceCount; ++index)
		{
			FTransform instanceTransform;
			if (!instancedMeshComponent->GetInstanceTransform(index, instanceTransform, true))
				continue;

			instanceTransform.AddToTranslation(delta);
			const bool markRenderStateDirty = (index + 1) == instanceCount;
			instancedMeshComponent->UpdateInstanceTransform(index, instanceTransform, true, markRenderStateDirty, true);
		}

		if (auto* hierarchicalComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(instancedMeshComponent))
		{
			hierarchicalComponent->BuildTreeIfOutdated(true, false);
		}
	}

	TArray<AActor*> generatedActors;
	UGameplayStatics::GetAllActorsWithTag(this, GetDungeonGeneratorTag(), generatedActors);
	for (AActor* actor : generatedActors)
	{
		if (!IsValid(actor))
			continue;
		if (actor == this)
			continue;
		if (actor->GetAttachParentActor() == this)
			continue;

		if (USceneComponent* rootComponent = actor->GetRootComponent())
		{
			if (rootComponent->Mobility != EComponentMobility::Movable)
				rootComponent->SetMobility(EComponentMobility::Movable);
		}
		actor->SetActorLocation(actor->GetActorLocation() + delta);
		shiftFixedPartitionRegistrationLocation(actor);
	}

	// Shift pending deferred-spawn transforms so queued actors follow world-offset alignment.
	mDungeonDeferredActorSpawnManager.ShiftQueuedRequests(delta);


	return true;
}

bool ADungeonGenerateBase::AlignGeneratedDungeonStartRoomBoundsMinToWorldLocation(const FVector& targetLocation, const bool alignXYOnly)
{
	const auto startBoundingBox = GetStartBoundingBox();
	if (startBoundingBox.IsValid == false)
		return false;

	auto delta = targetLocation - startBoundingBox.Min;
	if (alignXYOnly)
	{
		delta.Z = 0.f;
	}
	return ShiftGeneratedDungeonWorldOffset(delta);
}


FVector ADungeonGenerateBase::GetGoalLocation() const
{
	if (IsValid(mParameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		FVector location = *mGenerator->GetGoalPoint() * mParameter->GetGridSize().To3D();
		location += GetActorLocation();
		return location;
	}
	return FVector::ZeroVector;
}

FBox ADungeonGenerateBase::CalculateBoundingBox() const
{
	if (mGenerator && IsValid(mParameter))
	{
		FBox boundingBox(EForceInit::ForceInitToZero);
		mGenerator->ForEach([this, &boundingBox](const std::shared_ptr<const dungeon::Room>& room)
			{
				/*
				2D空間は（X軸:前 Y軸:右）
				3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
				*/
				const FVector min = mParameter->ToWorld(room->GetLeft(), room->GetTop(), room->GetBackground());
				const FVector max = mParameter->ToWorld(room->GetRight(), room->GetBottom(), room->GetForeground());
				boundingBox += FBox(min, max);
			}
		);
		boundingBox = boundingBox.ShiftBy(GetActorLocation());
		boundingBox.Min.Z -= mParameter->GetGridSize().VerticalSize;
		boundingBox.Max.Z += mParameter->GetGridSize().VerticalSize;
		return boundingBox;
	}

	return FBox(EForceInit::ForceInitToZero);
}

FVector2D ADungeonGenerateBase::GetLongestStraightPath() const noexcept
{
	if (mGenerator && IsValid(mParameter))
	{
		if (std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel())
		{
			const auto& longestStraightPath = voxel->GetLongestStraightPath();
			return FVector2D(
				longestStraightPath.X * mParameter->GetGridSize().HorizontalSize,
				longestStraightPath.Y * mParameter->GetGridSize().HorizontalSize
			);
		}
	}
	return FVector2D(ForceInitToZero);
}

FDungeonLayoutMetrics ADungeonGenerateBase::GetLastLayoutMetrics() const noexcept
{
	return mGenerator ? mGenerator->GetLastLayoutMetrics() : FDungeonLayoutMetrics();
}

FDungeonLayoutScore ADungeonGenerateBase::GetLastLayoutScore() const noexcept
{
	return mGenerator ? mGenerator->GetLastLayoutScore() : FDungeonLayoutScore();
}

////////////////////////////////////////////////////////////////////////////////

uint32_t ADungeonGenerateBase::CalculateCRC32() const noexcept
{
	uint32_t crc32 = mCrc32AtCreation;
	if (mGenerator)
		crc32 = mGenerator->CalculateCRC32(crc32);
	return crc32;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
void ADungeonGenerateBase::DrawDebugInformation(const bool showRoomAisleInformation, const bool showVoxelGridType) const
{
	// 部屋と接続情報のデバッグ情報を表示します
	if (showRoomAisleInformation)
		DrawRoomAisleInformation();

	// ボクセルグリッドのデバッグ情報を表示します
	if (showVoxelGridType)
		DrawVoxelGridType();

#if 0
	// ダンジョン全体の領域を可視化
	{
		const FBox& bounding = BoundingBox();
		UKismetSystemLibrary::DrawDebugBox(
			FindWorld(),
			bounding.GetCenter(),
			bounding.GetExtent(),
			FColor::Orange,
			FRotator::ZeroRotator,
			0.f,
			5.f
		);
	}
#endif
}

void ADungeonGenerateBase::DrawRoomAisleInformation() const
{
	if (IsValid(mParameter) == false)
		return;

	UWorld* world = GetWorld();
	if (IsValid(world) == false)
		return;

	check(mGenerator);

	mGenerator->ForEach([this, world](const std::shared_ptr<const dungeon::Room>& room)
		{
			UKismetSystemLibrary::DrawDebugBox(
				world,
				room->GetCenter() * mParameter->GetGridSize().To3D(),
				room->GetExtent() * mParameter->GetGridSize().To3D(),
				FColor::Magenta,
				FRotator::ZeroRotator,
				0.f,
				10.f
			);

			UKismetSystemLibrary::DrawDebugSphere(
				world,
				room->GetGroundCenter() * mParameter->GetGridSize().To3D(),
				10.f,
				12,
				FColor::Magenta,
				0.f,
				2.f
			);
		}
	);

	mGenerator->EachAisle([this, world](const dungeon::Aisle& edge)
		{
			UKismetSystemLibrary::DrawDebugLine(
				world,
				*edge.GetPoint(0) * mParameter->GetGridSize().To3D(),
				*edge.GetPoint(1) * mParameter->GetGridSize().To3D(),
				FColor::Red,
				0.f,
				5.f
			);

			const FVector start(static_cast<int32>(edge.GetPoint(0)->X), static_cast<int32>(edge.GetPoint(0)->Y), static_cast<int32>(edge.GetPoint(0)->Z));
			const FVector goal(static_cast<int32>(edge.GetPoint(1)->X), static_cast<int32>(edge.GetPoint(1)->Y), static_cast<int32>(edge.GetPoint(1)->Z));
			UKismetSystemLibrary::DrawDebugSphere(
				world,
				start * mParameter->GetGridSize().To3D() + (mParameter->GetGridSize().To3D() / 2),
				10.f,
				12,
				FColor::Green,
				0.f,
				5.f
			);
			UKismetSystemLibrary::DrawDebugSphere(
				world,
				goal * mParameter->GetGridSize().To3D() + (mParameter->GetGridSize().To3D() / 2.),
				10.f,
				12,
				FColor::Red,
				0.f,
				5.f
			);

			return true;
		}
	);
}

void ADungeonGenerateBase::DrawVoxelGridType() const
{
	if (IsValid(mParameter) == false)
		return;

	UWorld* world = GetWorld();
	if (IsValid(world) == false)
		return;

	check(mGenerator);

	mGenerator->GetVoxel()->Each([this, world](const FIntVector& location, const dungeon::Grid& grid)
		{
			//if (grid.GetType() == dungeon::Grid::Aisle || grid.GetType() == dungeon::Grid::Slope)
			if (grid.GetType() != dungeon::Grid::Type::Empty && grid.GetType() != dungeon::Grid::Type::OutOfBounds)
			{
				const FVector halfGrid = mParameter->GetGridSize().To3D() / 2;
				UKismetSystemLibrary::DrawDebugBox(
					world,
					FVector(location.X, location.Y, location.Z) * mParameter->GetGridSize().To3D() + halfGrid,
					halfGrid * 0.95,
					grid.GetTypeColor(),
					FRotator::ZeroRotator,
					0.f,
					5.f
				);
			}

			return true;
		}
	);
}
#endif

// Vegetation
