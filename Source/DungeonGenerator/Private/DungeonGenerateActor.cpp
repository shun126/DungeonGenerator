/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGenerateActor.h"
#include "DungeonGeneratorCore.h"
#include "Core/Generator.h"
#include "Core/RoomGeneration/Room.h"
#include "Core/Debug/BuildInfomation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Vector.h"
#include "Core/Voxelization/Grid.h"
#include "Core/Voxelization/Voxel.h"
#include "Parameter/DungeonGenerateParameter.h"
#include <Components/CapsuleComponent.h>
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Engine/LevelStreaming.h>
#include <GameFramework/Character.h>
#include <Kismet/GameplayStatics.h>
#include <NavigationSystem.h>
#include <NavMesh/NavMeshBoundsVolume.h>
#include <NavMesh/RecastNavMesh.h>

#include <Engine/NetDriver.h>
#include <Net/UnrealNetwork.h>

#if WITH_EDITOR
#include <DrawDebugHelpers.h>
#include <Kismet/KismetSystemLibrary.h>
#endif

#define LOCTEXT_NAMESPACE "ADungeonGenerateActor"

ADungeonGenerateActor::ADungeonGenerateActor(const FObjectInitializer& initializer)
	: Super(initializer)
	, BuildJobTag(TEXT(JENKINS_JOB_TAG))
	, LicenseTag(TEXT(JENKINS_LICENSE))
	, LicenseId(TEXT(JENKINS_UUID))
{
	// Tick Enable
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;

	// Create root scene component
	RootComponent = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"), true);
	check(RootComponent);

	// Create default parameters
	DungeonGenerateParameter = NewObject<UDungeonGenerateParameter>();
	check(DungeonGenerateParameter);
}

ADungeonGenerateActor::~ADungeonGenerateActor()
{
	DestroyImplementation();
}

/********** InstancedStaticMesh **********/
void ADungeonGenerateActor::CreateInstancedMeshComponent(UStaticMesh* staticMesh)
{
	check(
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh ||
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh
	);

	for (const UInstancedStaticMeshComponent* mesh : InstancedStaticMeshes)
	{
		if (IsValid(mesh) && mesh->GetStaticMesh() == staticMesh)
			return;
	}

	if (DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh)
	{
		auto component = NewObject<UInstancedStaticMeshComponent>(this);
		AddInstanceComponent(component);
		component->RegisterComponent();
		component->SetStaticMesh(staticMesh);
		InstancedStaticMeshes.Add(component);
	}
	else
	{
		auto component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
		AddInstanceComponent(component);
		component->RegisterComponent();
		component->SetStaticMesh(staticMesh);
		component->bAutoRebuildTreeOnInstanceChanges = false;
		InstancedStaticMeshes.Add(component);
	}
}

void ADungeonGenerateActor::ClearInstancedMeshComponents()
{
	for (UInstancedStaticMeshComponent* instancedStaticMeshComponent : InstancedStaticMeshes)
	{
		if (IsValid(instancedStaticMeshComponent))
		{
			instancedStaticMeshComponent->UnregisterComponent();
			instancedStaticMeshComponent->ConditionalBeginDestroy();
			//instancedStaticMeshComponent->DestroyComponent();
		}
	}
	InstancedStaticMeshes.Empty();
}

void ADungeonGenerateActor::AddInstance(const UStaticMesh* staticMesh, const FTransform& transform)
{
	check(
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh ||
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh
	);

	for (UInstancedStaticMeshComponent* instancedStaticMeshComponent : InstancedStaticMeshes)
	{
		if (IsValid(instancedStaticMeshComponent) && instancedStaticMeshComponent->GetStaticMesh() == staticMesh)
		{
			instancedStaticMeshComponent->AddInstance(transform);
			break;
		}
	}
}

void ADungeonGenerateActor::CommitAddInstance()
{
	check(
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::InstancedStaticMesh ||
		DungeonMeshGenerationMethod == EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh
	);

	for (UInstancedStaticMeshComponent* instancedStaticMeshComponent : InstancedStaticMeshes)
	{
		auto hierarchicalInstancedStaticMesh = Cast<UHierarchicalInstancedStaticMeshComponent>(instancedStaticMeshComponent);
		if (IsValid(hierarchicalInstancedStaticMesh))
		{
			hierarchicalInstancedStaticMesh->bAutoRebuildTreeOnInstanceChanges = false;
			hierarchicalInstancedStaticMesh->BuildTreeIfOutdated(true, false);
		}
	}
}

