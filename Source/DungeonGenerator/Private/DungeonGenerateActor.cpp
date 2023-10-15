/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGenerateActor.h"
#include "DungeonGenerateParameter.h"
#include "DungeonGeneratorCore.h"
#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.h"
#include "Core/Grid.h"
#include "Core/Generator.h"
#include "Core/Room.h"
#include "Core/Voxel.h"
#include "Core/Debug/BuildInfomation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Vector.h"
#include "MiniMap/DungeonMiniMapTextureLayer.h"
#include <Components/CapsuleComponent.h>
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

ADungeonGenerateActor::ADungeonGenerateActor(const FObjectInitializer& initializer)
	: Super(initializer)
	, BuildJobTag(TEXT(JENKINS_JOB_TAG))
	, LicenseTag(TEXT(JENKINS_LICENSE))
	, LicenseId(TEXT(JENKINS_UUID))
{
	// Tick Enable
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;

	// Replicates
	bReplicates = ReplicatedServerDungeon;

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

void ADungeonGenerateActor::BeginAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh))
		{
			mesh->BeginTransaction(true);
		}
	}
}

void ADungeonGenerateActor::AddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs, const UStaticMesh* staticMesh, const FTransform& transform)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh) && mesh->GetStaticMesh() == staticMesh)
		{
			mesh->AddInstance(transform);
			break;
		}
	}
}

void ADungeonGenerateActor::EndAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs)
{
	for (auto mesh : meshs)
	{
		if (IsValid(mesh))
		{
			mesh->EndTransaction(true);
		}
	}
}

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

	// Reset
	mDungeonGeneratorCore->OnResetDoor([this](AActor* actor, EDungeonRoomProps props)
		{
			OnResetDoor.Broadcast(actor, props);
		}
	);

	if (InstancedStaticMesh)
	{
		DungeonGenerateParameter->EachFloorParts([this](const FDungeonMeshParts& meshParts)
			{
				for (const UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* mesh : FloorMeshs)
				{
					if (IsValid(mesh) && mesh->GetStaticMesh() == meshParts.StaticMesh)
						return;
				}
				UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* component
					= NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
				if (IsValid(component))
				{
					AddInstanceComponent(component);
					component->RegisterComponent();
					component->SetStaticMesh(meshParts.StaticMesh);
					FloorMeshs.Add(component);
				}
			}
		);
		DungeonGenerateParameter->EachWallParts([this](const FDungeonMeshParts& meshParts)
			{
				for (const UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* mesh : WallMeshs)
				{
					if (IsValid(mesh) && mesh->GetStaticMesh() == meshParts.StaticMesh)
						return;
				}
				UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* component
					= NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
				if (IsValid(component))
				{
					AddInstanceComponent(component);
					component->RegisterComponent();
					component->SetStaticMesh(meshParts.StaticMesh);
					WallMeshs.Add(component);
				}
			}
		);
		DungeonGenerateParameter->EachRoofParts([this](const FDungeonMeshParts& meshParts)
			{
				for (const UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* mesh : RoofMeshs)
				{
					if (IsValid(mesh) && mesh->GetStaticMesh() == meshParts.StaticMesh)
						return;
				}
				UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* component
					= NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
				if (IsValid(component))
				{
					AddInstanceComponent(component);
					component->RegisterComponent();
					component->SetStaticMesh(meshParts.StaticMesh);
					RoofMeshs.Add(component);
				}
			}
		);
		DungeonGenerateParameter->EachSlopeParts([this](const FDungeonMeshParts& meshParts)
			{
				for (const UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* mesh : SlopeMeshs)
				{
					if (IsValid(mesh) && mesh->GetStaticMesh() == meshParts.StaticMesh)
						return;
				}
				UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* component
					= NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
				if (IsValid(component))
				{
					AddInstanceComponent(component);
					component->RegisterComponent();
					component->SetStaticMesh(meshParts.StaticMesh);
					SlopeMeshs.Add(component);
				}
			}
		);
		DungeonGenerateParameter->EachPillarParts([this](const FDungeonMeshParts& meshParts)
			{
				for (const UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* mesh : PillarMeshs)
				{
					if (IsValid(mesh) && mesh->GetStaticMesh() == meshParts.StaticMesh)
						return;
				}
				UDungeonTransactionalHierarchicalInstancedStaticMeshComponent* component
					= NewObject<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>(this);
				if (IsValid(component))
				{
					AddInstanceComponent(component);
					component->RegisterComponent();
					component->SetStaticMesh(meshParts.StaticMesh);
					PillarMeshs.Add(component);
				}
			}
		);

		BeginAddInstance(FloorMeshs);
		BeginAddInstance(WallMeshs);
		BeginAddInstance(RoofMeshs);
		BeginAddInstance(SlopeMeshs);
		BeginAddInstance(PillarMeshs);

		mDungeonGeneratorCore->OnAddFloor([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(FloorMeshs, staticMesh, transform);
				OnCreateFloor.Broadcast(transform);
			}
		);
		mDungeonGeneratorCore->OnAddSlope([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(SlopeMeshs, staticMesh, transform);
				OnCreateSlope.Broadcast(transform);
			}
		);
		mDungeonGeneratorCore->OnAddWall([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(WallMeshs, staticMesh, transform);
				OnCreateWall.Broadcast(transform);
			}
		);
		mDungeonGeneratorCore->OnAddRoof([this](const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(RoofMeshs, staticMesh, transform);
				OnCreateRoomRoof.Broadcast(transform);
			}
		);
		mDungeonGeneratorCore->OnAddPillar([this](uint32_t gridHeight, const UStaticMesh* staticMesh, const FTransform& transform)
			{
				AddInstance(PillarMeshs, staticMesh, transform);
				OnCreatePillar.Broadcast(transform);
			}
		);

		if (mDungeonGeneratorCore->Create(DungeonGenerateParameter))
		{
			EndAddInstance(FloorMeshs);
			EndAddInstance(WallMeshs);
			EndAddInstance(RoofMeshs);
			EndAddInstance(SlopeMeshs);
			EndAddInstance(PillarMeshs);
			MovePlayerStart();
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon"));
			DestroyImplementation();
		}
	}
	else
	{
		if (mDungeonGeneratorCore->Create(DungeonGenerateParameter))
		{
			MovePlayerStart();
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon"));
			DestroyImplementation();
		}
	}

