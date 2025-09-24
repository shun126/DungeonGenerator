/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.

ADungeonGeneratedActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
ADungeonGenerateActorは配置可能(Placeable)、ADungeonGeneratedActorは配置不可能(NotPlaceable)にするため、
継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
*/

#include "DungeonGenerateActor.h"
#include "PluginInformation.h"
#include "Core/Generator.h"
#include "Core/Debug/BuildInformation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Vector.h"
#include "Core/Voxelization/Grid.h"
#include "Core/Voxelization/Voxel.h"
#include "Helper/DungeonAisleGridMap.h"
#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "Parameter/DungeonGenerateParameter.h"
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Engine/LevelStreaming.h>
#include <Kismet/GameplayStatics.h>

#include <Engine/NetDriver.h>

#if WITH_EDITOR
#include <DrawDebugHelpers.h>
#include <Kismet/KismetSystemLibrary.h>
#endif

#define LOCTEXT_NAMESPACE "ADungeonGenerateActor"

ADungeonGenerateActor::ADungeonGenerateActor(const FObjectInitializer& initializer)
	: Super(initializer)
	, BuildJobTag(TEXT(DUNGENERATOR_PLUGIN_VERSION_NAME "-" JENKINS_JOB_TAG))
	, LicenseTag(TEXT(JENKINS_LICENSE))
	, LicenseId(TEXT(JENKINS_UUID))
{
	// Tick Enable
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 6.f / 60.f;

	SetCanBeDamaged(false);
}

void ADungeonGenerateActor::PreInitializeComponents()
{
	// Calling the parent class
	Super::PreInitializeComponents();

	if (AutoGenerateAtStart == true)
	{
		PreGenerateImplementation();
	}
}

void ADungeonGenerateActor::PostInitializeComponents()
{
	// Calling the parent class
	Super::PostInitializeComponents();

	if (AutoGenerateAtStart == true)
	{
		PostGenerateImplementation();
	}

	//if (GetNetMode() != NM_Standalone)
	if (GetLocalRole() == ROLE_Authority)
	{
		SetReplicates(true);
		SetAutonomousProxy(true);
	}
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
#endif
}

void ADungeonGenerateActor::Tick(float DeltaSeconds)
{
	// Calling the parent class
	Super::Tick(DeltaSeconds);


#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	DrawDebugInformation();
#endif
}

void ADungeonGenerateActor::OnPreDungeonGeneration()
{
	if (auto* level = GetLevel())
	{
		if (auto* levelScript = Cast<ADungeonMainLevelScriptActor>(level->GetLevelScriptActor()))
			levelScript->OnPreDungeonGeneration(this);
	}
}

void ADungeonGenerateActor::OnPostDungeonGeneration(const bool result)
{
	if (auto* level = GetLevel())
	{
		if (auto* levelScript = Cast<ADungeonMainLevelScriptActor>(level->GetLevelScriptActor()))
			levelScript->OnPostDungeonGeneration(this, result);
	}
}

/********** InstancedStaticMesh **********/
uint32 ADungeonGenerateActor::InstancedMeshHash(const FVector& position, const double quantizationSize)
{
	uint32 x = static_cast<uint32>(position.X / quantizationSize);
	uint32 y = static_cast<uint32>(position.Y / quantizationSize);
	y <<= 16;
	x &= 0xFFFF;
	return y | x;
}

void ADungeonGenerateActor::BeginInstanceTransaction()
{
	for (auto& chunk : mInstancedMeshCluster)
	{
		chunk.Value.BeginTransaction();
	}
}

void ADungeonGenerateActor::AddInstance(UStaticMesh* staticMesh, const FTransform& transform, const EDungeonMeshGenerationMethod meshGenerationMethod)
{
	check(
		meshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh ||
		meshGenerationMethod == EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh
	);

	double quantizationSize = 25 * 100;
	if (mParameter)
		quantizationSize = mParameter->GetGridSize().HorizontalSize * 5.f;
	const uint32 hash = InstancedMeshHash(transform.GetTranslation(), quantizationSize);
	auto& chunk = mInstancedMeshCluster.FindOrAdd(hash);
	if (meshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh)
	{
		auto* component = chunk.FindOrCreateInstance(this, staticMesh);
		component->AddInstance(transform);
	}
	else
	{
		auto* component = chunk.FindOrCreateHierarchicalInstance(this, staticMesh);
		component->AddInstance(transform);
	}
}