void ADungeonGenerateActor::SetInstancedMeshCullDistance(const double cullDistance)
{
	for (UInstancedStaticMeshComponent* mesh : InstancedStaticMeshes)
	{
		if (IsValid(mesh))
		{
			mesh->SetCullDistances(cullDistance, cullDistance + 100);
		}
	}
}

/********** 生成と破棄 **********/
/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FBox ADungeonGenerateActor::ToWorldBoundingBox(const std::shared_ptr<const dungeon::Room>& room, const float gridSize)
{
	const FVector min = FVector(room->GetLeft(), room->GetTop(), room->GetBackground()) * gridSize;
	const FVector max = FVector(room->GetRight(), room->GetBottom(), room->GetForeground()) * gridSize;
	return FBox(min, max);
}

void ADungeonGenerateActor::PreGenerateImplementation()
{
	DestroyImplementation();

	if (!IsValid(DungeonGenerateParameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
		return;
	}

	mDungeonGeneratorCore = std::make_shared<CDungeonGeneratorCore>(GetWorld());
	if (mDungeonGeneratorCore == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate the DungeonGeneratorCore class"));
		return;
	}

	if (GetActorRotation().Equals(FRotator::ZeroRotator) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("The actor's rotation is not applied in the generated dungeon."));
	}

	if (GetActorScale().Equals(FVector::OneVector) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("The actor's scale is not applied in the generated dungeon."));
	}

	if (DungeonMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
	{
		// StaticMeshを先に登録
		DungeonGenerateParameter->EachFloorParts([this](const FDungeonMeshParts& meshParts)
			{
				CreateInstancedMeshComponent(meshParts.StaticMesh);
			}
		);
		DungeonGenerateParameter->EachWallParts([this](const FDungeonMeshParts& meshParts)
			{
				CreateInstancedMeshComponent(meshParts.StaticMesh);
			}
		);
		DungeonGenerateParameter->EachRoofParts([this](const FDungeonMeshParts& meshParts)
			{
				CreateInstancedMeshComponent(meshParts.StaticMesh);
			}
		);
		DungeonGenerateParameter->EachSlopeParts([this](const FDungeonMeshParts& meshParts)
			{
				CreateInstancedMeshComponent(meshParts.StaticMesh);
			}
		);
		DungeonGenerateParameter->EachPillarParts([this](const FDungeonMeshParts& meshParts)
			{
				CreateInstancedMeshComponent(meshParts.StaticMesh);
			}
		);

		// インスタンスメッシュを登録
		mDungeonGeneratorCore->OnAddFloor([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform);
			}
		);
		mDungeonGeneratorCore->OnAddSlope([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform);
			}
		);
		mDungeonGeneratorCore->OnAddWall([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform);
			}
		);
		mDungeonGeneratorCore->OnAddRoof([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform);
			}
		);
		mDungeonGeneratorCore->OnAddPillar([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(staticMesh, transform);
			}
		);
	}

	if (mDungeonGeneratorCore->Create(DungeonGenerateParameter, GetActorLocation(), HasAuthority()))
	{
		if (DungeonMeshGenerationMethod != EDungeonMeshGenerationMethod::StaticMesh)
			CommitAddInstance();

		/*
		List of PlayerStart, not including PlayerStartPIE.
		PlayerStartの一覧。PlayerStartPIEは含まない。
		*/
		TArray<APlayerStart*> playerStart;
		mDungeonGeneratorCore->MovePlayerStart(playerStart);
		mDungeonGeneratorCore->MovePlayerStartPIE(GetActorLocation(), playerStart);

		// スタート部屋とゴール部屋の位置を記録
		StartRoomLocation = mDungeonGeneratorCore->GetStartLocation();
		GoalRoomLocation = mDungeonGeneratorCore->GetGoalLocation();

		// 成功を通知
		OnGenerationSuccess.Broadcast();
	}
	else
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon"));
		DestroyImplementation();

		// 失敗を通知
		OnGenerationFailure.Broadcast();
	}