#if WITH_EDITOR
	// Record dungeon-generated random numbers
	GeneratedRandomSeed = DungeonGenerateParameter->GetGeneratedRandomSeed();
	GeneratedDungeonCRC32 = CalculateCRC32();
#endif
}

void ADungeonGenerateActor::PostGenerateImplementation()
{
	if (mDungeonGeneratorCore == nullptr)
		return;

	// refresh navigation system
	if (OnRoomCreated.IsBound())
	{
		std::shared_ptr<const dungeon::Generator> generator = mDungeonGeneratorCore->GetGenerator();
		if (generator)
		{
			UNavigationSystemV1* navigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
			const float gridSize = DungeonGenerateParameter->GetGridSize();

			generator->ForEach([this, gridSize, navigationSystem](const std::shared_ptr<const dungeon::Room>& room)
				{
					const FBox box = ToWorldBoundingBox(room, gridSize);
					OnRoomCreated.Broadcast(false, static_cast<EDungeonRoomParts>(room->GetParts()), box);

					if (navigationSystem)
					{
						navigationSystem->AddDirtyArea(box, 0);
					}
				}
			);

			if (navigationSystem)
			{
				navigationSystem->Build();
			}
		}
	}
}

void ADungeonGenerateActor::DestroyImplementation()
{
	if (mDungeonGeneratorCore != nullptr)
		mDungeonGeneratorCore.reset();

	FloorMeshs.Empty();
	SlopeMeshs.Empty();
	WallMeshs.Empty();
	RoofMeshs.Empty();
	PillarMeshs.Empty();

	DungeonMiniMapTextureLayer = nullptr;
}