void ADungeonGenerateActor::EndInstanceTransaction()
{
	mInstancedMeshCluster.Shrink();

	for (auto& chunk : mInstancedMeshCluster)
	{
		chunk.Value.EndTransaction();
	}

	if (mInstancedMeshCullDistance.Min < mInstancedMeshCullDistance.Max)
		ApplyInstancedMeshCullDistance();
}

void ADungeonGenerateActor::DestroyAllInstance()
{
	for (auto& chunk : mInstancedMeshCluster)
	{
		chunk.Value.DestroyAll();
	}
	mInstancedMeshCluster.Reset();
}

void ADungeonGenerateActor::SetInstancedMeshCullDistance(const FInt32Interval& cullDistance)
{
	mInstancedMeshCullDistance = cullDistance;
	ApplyInstancedMeshCullDistance();
}

void ADungeonGenerateActor::ApplyInstancedMeshCullDistance()
{
	for (auto& pair : mInstancedMeshCluster)
	{
		pair.Value.SetCullDistance(mInstancedMeshCullDistance);
	}
}

/********** 生成と破棄 **********/
void ADungeonGenerateActor::PreGenerateImplementation()
{
	if (!IsValid(DungeonGenerateParameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
		return;
	}

	Dispose(true);

	// インスタンスメッシュを登録
	if (DungeonMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
	{
		OnAddFloor([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod);
			}
		);
		OnAddSlope([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod);
			}
		);
		OnAddCatwalk([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonMeshGenerationMethod);
			}
		);
	}

	// インスタンスメッシュを登録
	if (DungeonWallRoofPillarMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
	{
		OnAddWall([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod);
			}
		);
		OnAddRoof([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod);
			}
		);
		OnAddPillar([this](UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform, DungeonWallRoofPillarMeshGenerationMethod);
			}
		);
	}

	BeginInstanceTransaction();

	if (BeginDungeonGeneration(DungeonGenerateParameter, HasAuthority()) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon. Seed(%d)"), DungeonGenerateParameter->GetRandomSeed());

		EndDungeonGeneration();
		Dispose(false);
		return;
	}

	EndInstanceTransaction();

	/*
	List of PlayerStart, not including PlayerStartPIE.
	PlayerStartの一覧。PlayerStartPIEは含まない。
	*/
	TArray<APlayerStart*> playerStart;
	CollectPlayerStartExceptPlayerStartPIE(playerStart);
	MovePlayerStart(playerStart);

	// スタート部屋とゴール部屋の位置を記録
	StartRoomLocation = GetStartLocation();
	GoalRoomLocation = GetGoalLocation();

#if WITH_EDITOR
	// Record dungeon-generated random numbers
	GeneratedRandomSeed = DungeonGenerateParameter->GetGeneratedRandomSeed();
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

	EndDungeonGeneration();
}

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
	if (IsGenerated())
	{
		DestroyAllInstance();
	}

	Super::Dispose(flushStreamLevels);
}