#if WITH_EDITOR
	// Record dungeon-generated random numbers
	GeneratedRandomSeed = DungeonGenerateParameter->GetGeneratedRandomSeed();
	GeneratedDungeonCRC32 = CalculateCRC32();

	if (HasAuthority())
		DungeonGenerateParameter->SetGeneratedDungeonCRC32(GeneratedDungeonCRC32);

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
}

void ADungeonGenerateActor::DestroyImplementation()
{
	ClearInstancedMeshComponents();

	if (mDungeonGeneratorCore != nullptr)
		mDungeonGeneratorCore.reset();

}

/********** ゲームの状態 **********/
void ADungeonGenerateActor::PreInitializeComponents()
{
	// Calling the parent class
	Super::PreInitializeComponents();

	if (AutoGenerateAtStart)
	{
		PreGenerateImplementation();
	}
}

void ADungeonGenerateActor::PostInitializeComponents()
{
	// Calling the parent class
	Super::PostInitializeComponents();
}

void ADungeonGenerateActor::BeginPlay()
{
	// Calling the parent class
	Super::BeginPlay();
}

void ADungeonGenerateActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyImplementation();

	// Calling the parent class
	Super::EndPlay(EndPlayReason);
}

void ADungeonGenerateActor::Tick(float DeltaSeconds)
{
	// Calling the parent class
	Super::Tick(DeltaSeconds);

	// Room generation notification
	if (mDungeonGeneratorCore != nullptr)
	{
	}

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	DrawDebugInformation();
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint Useful Functions
void ADungeonGenerateActor::GenerateDungeon()
{
	PreGenerateImplementation();
}

void ADungeonGenerateActor::GenerateDungeonWithParameter(UDungeonGenerateParameter* dungeonGenerateParameter)
{
	DungeonGenerateParameter = dungeonGenerateParameter;
	GenerateDungeon();
}

void ADungeonGenerateActor::DestroyDungeon()
{
	DestroyImplementation();
}

int32 ADungeonGenerateActor::FindFloorHeight(const float z) const
{
	if (mDungeonGeneratorCore == nullptr)
		return 0;

	const int32 gridZ = FindVoxelHeight(z);
	const std::shared_ptr<const dungeon::Generator>& generator = mDungeonGeneratorCore->GetGenerator();
	return generator->FindFloor(gridZ);
}

int32 ADungeonGenerateActor::FindVoxelHeight(const float z) const
{
	if (!IsValid(DungeonGenerateParameter))
		return 0;

	return static_cast<int32>(z / DungeonGenerateParameter->GetGridSize().VerticalSize);
}


int32 ADungeonGenerateActor::CalculateCRC32() const noexcept
{
	return mDungeonGeneratorCore ? static_cast<int32>(mDungeonGeneratorCore->CalculateCRC32()) : 0;
}

#if WITH_EDITOR
int32 ADungeonGenerateActor::GetGeneratedDungeonCRC32() const noexcept
{
	return GeneratedDungeonCRC32;
}
#endif

FBox ADungeonGenerateActor::CalculateBoundingBox() const
{
	if (mDungeonGeneratorCore)
		return mDungeonGeneratorCore->CalculateBoundingBox();
	return FBox(EForceInit::ForceInitToZero);
}

FVector ADungeonGenerateActor::GetRoomMaxSize() const
{
	if (DungeonGenerateParameter)
	{
		FVector result;
		result.X = DungeonGenerateParameter->GetRoomWidth().Max;
		result.Y = DungeonGenerateParameter->GetRoomDepth().Max;
		result.Z = DungeonGenerateParameter->GetRoomHeight().Max;
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

void ADungeonGenerateActor::DrawDebugInformation()
{
	if (!IsValid(DungeonGenerateParameter))
		return;

	if (mDungeonGeneratorCore == nullptr)
		return;

	std::shared_ptr<const dungeon::Generator> generator = mDungeonGeneratorCore->GetGenerator();
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

					const size_t index = static_cast<size_t>(grid.GetType());
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
						message.Append(TEXT("Type: ") + grid.GetTypeName() + TEXT("\n"));
						message.Append(TEXT("Props: ") + grid.GetPropsName() + TEXT("\n"));
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
		output.Add(TEXT("Type: ") + grid.GetTypeName());
		output.Add(TEXT("Props: ") + grid.GetPropsName());

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