/*
The starting position PlayerStart, the ending position ADungeonPlayerGoal,
and the position of the Moves the player-controlled character.
*/
void ADungeonGenerateActor::MovePlayerStart()
{
	mDungeonGeneratorCore->MovePlayerStart();
}

void ADungeonGenerateActor::PreInitializeComponents()
{
	// Calling the parent class
	Super::PreInitializeComponents();

	if (AutoGenerateAtStart)
	{
		if (ReplicatedServerDungeon)
		{
			if (HasAuthority() == false)
			{
				if (IsValid(DungeonGenerateParameter))
				{
#if WITH_EDITOR
					DUNGEON_GENERATOR_LOG(TEXT("PreInitializeComponents: RandomSeed=%d, GeneratedRandomSeed=%d"),
						DungeonGenerateParameter->GetRandomSeed(),
						DungeonGenerateParameter->GetGeneratedRandomSeed()
					);
#endif
					DungeonGenerateParameter->SetRandomSeed(DungeonGenerateParameter->GetGeneratedRandomSeed());
				}
				else
				{
					DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
				}
			}
			else
			{
				DUNGEON_GENERATOR_LOG(TEXT("Has Authority"));
			}
		}

		PreGenerateImplementation();

#if WITH_EDITOR
		if (IsValid(DungeonGenerateParameter))
		{
			DUNGEON_GENERATOR_LOG(TEXT("PreInitializeComponents: RandomSeed=%d, GeneratedRandomSeed=%d"),
				DungeonGenerateParameter->GetRandomSeed(),
				DungeonGenerateParameter->GetGeneratedRandomSeed()
			);
		}
#endif
	}
}

void ADungeonGenerateActor::PostInitializeComponents()
{
	// Calling the parent class
	Super::PostInitializeComponents();

	if (mDungeonGeneratorCore)
	{
		mDungeonGeneratorCore->FlushLoadStreamLevels();
	}
}

void ADungeonGenerateActor::BeginPlay()
{
	// Calling the parent class
	Super::BeginPlay();

	if (AutoGenerateAtStart)
	{
		mPostGenerated = false;
	}
}

void ADungeonGenerateActor::Tick(float DeltaSeconds)
{
	// Calling the parent class
	Super::Tick(DeltaSeconds);

	if (!mPostGenerated)
	{
		mPostGenerated = true;

		PostGenerateImplementation();

#if WITH_EDITOR
		if (IsValid(DungeonGenerateParameter))
		{
			DUNGEON_GENERATOR_LOG(TEXT("PostGenerateImplementation: RandomSeed=%d, GeneratedRandomSeed=%d, CRC32=%d"),
				DungeonGenerateParameter->GetRandomSeed(),
				DungeonGenerateParameter->GetGeneratedRandomSeed(),
				GeneratedRandomSeed,
				CalculateCRC32()
			);
		}
#endif
	}

	// Room generation notification
	if (mDungeonGeneratorCore != nullptr)
	{
		mDungeonGeneratorCore->AsyncLoadStreamLevels();
	}

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	DrawDebugInformation();
#endif
}

void ADungeonGenerateActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyImplementation();

	// Calling the parent class
	Super::EndPlay(EndPlayReason);
}

void ADungeonGenerateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Calling the parent class
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Set variables to be replicated
	//DOREPLIFETIME(ADungeonGenerateActor, GeneratedRandomSeed);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint Useful Functions
void ADungeonGenerateActor::GenerateDungeon()
{
	PreGenerateImplementation();
	PostGenerateImplementation();
}

void ADungeonGenerateActor::GenerateDungeonWithSeed(const int32 generatedRandomSeed)
{
	if (IsValid(DungeonGenerateParameter))
	{
		//DungeonGenerateParameter->SetGeneratedRandomSeed(generatedRandomSeed);
		GenerateDungeon();
	}
	else
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set"));
	}
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

	return static_cast<int32>(z / DungeonGenerateParameter->GetGridSize());
}