void ADungeonGenerateActor::FitNavMeshBoundsVolume()
{
	Super::FitNavMeshBoundsVolume();

	if (DungeonMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
		ReregisterAllComponents();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint Useful Functions
// サーバープロセスで実行する
void ADungeonGenerateActor::GenerateDungeon_Implementation()
{
#if JENKINS_FOR_DEVELOP
	DUNGEON_GENERATOR_LOG(TEXT("ServerOnGenerateDungeon: %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
#endif
	MulticastOnGenerateDungeon();
}

// 全てのクライアントプロセスで実行する
void ADungeonGenerateActor::MulticastOnGenerateDungeon_Implementation()
{
#if JENKINS_FOR_DEVELOP
	DUNGEON_GENERATOR_LOG(TEXT("MulticastOnGenerateDungeon: %s"), HasAuthority() ? TEXT("Server") : TEXT("Client"));
#endif
	PreGenerateImplementation();
	PostGenerateImplementation();
}

void ADungeonGenerateActor::GenerateDungeonWithParameter(UDungeonGenerateParameter* dungeonGenerateParameter)
{
	DungeonGenerateParameter = dungeonGenerateParameter;
	GenerateDungeon();
}

void ADungeonGenerateActor::DestroyDungeon()
{
	Dispose(false);
}

int32 ADungeonGenerateActor::FindFloorHeight(const float z) const
{
	const int32 gridZ = FindVoxelHeight(z);
	const std::shared_ptr<const dungeon::Generator>& generator = GetGenerator();
	return generator->FindFloor(gridZ);
}

int32 ADungeonGenerateActor::FindVoxelHeight(const float z) const
{
	if (!IsValid(DungeonGenerateParameter))
		return 0;

	return static_cast<int32>(z / DungeonGenerateParameter->GetGridSize().VerticalSize);
}


#if WITH_EDITOR
int32 ADungeonGenerateActor::GetGeneratedDungeonCRC32() const noexcept
{
	return GeneratedDungeonCRC32;
}
#endif

float ADungeonGenerateActor::GetGridSize() const
{
	return DungeonGenerateParameter && DungeonGenerateParameter->GridSize;
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

void ADungeonGenerateActor::DrawDebugInformation() const
{
	if (!IsValid(DungeonGenerateParameter))
		return;

	std::shared_ptr<const dungeon::Generator> generator = GetGenerator();
	if (generator == nullptr)
		return;

	const APawn* playerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (IsValid(playerPawn) == false)
		return;
	const FVector playerLocation = playerPawn->GetActorLocation() - GetActorLocation();
	const FIntVector playerGridLocation = DungeonGenerateParameter->ToGrid(playerLocation);

	// Displays voxel grid debugging information
	if (ShowVoxelGridType)
	{
		generator->GetVoxel()->Each([this, &playerGridLocation](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (grid.GetType() != dungeon::Grid::Type::Empty && grid.GetType() != dungeon::Grid::Type::OutOfBounds)
				{
					const FVector gridSize = DungeonGenerateParameter->GetGridSize().To3D();
					const FVector halfGridSize = gridSize / 2.;
					const FVector center = dungeon::ToVector(location) * gridSize + halfGridSize + GetActorLocation();
					const FColor& color = dungeon::Grid::GetTypeColor(grid.GetType());

					UKismetSystemLibrary::DrawDebugBox(
						GetWorld(),
						center,
						halfGridSize * 0.95,
						color,
						FRotator::ZeroRotator,
						0.f,
						10.f
					);

					const FVector direction = dungeon::ToVector(grid.GetDirection().GetVector());
					UKismetSystemLibrary::DrawDebugArrow(
						GetWorld(),
						center,
						center + halfGridSize * direction,
						halfGridSize.Length(),
						color,
						0.f,
						5.f
					);

					constexpr int32 range = 3;
					constexpr int32 squaredRange = range * range;
					const FIntVector delta = location - playerGridLocation;
					const FIntVector squaredDelta = delta * delta;
					if (squaredDelta.X <= squaredRange && squaredDelta.Y <= squaredRange && squaredDelta.Z <= squaredRange)
					{
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
						message.Append(grid.GetNoMeshGenerationName());
						DrawDebugString(
							GetWorld(),
							center,
							message,
							nullptr,
							color,
							0.f,
							true,
							1.f
						);
					}
				}

				return true;
			}
		);
	}

	// Displays debugging information for the voxel grid at the player's position
	if (ShowVoxelGridTypeAtPlayerLocation)
	{
		TArray<FString> output;

		output.Add(TEXT("Location:")
			+ FString::SanitizeFloat(playerLocation.X) + TEXT(",")
			+ FString::SanitizeFloat(playerLocation.Y) + TEXT(",")
			+ FString::SanitizeFloat(playerLocation.Z));
		output.Add(TEXT("Grid:")
			+ FString::FromInt(playerGridLocation.X) + TEXT(",")
			+ FString::FromInt(playerGridLocation.Y) + TEXT(",")
			+ FString::FromInt(playerGridLocation.Z));

		const dungeon::Grid& grid = generator->GetGrid(playerGridLocation);
		output.Add(TEXT("Identifier:") + FString::FromInt(grid.GetIdentifier()));
		output.Add(TEXT("DepthRatioFromStart:") + FString::SanitizeFloat(static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f));
		output.Add(TEXT("Type: ") + grid.GetTypeName());
		output.Add(TEXT("Props: ") + grid.GetPropsName());
		if (grid.IsReserved())
			output.Add(TEXT("Reserved"));
		if (grid.IsCatwalk())
			output.Add(TEXT("Catwalk"));
		if (grid.IsSubLevel())
			output.Add(TEXT("SubLevel"));
		
		FString message;
		for (const FString& line : output)
		{
			message.Append(line);
			message.Append(TEXT("\n"));
		}

		message.Append(grid.GetNoMeshGenerationName() + TEXT("\n"));

		DrawDebugString(
			playerPawn->GetWorld(),
			playerPawn->GetPawnViewLocation(),
			message,
			nullptr,
			dungeon::Grid::GetTypeColor(grid.GetType()),
			0,
			true,
			1.f
		);
	}
}
#endif

#undef LOCTEXT_NAMESPACE