UDungeonMiniMapTextureLayer* ADungeonGenerateActor::GenerateMiniMapTextureLayerWithSize(const int32 textureWidth)
{
	if (textureWidth <= 0)
		return nullptr;

	if (!IsValid(DungeonGenerateParameter))
		return nullptr;

	const float gridSize = DungeonGenerateParameter->GetGridSize();

	DungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>(this);
	if (!IsValid(DungeonMiniMapTextureLayer))
		return nullptr;

	if (DungeonMiniMapTextureLayer->GenerateMiniMapTextureWithSize(mDungeonGeneratorCore, textureWidth, gridSize) == false)
		DungeonMiniMapTextureLayer = nullptr;

	return DungeonMiniMapTextureLayer;
}

UDungeonMiniMapTextureLayer* ADungeonGenerateActor::GenerateMiniMapTextureLayerWithScale(const int32 dotScale)
{
	if (dotScale <= 0)
		return nullptr;

	if (!IsValid(DungeonGenerateParameter))
		return nullptr;

	const float gridSize = DungeonGenerateParameter->GetGridSize();

	DungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>(this);
	if (!IsValid(DungeonMiniMapTextureLayer))
		return nullptr;

	if (DungeonMiniMapTextureLayer->GenerateMiniMapTextureWithScale(mDungeonGeneratorCore, dotScale, gridSize) == false)
		DungeonMiniMapTextureLayer = nullptr;

	return DungeonMiniMapTextureLayer;
}

UDungeonMiniMapTextureLayer* ADungeonGenerateActor::GetGeneratedMiniMapTextureLayer() const
{
	return DungeonMiniMapTextureLayer;
}

int32 ADungeonGenerateActor::CalculateCRC32() const noexcept
{
	return mDungeonGeneratorCore ? static_cast<int32>(mDungeonGeneratorCore->CalculateCRC32()) : 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// for debug
#if WITH_EDITOR
bool ADungeonGenerateActor::ShouldTickIfViewportsOnly() const
{
	return true;
}
#endif

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
void ADungeonGenerateActor::DrawDebugInformation()
{
	if (!IsValid(DungeonGenerateParameter))
		return;

	if (mDungeonGeneratorCore == nullptr)
		return;

	std::shared_ptr<const dungeon::Generator> generator = mDungeonGeneratorCore->GetGenerator();
	if (generator == nullptr)
		return;
	
	const float gridSize = DungeonGenerateParameter->GetGridSize();

	// Displays debugging information on room and connection information
	if (ShowRoomAisleInformation)
	{
		// Debugging information by room
		generator->ForEach([this, gridSize](const std::shared_ptr<const dungeon::Room>& room)
			{
				UKismetSystemLibrary::DrawDebugBox(
					GetWorld(),
					room->GetCenter() * gridSize,
					room->GetExtent() * gridSize,
					FColor::Magenta,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);

				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					room->GetGroundCenter() * gridSize,
					10.f,
					12,
					FColor::Magenta,
					0.f,
					2.f
				);
			}
		);

		// Debugging information per grid
		generator->EachAisle([this, gridSize](const dungeon::Aisle& edge)
			{
				UKismetSystemLibrary::DrawDebugLine(
					GetWorld(),
					*edge.GetPoint(0) * gridSize,
					*edge.GetPoint(1) * gridSize,
					FColor::Red,
					0.f,
					5.f
				);

				const FVector start(static_cast<int32>(edge.GetPoint(0)->X), static_cast<int32>(edge.GetPoint(0)->Y), static_cast<int32>(edge.GetPoint(0)->Z));
				const FVector goal(static_cast<int32>(edge.GetPoint(1)->X), static_cast<int32>(edge.GetPoint(1)->Y), static_cast<int32>(edge.GetPoint(1)->Z));
				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					start * gridSize + FVector(gridSize / 2., gridSize / 2., gridSize / 2.),
					10.f,
					12,
					FColor::Green,
					0.f,
					5.f
				);
				UKismetSystemLibrary::DrawDebugSphere(
					GetWorld(),
					goal * gridSize + FVector(gridSize / 2., gridSize / 2., gridSize / 2.),
					10.f,
					12,
					FColor::Red,
					0.f,
					5.f
				);
			}
		);
	}

	// Displays voxel grid debugging information
	if (ShowVoxelGridType)
	{
		const APawn* playerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (IsValid(playerPawn))
		{
			const FVector playerLocation = playerPawn->GetActorLocation();
			generator->GetVoxel()->Each([this, gridSize, &playerLocation](const FIntVector& location, const dungeon::Grid& grid)
				{
					static const FColor colors[] = {
						FColor::Blue,		// Floor
						FColor::Yellow,		// Deck
						FColor::Red,		// Gate
						FColor::Green,		// Aisle
						FColor::Magenta,	// Slope
						FColor::Cyan,		// Atrium
						FColor::Black,		// Empty
						FColor::Black,		// OutOfBounds
					};
					static constexpr size_t colorSize = sizeof(colors) / sizeof(colors[0]);
					static_assert(colorSize == dungeon::Grid::TypeSize);

					//if (grid.mType == dungeon::Grid::Type::Aisle || grid.mType == dungeon::Grid::Type::Slope)
					if (grid.GetType() != dungeon::Grid::Type::Empty && grid.GetType() != dungeon::Grid::Type::OutOfBounds)
					{
						const double halfGridSize = gridSize / 2.;
						const FVector halfGrid(halfGridSize, halfGridSize, halfGridSize);
						const FVector center = dungeon::ToVector(location) * gridSize + halfGrid;

						const size_t index = static_cast<size_t>(grid.GetType());
						UKismetSystemLibrary::DrawDebugBox(
							GetWorld(),
							center,
							halfGrid * 0.95,
							colors[index],
							FRotator::ZeroRotator,
							0.f,
							10.f
						);

						const FVector direction = dungeon::ToVector(grid.GetDirection().GetVector());
						UKismetSystemLibrary::DrawDebugArrow(
							GetWorld(),
							center,
							center + direction * halfGridSize,
							gridSize,
							colors[index],
							0.f,
							5.f
						);

						if (FVector::Distance(center, playerLocation) < 10 * 100)
						{
							DrawDebugString(
								GetWorld(),
								center,
								grid.GetNoMeshGenerationName(),
								nullptr,
								FColor::White,
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
	}

#if 0
	// Visualization of the entire dungeon area
	{
		const FBox& bounding = BoundingBox();
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounding.GetCenter(),
			bounding.GetExtent(),
			FColor::Orange,
			FRotator::ZeroRotator,
			0.f,
			5.f
		);
	}
#endif

	// Displays debugging information for the voxel grid at the player's position
	if (ShowVoxelGridTypeAtPlayerLocation)
	{
		APawn* playerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
		if (IsValid(playerPawn))
		{
			TArray<FString> output;

			FIntVector location = DungeonGenerateParameter->ToGrid(playerPawn->GetActorLocation());
			output.Add(TEXT("Grid:") + FString::FromInt(location.X) + TEXT(",") + FString::FromInt(location.Y) + TEXT(",") + FString::FromInt(location.Z));

			const dungeon::Grid& grid = generator->GetGrid(location);
			output.Add(TEXT("Identifier:") + FString::FromInt(grid.GetIdentifier()));
			output.Add(TEXT(" Type : ") + grid.GetTypeName());
			//Direction GetDirection() const noexcept;
			output.Add(TEXT(" Props : ") + grid.GetPropsName());

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
				FColor::White,
				0,
				true,
				1.f
			);
		}
	}
}
#endif
