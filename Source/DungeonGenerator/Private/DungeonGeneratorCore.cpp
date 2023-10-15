/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorCore.h"
#include "DungeonGenerateParameter.h"
#include "Core/Debug/Debug.h"
#include "Core/Debug/BuildInfomation.h"
#include "Core/Identifier.h"
#include "Core/Generator.h"
#include "Core/Voxel.h"
#include "Core/Debug/Stopwatch.h"
#include "Core/Math/Math.h"
#include "Core/Math/Random.h"
#include "Decorator/DungeonInteriorUnplaceableBounds.h"
#include "Mission/DungeonRoomProps.h"
#include "SubActor/DungeonRoomSensor.h"
#include "SubActor/DungeonDoor.h"
#include "SubLevel/DungeonLevelStreamingDynamic.h"
#include "SubLevel/DungeonRoomLocator.h"
#include "PluginInfomation.h"
#include <TextureResource.h>
#include <Components/StaticMeshComponent.h>
#include <GameFramework/PlayerStart.h>
#include <Engine/LevelStreamingDynamic.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/StaticMeshActor.h>
#include <Engine/Texture2D.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/Paths.h>
#include <NavMesh/NavMeshBoundsVolume.h>
#include <NavMesh/RecastNavMesh.h>

#include <Components/BrushComponent.h>
#include <Components/LocalLightComponent.h>
#include <Engine/Polys.h>

#if WITH_EDITOR
// UnrealEd
#include <EditorActorFolders.h>
#include <EditorLevelUtils.h>
#include <Builders/CubeBuilder.h>
#endif

// 定義するとミッショングラフのデバッグファイル(PlantUML)を出力します
#define DEBUG_GENERATE_MISSION_GRAPH_FILE

static const FName DungeonGeneratorTag(TEXT("DungeonGenerator"));

namespace
{
	FTransform GetWorldTransform_(const float yaw, const FVector& position)
	{
		return FTransform(FRotator(0.f, yaw, 0.f).Quaternion(), position);
	}
}

const FName& CDungeonGeneratorCore::GetDungeonGeneratorTag()
{
	return DungeonGeneratorTag;
}

CDungeonGeneratorCore::CDungeonGeneratorCore(const TWeakObjectPtr<UWorld>& world)
	: mWorld(world)
	, mInteriorUnplaceableBounds(std::make_shared<CDungeonInteriorUnplaceableBounds>())
{
	AddStaticMeshEvent addStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
	{
		SpawnStaticMeshActor(staticMesh, TEXT("Dungeon/Meshes"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	};
	AddPillarStaticMeshEvent addPillarStaticMeshEvent = [this](uint32_t gridHeight, UStaticMesh* staticMesh, const FTransform& transform)
	{
		SpawnStaticMeshActor(staticMesh, TEXT("Dungeon/Meshes"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	};

	mOnAddFloor = addStaticMeshEvent;
	mOnAddSlope = addStaticMeshEvent;
	mOnAddWall = addStaticMeshEvent;
	mOnAddRoof = addStaticMeshEvent;
	mOnResetPillar = addPillarStaticMeshEvent;
}

CDungeonGeneratorCore::~CDungeonGeneratorCore()
{
	DestroySpawnedActors();
	UnloadStreamLevels();
}

bool CDungeonGeneratorCore::Create(const UDungeonGenerateParameter* parameter)
{
	DUNGEON_GENERATOR_LOG(TEXT("version '%s', license '%s', uuid '%s', commit '%s'"),
		TEXT(DUNGENERATOR_PLUGIN_VERSION_NAME),
		TEXT(JENKINS_LICENSE),
		TEXT(JENKINS_UUID),
		TEXT(JENKINS_GIT_COMMIT)
	);

	// Conversion from UDungeonGenerateParameter to dungeon::GenerateParameter
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the dungeon generation parameters"));
		Clear();
	}

	dungeon::GenerateParameter generateParameter;
	int32 randomSeed = parameter->GetRandomSeed();
	if (parameter->GetRandomSeed() == 0)
		randomSeed = static_cast<int32>(time(nullptr));
	generateParameter.mRandom->SetSeed(randomSeed);
	const_cast<UDungeonGenerateParameter*>(parameter)->SetGeneratedRandomSeed(randomSeed);
	generateParameter.mNumberOfCandidateRooms = parameter->NumberOfCandidateRooms;
	generateParameter.mMinRoomWidth = parameter->RoomWidth.Min;
	generateParameter.mMaxRoomWidth = parameter->RoomWidth.Max;
	generateParameter.mMinRoomDepth = parameter->RoomDepth.Min;
	generateParameter.mMaxRoomDepth = parameter->RoomDepth.Max;
	generateParameter.mMinRoomHeight = parameter->RoomHeight.Min;
	generateParameter.mMaxRoomHeight = parameter->RoomHeight.Max;
	// TODO:UDungeonGenerateParameter
	if (parameter->MergeRooms)
	{
		generateParameter.mHorizontalRoomMargin = 0;
		generateParameter.mVerticalRoomMargin = 0;
		generateParameter.mNumberOfCandidateFloors = 1;
	}
	else if (parameter->RoomMargin <= 1)
	{
		generateParameter.mHorizontalRoomMargin = parameter->RoomMargin;
		generateParameter.mVerticalRoomMargin = 0;
		generateParameter.mNumberOfCandidateFloors = 1;
	}
	else
	{
		generateParameter.mHorizontalRoomMargin = parameter->RoomMargin;
		generateParameter.mVerticalRoomMargin = parameter->VerticalRoomMargin;
		generateParameter.mNumberOfCandidateFloors = static_cast<uint8_t>(std::min(
			static_cast<uint16_t>(generateParameter.mMaxRoomHeight + 2),
			static_cast<uint16_t>(std::numeric_limits<uint8_t>::max())
		));
	}
	if (parameter->GetStartRoom().IsValid())
	{
		generateParameter.mStartRoomSize = parameter->GetStartRoom().GetSize();
	}
	if (parameter->GetGoalRoom().IsValid())
	{
		generateParameter.mGoalRoomSize = parameter->GetStartRoom().GetSize();
	}
	mParameter = parameter;

	mAisleInteriorDecorator.Initialize(mParameter->DungeonInteriorDatabase);
	mSlopeInteriorDecorator.Initialize(mParameter->DungeonInteriorDatabase);
	mRoomInteriorDecorator.Initialize(mParameter->DungeonInteriorDatabase);

	mGenerator = std::make_shared<dungeon::Generator>();
	mGenerator->OnQueryParts([this, parameter](const std::shared_ptr<dungeon::Room>& room)
	{
		CreateImplement_AddRoomAsset(parameter, room);
	});
	if (parameter->GetStartRoom().IsValid())
	{
		mGenerator->OnStartParts([this, parameter](const std::shared_ptr<dungeon::Room>& room)
			{
				CreateImplement_AddRoomAsset(parameter->GetStartRoom(), room, parameter->GetGridSize());
			});
	}
	if (parameter->GetGoalRoom().IsValid())
	{
		mGenerator->OnGoalParts([this, parameter](const std::shared_ptr<dungeon::Room>& room)
			{
				CreateImplement_AddRoomAsset(parameter->GetGoalRoom(), room, parameter->GetGridSize());
			});
	}
	mGenerator->Generate(generateParameter);

	// デバッグ情報を出力
#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
	{
		// TODO:外部からファイル名を与えられるように変更して下さい
		const FString path = FPaths::ProjectSavedDir() + TEXT("/dungeon_diagram.pu");
		mGenerator->DumpRoomDiagram(TCHAR_TO_UTF8(*path));
	}
#endif

	// 生成エラーを確認する
	dungeon::Generator::Error generatorError = mGenerator->GetLastError();
	if (dungeon::Generator::Error::Success != generatorError)
	{
		DUNGEON_GENERATOR_LOG(TEXT("Found error."));

#if WITH_EDITOR
		// デバッグに必要な情報（デバッグ生成パラメータ）を出力する
		/*
		TODO:外部からファイル名を与えられるように変更して下さい
		const FString path = FPaths::ProjectSavedDir() + TEXT("/dungeon_diagram.json");
		*/
		parameter->DumpToJson();
#endif

		return false;
	}
	else
	{
		CreateImplement_AddTerrain();

		DUNGEON_GENERATOR_LOG(TEXT("Done."));

		return true;
	}
}

bool CDungeonGeneratorCore::CreateImplement_AddRoomAsset(const UDungeonGenerateParameter* parameter, const std::shared_ptr<dungeon::Room>& room)
{
	parameter->EachDungeonRoomLocator([this, parameter, &room](const FDungeonRoomLocator& dungeonRoomLocator)
	{
		if (room->GetParts() == dungeon::Room::Parts::Start || room->GetParts() == dungeon::Room::Parts::Goal)
			return;

		if (dungeonRoomLocator.GetDungeonParts() != EDungeonRoomLocatorParts::Any)
		{
			// Match the order of EDungeonRoomParts and dungeon::Room::Parts
			if (Equal(dungeonRoomLocator.GetDungeonParts(), static_cast<EDungeonRoomParts>(room->GetParts())) == false)
				return;
		}

		switch (dungeonRoomLocator.GetWidthCondition())
		{
		case EDungeonRoomSizeCondition::Equal:
			if (room->GetWidth() != dungeonRoomLocator.GetWidth())
				return;
			break;

		case EDungeonRoomSizeCondition::EqualGreater:
			if (room->GetWidth() < dungeonRoomLocator.GetWidth())
				return;
			break;
		}

		switch (dungeonRoomLocator.GetDepthCondition())
		{
		case EDungeonRoomSizeCondition::Equal:
			if (room->GetDepth() != dungeonRoomLocator.GetDepth())
				return;
			break;

		case EDungeonRoomSizeCondition::EqualGreater:
			if (room->GetDepth() < dungeonRoomLocator.GetDepth())
				return;
			break;
		}

		switch (dungeonRoomLocator.GetHeightCondition())
		{
		case EDungeonRoomSizeCondition::Equal:
			if (room->GetHeight() != dungeonRoomLocator.GetHeight())
				return;
			break;

		case EDungeonRoomSizeCondition::EqualGreater:
			if (room->GetHeight() < dungeonRoomLocator.GetHeight())
				return;
			break;
		}

		if (!IsStreamLevelRequested(dungeonRoomLocator.GetLevelPath()))
		{
			// TODO CreateImplement_AddRoomAssetと共通化できるか検討して下さい
			const float gridSize = parameter->GetGridSize();

			check(room->GetWidth() >= dungeonRoomLocator.GetWidth());
			check(room->GetDepth() >= dungeonRoomLocator.GetDepth());
			check(room->GetHeight() >= dungeonRoomLocator.GetHeight());
			room->SetDataSize(dungeonRoomLocator.GetWidth(), dungeonRoomLocator.GetDepth(), dungeonRoomLocator.GetHeight());

			FIntVector min, max;
			room->GetDataBounds(min, max);
			room->SetNoMeshGeneration(!dungeonRoomLocator.IsGenerateRoofMesh(), !dungeonRoomLocator.IsGenerateFloorMesh());
			RequestStreamLevel(dungeonRoomLocator.GetLevelPath(), FVector(min) * gridSize);

			// add un-place able bounds
			{
				const FBox unplacatableBounds(FVector(min) * gridSize, FVector(max) * gridSize);
				mInteriorUnplaceableBounds->Add(unplacatableBounds);
			}
		}
	});

	return true;
}

bool CDungeonGeneratorCore::CreateImplement_AddRoomAsset(const FDungeonRoomRegister& roomRegister, const std::shared_ptr<dungeon::Room>& room, const float gridSize)
{
	if (room == nullptr)
		return false;

	check(room->GetWidth() == roomRegister.GetWidth());
	check(room->GetDepth() == roomRegister.GetDepth());
	check(room->GetHeight() == roomRegister.GetHeight());
	room->SetDataSize(roomRegister.GetWidth(), roomRegister.GetDepth(), roomRegister.GetHeight());

	FIntVector min, max;
	room->GetDataBounds(min, max);
	room->SetNoMeshGeneration(!roomRegister.IsGenerateRoofMesh(), !roomRegister.IsGenerateFloorMesh());
	RequestStreamLevel(roomRegister.GetLevelPath(), FVector(min) * gridSize);

	// add un-place able bounds
	{
		const FBox unplacatableBounds(FVector(min) * gridSize, FVector(max) * gridSize);
		mInteriorUnplaceableBounds->Add(unplacatableBounds);
	}

	return true;
}

void CDungeonGeneratorCore::CreateImplement_AddTerrain()
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set. Please set it."));
		return;
	}

	mAisleInteriorDecorator.ClearDecorationLocation(mInteriorUnplaceableBounds);
	mSlopeInteriorDecorator.ClearDecorationLocation(mInteriorUnplaceableBounds);
	mRoomInteriorDecorator.ClearDecorationLocation(mInteriorUnplaceableBounds);

	std::vector<FSphere> spawnedTorchBounds;
	{
		dungeon::Stopwatch stopwatch;
		mGenerator->GetVoxel()->Each([this, parameter, &spawnedTorchBounds](const FIntVector& location, const dungeon::Grid& grid)
			{
				// Generate floor and slope meshes
				CreateImplement_AddFloorAndSlope(parameter, location);

				// Generate wall mesh
				CreateImplement_AddWall(parameter, location);

				// Generate mesh for pillars and torches
				CreateImplement_AddPillarAndTorch(spawnedTorchBounds, parameter, location);

				// Generate door mesh
				CreateImplement_AddDoor(parameter, location);

				// Generate roof mesh
				CreateImplement_AddRoof(parameter, location);

				return true;
			}
		);
		DUNGEON_GENERATOR_LOG(TEXT("Spawn meshs and actors: %lf seconds"), stopwatch.Lap());
	}
	{
		dungeon::Stopwatch stopwatch;
		mAisleInteriorDecorator.ShuffleDecorationLocation(GetRandom());
		mAisleInteriorDecorator.TrySpawnActors(mWorld.Get(), TArray<FString>({ TEXT("aisle"), TEXT("wall"), TEXT("major") }), GetRandom());
		mAisleInteriorDecorator.TrySpawnActors(mWorld.Get(), TArray<FString>({ TEXT("aisle"), TEXT("wall") }), GetRandom());
		mAisleInteriorDecorator.ClearDecorationLocation();
		DUNGEON_GENERATOR_LOG(TEXT("Spawn aisle interior decorate actors: %lf seconds"), stopwatch.Lap());
	}

	{
		dungeon::Stopwatch stopwatch;
		mSlopeInteriorDecorator.ShuffleDecorationLocation(GetRandom());
		mSlopeInteriorDecorator.TrySpawnActors(mWorld.Get(), TArray<FString>({ TEXT("slope"), TEXT("wall"), TEXT("major") }), GetRandom());
		mSlopeInteriorDecorator.TrySpawnActors(mWorld.Get(), TArray<FString>({ TEXT("slope"), TEXT("wall") }), GetRandom());
		mSlopeInteriorDecorator.ClearDecorationLocation();
		DUNGEON_GENERATOR_LOG(TEXT("Spawn slope interior decorate actors: %lf seconds"), stopwatch.Lap());
	}

	// RoomSensorActorを生成
	{
		dungeon::Stopwatch stopwatch;
		mGenerator->ForEach([this, parameter](const std::shared_ptr<const dungeon::Room>& room)
			{
				const FVector center = room->GetCenter() * parameter->GetGridSize();
				const FVector extent = room->GetExtent() * parameter->GetGridSize();

				// RoomSensor
				ADungeonRoomSensor* roomSensorActor = SpawnRoomSensorActor(
					parameter->GetRoomSensorClass(),
					room->GetIdentifier(),
					center,
					extent,
					static_cast<EDungeonRoomParts>(room->GetParts()),
					static_cast<EDungeonRoomItem>(room->GetItem()),
					room->GetBranchId(),
					room->GetDepthFromStart(),
					mGenerator->GetDeepestDepthFromStart()	//!< TODO:適切な関数名に変えて下さい
				);

				if ((room->GetParts() == dungeon::Room::Parts::Start) || (room->GetParts() == dungeon::Room::Parts::Goal))
				{
					int i = 0;
				}

				// center interior
				{
					FVector C = center + FVector(0, 0, -extent.Z);
					FIntVector location = parameter->ToGrid(C);
					const auto& grid = mGenerator->GetVoxel()->Get(location.X, location.Y, location.Z);
					if (grid.IsKindOfRoomTypeWithoutGate())
					{
						FDungeonInteriorDecorator localRoomCenterInteriorDecorator(
							mRoomInteriorDecorator.GetAsset(),
							mInteriorUnplaceableBounds
						);

						localRoomCenterInteriorDecorator.AddDecorationLocation(C, 0.f);

						{
							TArray<FString> tags = { TEXT("center"), TEXT("major") };
							tags.Add(FString(room->GetPartsName().size(), room->GetPartsName().data()));
							if (IsValid(roomSensorActor))
								tags.Append(roomSensorActor->GetInquireInteriorTags());
							localRoomCenterInteriorDecorator.TrySpawnActors(mWorld.Get(), tags, GetRandom());
						}
						{
							TArray<FString> tags = { TEXT("center") };
							tags.Add(FString(room->GetPartsName().size(), room->GetPartsName().data()));
							if (IsValid(roomSensorActor))
								tags.Append(roomSensorActor->GetInquireInteriorTags());
							localRoomCenterInteriorDecorator.TrySpawnActors(mWorld.Get(), tags, GetRandom());
						}
					}
				}
				// wall interior
				{
					const std::shared_ptr<FDungeonInteriorDecorator> localRoomWallInteriorDecorator
						= mRoomInteriorDecorator.CreateInteriorDecorator(FBox(center - extent, center + extent));
					localRoomWallInteriorDecorator->ShuffleDecorationLocation(GetRandom());
					{
						TArray<FString> tags = { TEXT("wall"), TEXT("major") };
						tags.Add(FString(room->GetPartsName().size(), room->GetPartsName().data()));
						if (IsValid(roomSensorActor))
							tags.Append(roomSensorActor->GetInquireInteriorTags());
						localRoomWallInteriorDecorator->TrySpawnActors(mWorld.Get(), tags, GetRandom());
					}
					{
						TArray<FString> tags = { TEXT("wall") };
						tags.Add(FString(room->GetPartsName().size(), room->GetPartsName().data()));
						if (IsValid(roomSensorActor))
							tags.Append(roomSensorActor->GetInquireInteriorTags());
						localRoomWallInteriorDecorator->TrySpawnActors(mWorld.Get(), tags, GetRandom());
					}
				}

				if ((room->GetParts() == dungeon::Room::Parts::Start) || (room->GetParts() == dungeon::Room::Parts::Goal))
				{
					int i = 0;
				}
			}
		);
		DUNGEON_GENERATOR_LOG(TEXT("Spawn DungeonRoomSensor and room interior decorate actors: %lf seconds"), stopwatch.Lap());
	}

	mRoomInteriorDecorator.ClearDecorationLocation();

	SpawnRecastNavMesh();

#if 0
	//SpawnRecastNavMesh();
	SpawnNavMeshBoundsVolume(parameter);

#else
	if (ANavMeshBoundsVolume* navMeshBoundsVolume = FindActor<ANavMeshBoundsVolume>())
	{
		const FBox& bounding = CalculateBoundingBox();
		const FVector& boundingCenter = bounding.GetCenter();
		const FVector& boundingExtent = bounding.GetExtent();

		if (USceneComponent* rootComponent = navMeshBoundsVolume->GetRootComponent())
		{
			const EComponentMobility::Type mobility = rootComponent->Mobility;
			rootComponent->SetMobility(EComponentMobility::Stationary);

			navMeshBoundsVolume->SetActorLocation(boundingCenter);
			navMeshBoundsVolume->SetActorScale3D(FVector::OneVector);

#if WITH_EDITOR
			// ブラシビルダーを生成 (UnrealEd)
			if (UCubeBuilder* cubeBuilder = NewObject<UCubeBuilder>())
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
				DUNGEON_GENERATOR_ERROR(TEXT("CubeBuilder generation failed in CDungeonGeneratorCore"));
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
#endif
}

void CDungeonGeneratorCore::CreateImplement_AddWall(const UDungeonGenerateParameter* parameter, const FIntVector& location)
{
	/*
	壁のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面（北側の壁）になっています。
	*/
	if (mOnAddWall)
	{
		const size_t gridIndex = mGenerator->GetVoxel()->Index(location);
		const dungeon::Grid& currentGrid = mGenerator->GetVoxel()->Get(gridIndex);
		const dungeon::Grid& underGrid = mGenerator->GetVoxel()->Get(location.X, location.Y, location.Z - 1);
		const float gridSize = parameter->GetGridSize();
		const float halfGridSize = gridSize * 0.5f;
		const FVector halfOffset(halfGridSize, halfGridSize, 0);
		const FVector position = parameter->ToWorld(location);
		const FVector centerPosition = position + halfOffset;

		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		if (currentGrid.IsKindOfRoomType())
			dungeonMeshSetDatabase = parameter->GetDungeonRoomPartsDatabase();
		else
			dungeonMeshSetDatabase = parameter->GetDungeonAislePartsDatabase();

		if (const FDungeonMeshParts* parts = parameter->SelectWallParts(dungeonMeshSetDatabase, gridIndex, currentGrid, GetRandom()))
		{
			if (currentGrid.CanBuildWall(mGenerator->GetVoxel()->Get(location.X, location.Y - 1, location.Z), underGrid, dungeon::Direction::North, parameter->MergeRooms))
			{
				// 北側の壁
				FVector wallPosition = centerPosition;
				wallPosition.Y -= halfGridSize;
				mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 0.f));

				if (currentGrid.IsKindOfAisleType())
					mAisleInteriorDecorator.AddDecorationLocation(wallPosition, 0.f);
				else if (currentGrid.GetType() == dungeon::Grid::Type::Deck)
					mRoomInteriorDecorator.AddDecorationLocation(wallPosition, 0.f);
			}
			if (currentGrid.CanBuildWall(mGenerator->GetVoxel()->Get(location.X, location.Y + 1, location.Z), underGrid, dungeon::Direction::South, parameter->MergeRooms))
			{
				// 南側の壁
				FVector wallPosition = centerPosition;
				wallPosition.Y += halfGridSize;
				mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 180.f));

				if (currentGrid.IsKindOfAisleType())
					mAisleInteriorDecorator.AddDecorationLocation(wallPosition, 180.f);
				else if (currentGrid.GetType() == dungeon::Grid::Type::Deck)
					mRoomInteriorDecorator.AddDecorationLocation(wallPosition, 180.f);
			}
			if (currentGrid.CanBuildWall(mGenerator->GetVoxel()->Get(location.X + 1, location.Y, location.Z), underGrid, dungeon::Direction::East, parameter->MergeRooms))
			{
				// 東側の壁
				FVector wallPosition = centerPosition;
				wallPosition.X += halfGridSize;
				mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 90.f));

				if (currentGrid.IsKindOfAisleType())
					mAisleInteriorDecorator.AddDecorationLocation(wallPosition, 90.f);
				else if (currentGrid.GetType() == dungeon::Grid::Type::Deck)
					mRoomInteriorDecorator.AddDecorationLocation(wallPosition, 90.f);
			}
			if (currentGrid.CanBuildWall(mGenerator->GetVoxel()->Get(location.X - 1, location.Y, location.Z), underGrid, dungeon::Direction::West, parameter->MergeRooms))
			{
				// 西側の壁
				FVector wallPosition = centerPosition;
				wallPosition.X -= halfGridSize;
				mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, -90.f));

				if (currentGrid.IsKindOfAisleType())
					mAisleInteriorDecorator.AddDecorationLocation(wallPosition, -90.f);
				else if (currentGrid.GetType() == dungeon::Grid::Type::Deck)
					mRoomInteriorDecorator.AddDecorationLocation(wallPosition, -90.f);
			}
		}
	}
}

void CDungeonGeneratorCore::CreateImplement_AddDoor(const UDungeonGenerateParameter* parameter, const FIntVector& location)
{
	const size_t gridIndex = mGenerator->GetVoxel()->Index(location);
	const dungeon::Grid& grid = mGenerator->GetVoxel()->Get(gridIndex);
	const float gridSize = parameter->GetGridSize();
	const float halfGridSize = gridSize * 0.5f;
	const FVector halfOffset(halfGridSize, halfGridSize, 0);
	const FVector position = parameter->ToWorld(location);
	const FVector centerPosition = position + halfOffset;

	if (const FDungeonDoorActorParts* parts = parameter->SelectDoorParts(gridIndex, grid, GetRandom()))
	{
		const EDungeonRoomProps props = static_cast<EDungeonRoomProps>(grid.GetProps());

		if (grid.CanBuildGate(mGenerator->GetVoxel()->Get(location.X, location.Y - 1, location.Z), dungeon::Direction::North))
		{
			// 北側の扉
			FVector doorPosition = position;
			doorPosition.X += parameter->GridSize * 0.5f;
			SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 0.f), props);
		}
		if (grid.CanBuildGate(mGenerator->GetVoxel()->Get(location.X, location.Y + 1, location.Z), dungeon::Direction::South))
		{
			// 南側の扉
			FVector doorPosition = position;
			doorPosition.X += parameter->GridSize * 0.5f;
			doorPosition.Y += parameter->GridSize;
			SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 180.f), props);
		}
		if (grid.CanBuildGate(mGenerator->GetVoxel()->Get(location.X + 1, location.Y, location.Z), dungeon::Direction::East))
		{
			// 東側の扉
			FVector doorPosition = position;
			doorPosition.X += parameter->GridSize;
			doorPosition.Y += parameter->GridSize * 0.5f;
			SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 90.f), props);
		}
		if (grid.CanBuildGate(mGenerator->GetVoxel()->Get(location.X - 1, location.Y, location.Z), dungeon::Direction::West))
		{
			// 西側の扉
			FVector doorPosition = position;
			doorPosition.Y += parameter->GridSize * 0.5f;
			SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, -90.f), props);
		}
	}
}

void CDungeonGeneratorCore::CreateImplement_AddFloorAndSlope(const UDungeonGenerateParameter* parameter, const FIntVector& location)
{
	const size_t gridIndex = mGenerator->GetVoxel()->Index(location);
	const dungeon::Grid& grid = mGenerator->GetVoxel()->Get(gridIndex);
	const float gridSize = parameter->GetGridSize();
	const float halfGridSize = gridSize * 0.5f;
	const FVector halfOffset(halfGridSize, halfGridSize, 0);
	const FVector position = parameter->ToWorld(location);
	const FVector centerPosition = position + halfOffset;

	if (mOnAddSlope && grid.CanBuildSlope())
	{
		/*
		スロープのメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		if (const FDungeonMeshParts* parts = parameter->SelectSlopeParts(gridIndex, grid, GetRandom()))
		{
			mOnAddSlope(parts->StaticMesh, parts->CalculateWorldTransform(centerPosition, grid.GetDirection()));
		}
	}
	else if (mOnAddFloor && grid.CanBuildFloor(mGenerator->GetVoxel()->Get(location.X, location.Y, location.Z - 1), true))
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		if (grid.IsKindOfRoomType())
			dungeonMeshSetDatabase = parameter->GetDungeonRoomPartsDatabase();
		else
			dungeonMeshSetDatabase = parameter->GetDungeonAislePartsDatabase();

		if (const FDungeonMeshParts* parts = parameter->SelectFloorParts(dungeonMeshSetDatabase, gridIndex, grid, GetRandom()))
		{
			/*
			床のメッシュを生成
			メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
			*/
			mOnAddFloor(parts->StaticMesh, parts->CalculateWorldTransform(centerPosition, grid.GetDirection()));
		}
	}
}

void CDungeonGeneratorCore::CreateImplement_AddPillarAndTorch(std::vector<FSphere>& spawnedTorchBounds, const UDungeonGenerateParameter* parameter, const FIntVector& location)
{
	/*
	柱のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
	*/
	if (mOnResetPillar)
	{
		const size_t gridIndex = mGenerator->GetVoxel()->Index(location);
		const dungeon::Grid& grid = mGenerator->GetVoxel()->Get(gridIndex);
		const float gridSize = parameter->GetGridSize();
		const float halfGridSize = gridSize * 0.5f;
		const FVector halfOffset(halfGridSize, halfGridSize, 0);
		const FVector position = parameter->ToWorld(location);
		const FVector centerPosition = position + halfOffset;

		FVector wallVector(0.f);
		uint8_t wallCount = 0;
		uint8_t deckCount = 0;
		bool onFloor = false;
		uint32_t pillarGridHeight = 1;
		for (int_fast8_t dy = -1; dy <= 0; ++dy)
		{
			for (int_fast8_t dx = -1; dx <= 0; ++dx)
			{
				// Check the number of walls.
				const auto& result = mGenerator->GetVoxel()->Get(location.X + dx, location.Y + dy, location.Z);
				if (grid.CanBuildPillar(result))
				{
					wallVector += FVector(static_cast<float>(dx) + 0.5f, static_cast<float>(dy) + 0.5f, 0.f);
					++wallCount;
				}
				// Check the number of floors.
				if (
					(result.GetType() == dungeon::Grid::Type::Deck) ||
					(result.GetType() == dungeon::Grid::Type::Gate) ||
					(result.GetType() == dungeon::Grid::Type::Aisle) ||
					(result.GetType() == dungeon::Grid::Type::Slope) /* || TODO:スロープの上部分を判別する方法を検討してください
					(result.GetType() == dungeon::Grid::Type::Atrium)*/)
				{
					++deckCount;
				}

				// 床を調べます　TODO:dungeon::Grid::Type::Deckで調べられるか調べてください
				const auto& baseFloorGrid = mGenerator->GetVoxel()->Get(location.X + dx, location.Y + dy, location.Z);
				const auto& underFloorGrid = mGenerator->GetVoxel()->Get(location.X + dx, location.Y + dy, location.Z - 1);
				if (baseFloorGrid.IsKindOfSlopeType() || baseFloorGrid.CanBuildFloor(underFloorGrid, false))
				{
					onFloor = true;

					// 天井の高さを調べます
					uint32_t gridHeight = 1;
					while (true)
					{
						const auto& roofGrid = mGenerator->GetVoxel()->Get(location.X + dx, location.Y + dy, location.Z + gridHeight);
						if (roofGrid.GetType() == dungeon::Grid::Type::OutOfBounds)
							break;
						if (!grid.CanBuildRoof(roofGrid, false))
							break;
						++gridHeight;
					}
					if (pillarGridHeight < gridHeight)
						pillarGridHeight = gridHeight;
				}
			}
		}

		if (onFloor && (1 <= wallCount && wallCount <= 3))
		{
			wallVector.Normalize();
			FRotator wallRotation(wallVector.Rotation());
			//wallRotation.Yaw = static_cast<double>(static_cast<int32_t>(std::round(wallRotation.Yaw)) / 90) * 90.0;
			const FTransform transform(wallRotation, position);

			if (const FDungeonMeshParts* parts = parameter->SelectPillarParts(gridIndex, grid, GetRandom()))
			{
				mOnResetPillar(pillarGridHeight, parts->StaticMesh, parts->CalculateWorldTransform(transform));
			}

			// Need wall torch?
			if (deckCount >= 2)
			{
				if (const FDungeonActorParts* parts = parameter->SelectTorchParts(gridIndex, grid, GetRandom()))
				{
#if 0
					const FTransform worldTransform = parts->RelativeTransform * transform;
#else
					const FVector rotatedLocation = transform.Rotator().RotateVector(parts->RelativeTransform.GetLocation());
					const FTransform worldTransform(
						transform.Rotator() + parts->RelativeTransform.Rotator(),
						transform.GetLocation() + rotatedLocation,
						transform.GetScale3D() * parts->RelativeTransform.GetScale3D()
					);
#endif
					SpawnTorchActor(spawnedTorchBounds, wallVector, parts->ActorClass, TEXT("Dungeon/Actors"), worldTransform);
				}
			}
		}
	}
}

void CDungeonGeneratorCore::CreateImplement_AddRoof(const UDungeonGenerateParameter* parameter, const FIntVector& location)
{
	if (mOnAddRoof)
	{
		const size_t gridIndex = mGenerator->GetVoxel()->Index(location);
		const dungeon::Grid& grid = mGenerator->GetVoxel()->Get(gridIndex);
		const float gridSize = parameter->GetGridSize();
		const float halfGridSize = gridSize * 0.5f;
		const FVector halfOffset(halfGridSize, halfGridSize, 0);
		const FVector position = parameter->ToWorld(location);
		const FVector centerPosition = position + halfOffset;

		if (grid.CanBuildRoof(mGenerator->GetVoxel()->Get(location.X, location.Y, location.Z + 1), true))
		{
			const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
			if (grid.IsKindOfRoomType())
				dungeonMeshSetDatabase = parameter->GetDungeonRoomPartsDatabase();
			else
				dungeonMeshSetDatabase = parameter->GetDungeonAislePartsDatabase();

			/*
			壁のメッシュを生成
			メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
			*/
			const FTransform transform(centerPosition);
			if (const FDungeonMeshPartsWithDirection* parts = parameter->SelectRoofParts(dungeonMeshSetDatabase, gridIndex, grid, GetRandom()))
			{
				mOnAddRoof(
					parts->StaticMesh,
					parts->CalculateWorldTransform(GetRandom(), transform)
				);
			}
		}
	}
}

void CDungeonGeneratorCore::SpawnRecastNavMesh()
{
	if (ARecastNavMesh* navMeshBoundsVolume = FindActor<ARecastNavMesh>())
	{
		const auto mode = navMeshBoundsVolume->GetRuntimeGenerationMode();
		if (mode != ERuntimeGenerationType::Dynamic && mode != ERuntimeGenerationType::DynamicModifiersOnly)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Set RuntimeGenerationMode of RecastNavMesh to Dynamic"));
		}
	}
}

void CDungeonGeneratorCore::Clear()
{
	mGenerator.reset();
	mAisleInteriorDecorator.Finalize();
	mSlopeInteriorDecorator.Finalize();
	mRoomInteriorDecorator.Finalize();
	mParameter = nullptr;
}

FTransform CDungeonGeneratorCore::GetStartTransform() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		return FTransform(*mGenerator->GetStartPoint() * parameter->GetGridSize());
	}
	return FTransform::Identity;
}

FTransform CDungeonGeneratorCore::GetGoalTransform() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		return FTransform(*mGenerator->GetGoalPoint() * parameter->GetGridSize());
	}
	return FTransform::Identity;
}

FVector CDungeonGeneratorCore::GetStartLocation() const
{
	return GetStartTransform().GetLocation();
}

FVector CDungeonGeneratorCore::GetGoalLocation() const
{
	return GetGoalTransform().GetLocation();
}

/**
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
static inline FBox ToWorldBoundingBox(const UDungeonGenerateParameter* parameter, const std::shared_ptr<const dungeon::Room>& room)
{
	check(parameter);
	const FVector min = parameter->ToWorld(room->GetLeft(), room->GetTop(), room->GetBackground());
	const FVector max = parameter->ToWorld(room->GetRight(), room->GetBottom(), room->GetForeground());
	return FBox(min, max);
}

FBox CDungeonGeneratorCore::CalculateBoundingBox() const
{
	if (mGenerator)
	{
		const UDungeonGenerateParameter* parameter = mParameter.Get();
		if (IsValid(parameter))
		{
			FBox boundingBox(EForceInit::ForceInitToZero);
			mGenerator->ForEach([&boundingBox, parameter](const std::shared_ptr<const dungeon::Room>& room)
				{
					boundingBox += ToWorldBoundingBox(parameter, room);
				}
			);
			boundingBox.Min.Z -= parameter->GetGridSize();
			boundingBox.Max.Z += parameter->GetGridSize();
			return boundingBox;
		}
	}

	return FBox(EForceInit::ForceInitToZero);
}

void CDungeonGeneratorCore::MovePlayerStart()
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) == false)
		return;

	if (parameter->IsMovePlayerStartToStartingPoint() == false)
		return;

	/*
	if (parameter->GetStartRoom().IsValid() == true)
		return;
	*/

	size_t totalPlayerStart = 0;
	EachActors<APlayerStart>([&totalPlayerStart](APlayerStart* playerStart)
		{
			// APlayerStartはコリジョンが無効になっているので、GetSimpleCollisionCylinderを利用する事ができない
			if (USceneComponent* rootComponent = playerStart->GetRootComponent())
				++totalPlayerStart;
		}
	);
	if (totalPlayerStart == 0)
	{
		DUNGEON_GENERATOR_WARNING(TEXT("PlayerStart could not be found from the level"));
		return;
	}

	// Calculate the position to shift APlayerStart
	const double placementRadius = totalPlayerStart > 1 ? parameter->GetGridSize() : 0;
	const double placementAngle = (3.1415926535897932384626433832795 * 2.) / static_cast<double>(totalPlayerStart);

	size_t index = 0;
	EachActors<APlayerStart>([this, parameter, placementRadius, placementAngle, &index](APlayerStart* playerStart)
		{
			// "Play from Here" PlayerStart, if we find one while in PIE mode
			if (Cast<APlayerStartPIE>(playerStart))
				return;

			// APlayerStart cannot use GetSimpleCollisionCylinder because collision is disabled.
			if (USceneComponent* rootComponent = playerStart->GetRootComponent())
			{
				// Create a small margin to avoid grounding.
				static constexpr float heightMargin = 10.f;

				// Temporarily set EComponentMobility to Movable
				const EComponentMobility::Type mobility = rootComponent->Mobility;
				rootComponent->SetMobility(EComponentMobility::Movable);
				{
					float cylinderRadius, cylinderHalfHeight;
					rootComponent->CalcBoundingCylinder(cylinderRadius, cylinderHalfHeight);

					// If there are multiple APlayerStart, shift APlayerStart from the center of the room
					FVector location = GetStartLocation();
					location.X += placementRadius * std::cos(placementAngle * static_cast<double>(index));
					location.Y += placementRadius * std::sin(placementAngle * static_cast<double>(index));

					// Grounding the APlayerStart
					FHitResult hitResult;
					if (playerStart->GetWorld()->LineTraceSingleByChannel(hitResult, location + FVector(0, 0, parameter->GetGridSize()), location, ECollisionChannel::ECC_Pawn))
					{
						location = hitResult.ImpactPoint;
					}
					location.Z += cylinderHalfHeight + heightMargin;
					playerStart->SetActorLocation(location);

#if WITH_EDITOR
					FCollisionShape collisionShape;
#if UE_VERSION_OLDER_THAN(5, 0, 0)
					collisionShape.SetBox(FVector(cylinderRadius, cylinderRadius, cylinderHalfHeight));
#else
					collisionShape.SetBox(FVector3f(cylinderRadius, cylinderRadius, cylinderHalfHeight));
#endif
					if (playerStart->GetWorld()->OverlapBlockingTestByChannel(location, playerStart->GetActorQuat(), ECollisionChannel::ECC_Pawn, collisionShape))
					{
						DUNGEON_GENERATOR_ERROR(TEXT("PlayerStart(%s) is in contact with something"), *playerStart->GetName());
					}
#endif
				}
				// Undo EComponentMobility
				rootComponent->SetMobility(mobility);

				++index;
			}
#if WITH_EDITOR
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Set RootComponent of PlayerStart(%s)"), *playerStart->GetName());
			}
#endif
		}
	);
}

////////////////////////////////////////////////////////////////////////////////
AActor* CDungeonGeneratorCore::SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AActor* actor = SpawnActorDeferred<AActor>(actorClass, folderPath, transform, spawnActorCollisionHandlingMethod);
	//ESpawnActorCollisionHandlingMethod::AlwaysSpawn
	//ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding
	//ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	if (!IsValid(actor))
		return nullptr;
	actor->FinishSpawning(transform);
	return actor;
}

AStaticMeshActor* CDungeonGeneratorCore::SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AStaticMeshActor* actor = SpawnActorDeferred<AStaticMeshActor>(AStaticMeshActor::StaticClass(), folderPath, transform, spawnActorCollisionHandlingMethod);
	if (!IsValid(actor))
		return nullptr;

	UStaticMeshComponent* mesh = actor->GetStaticMeshComponent();
	if (IsValid(mesh))
		mesh->SetStaticMesh(staticMesh);

	actor->FinishSpawning(transform);

	return actor;
}

void CDungeonGeneratorCore::SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const
{
	AActor* actor = SpawnActor(actorClass, TEXT("Dungeon/Actors"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		FVector location = actor->GetActorLocation();
		location.Z += actor->GetSimpleCollisionHalfHeight();
		actor->SetActorLocation(location);
	}
}

void CDungeonGeneratorCore::SpawnDoorActor(UClass* actorClass, const FTransform& transform, EDungeonRoomProps props) const
{
	ADungeonDoor* actor = SpawnActorDeferred<ADungeonDoor>(actorClass, TEXT("Dungeon/Actors"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		actor->Initialize(props);

		if (mOnResetDoor)
		{
			mOnResetDoor(actor, props);
		}
	}
	if (IsValid(actor))
	{
		actor->FinishSpawning(transform);
	}
}

AActor* CDungeonGeneratorCore::SpawnTorchActor(std::vector<FSphere>& spawnedTorchBounds, const FVector& wallNormal, UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AActor* actor = SpawnActor(actorClass, folderPath, transform, spawnActorCollisionHandlingMethod);
	if (!IsValid(actor))
		return nullptr;
#if 0
	return actor;
#else
	FSphere localLightBounds(EForceInit::ForceInit);
	{
		TArray<ULocalLightComponent*> localLightComponents;
		actor->GetComponents(localLightComponents);
		for (const auto& localLightComponent : localLightComponents)
		{
			localLightBounds += localLightComponent->GetBoundingSphere();
		}
	}

	// Since there is no light influence behind the wall,
	// move the influence sphere about half of its radius away from the wall.
	{
		localLightBounds.W *= 0.5;
		localLightBounds.W *= 0.5;
		localLightBounds.Center += wallNormal * localLightBounds.W;
	}

	const auto i = std::find_if(spawnedTorchBounds.begin(), spawnedTorchBounds.end(), [&localLightBounds](const FSphere& bounds)
		{
			return bounds.Intersects(localLightBounds);
		}
	);
	if (i == spawnedTorchBounds.end())
	{
		// Registers a light range of torch actor
		spawnedTorchBounds.emplace_back(localLightBounds);

		return actor;
	}
	else
	{
		actor->Destroy();
		return nullptr;
	}
#endif
}

ADungeonRoomSensor* CDungeonGeneratorCore::SpawnRoomSensorActor(
	UClass* actorClass,
	const dungeon::Identifier& identifier,
	const FVector& center,
	const FVector& extent,
	EDungeonRoomParts parts,
	EDungeonRoomItem item,
	uint8 branchId,
	const uint8 depthFromStart,
	const uint8 deepestDepthFromStart) const
{
	const FTransform transform(center);
	ADungeonRoomSensor* actor = SpawnActorDeferred<ADungeonRoomSensor>(actorClass, TEXT("Dungeon/Sensors"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		actor->Initialize(GetRandom(), identifier.Get(), extent, parts, item, branchId, depthFromStart, deepestDepthFromStart);
		actor->FinishSpawning(transform);
	}
	return actor;
};

void CDungeonGeneratorCore::DestroySpawnedActors() const
{
	DestroySpawnedActors(mWorld.Get());
}

void CDungeonGeneratorCore::DestroySpawnedActors(UWorld* world)
{
	if (!IsValid(world))
		return;

	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsWithTag(world, DungeonGeneratorTag, actors);
	for (AActor* actor : actors)
	{
		if (!IsValid(actor))
			continue;

		if (ADungeonRoomSensor* dungeonRoomSensor = Cast<ADungeonRoomSensor>(actor))
		{
			dungeonRoomSensor->Finalize();
		}
		actor->Destroy();
	}

#if WITH_EDITOR
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	{
		TArray<FFolder> deleteFolders;
		FActorFolders::Get().ForEachFolder(*world, [&deleteFolders](const FFolder& folder)
			{
				const FName pathName = folder.GetPath();
				if (pathName.ToString().StartsWith("Dungeon"))
				{
					deleteFolders.Add(folder);
				}
				return true;
			});
		for (FFolder& folder : deleteFolders)
		{
			FActorFolders::Get().DeleteFolder(*world, folder);
		}
	}
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////
UTexture2D* CDungeonGeneratorCore::GenerateMiniMapTextureWithSize(uint32_t& worldToTextureScale, uint32_t textureWidthHeight, uint32_t currentLevel) const
{
	if (mGenerator->GetLastError() != dungeon::Generator::Error::Success)
		return nullptr;

	std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel();
	if (voxel == nullptr)
		return nullptr;

	uint32_t length = 1;
	if (length < voxel->GetWidth())
		length = voxel->GetWidth();
	if (length < voxel->GetDepth())
		length = voxel->GetDepth();
	worldToTextureScale = static_cast<uint32_t>(static_cast<float>(textureWidthHeight) / static_cast<float>(length));

	return GenerateMiniMapTexture(worldToTextureScale, textureWidthHeight, currentLevel);
}

UTexture2D* CDungeonGeneratorCore::GenerateMiniMapTextureWithScale(uint32_t& worldToTextureScale, uint32_t dotScale, uint32_t currentLevel) const
{
	if (mGenerator->GetLastError() != dungeon::Generator::Error::Success)
		return nullptr;

	std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel();
	if (voxel == nullptr)
		return nullptr;

	uint32_t length = 1;
	if (length < voxel->GetWidth())
		length = voxel->GetWidth();
	if (length < voxel->GetDepth())
		length = voxel->GetDepth();
	const uint32_t textureWidthHeight = length * dotScale;
	worldToTextureScale = static_cast<uint32_t>(static_cast<float>(textureWidthHeight) / static_cast<float>(length));

	return GenerateMiniMapTexture(worldToTextureScale, textureWidthHeight, currentLevel);
}

UTexture2D* CDungeonGeneratorCore::GenerateMiniMapTexture(uint32_t worldToTextureScale, uint32_t textureWidthHeight, uint32_t currentLevel) const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
		return nullptr;

	const size_t totalBufferSize = textureWidthHeight * textureWidthHeight;
	auto pixels = std::make_unique<uint8_t[]>(totalBufferSize);
	std::memset(pixels.get(), 0x00, totalBufferSize);

	std::shared_ptr<dungeon::Voxel> voxel = mGenerator->GetVoxel();

	if (currentLevel > voxel->GetHeight() - 1)
		currentLevel = voxel->GetHeight() - 1;

	auto rect = [&pixels, textureWidthHeight, worldToTextureScale](const uint32_t x, const uint32_t y, const uint8_t color) -> void
	{
		const uint32_t px = x * worldToTextureScale;
		const uint32_t py = y * worldToTextureScale;
		for (uint32_t oy = py; oy < py + worldToTextureScale; ++oy)
		{
			for (uint32_t ox = px; ox < px + worldToTextureScale; ++ox)
				pixels[textureWidthHeight * oy + ox] = color;
		}
	};

	auto line = [&pixels, textureWidthHeight, worldToTextureScale](const uint32_t x, const uint32_t y, const dungeon::Direction::Index dir, const uint8_t color) -> void
	{
		uint32_t px = x * worldToTextureScale;
		uint32_t py = y * worldToTextureScale;
		switch (dir)
		{
		case dungeon::Direction::North:
			for (uint32_t ox = px; ox < px + worldToTextureScale; ++ox)
				pixels[textureWidthHeight * py + ox] = color;
			break;

		case dungeon::Direction::South:
			py += worldToTextureScale;
			for (uint32_t ox = px; ox < px + worldToTextureScale; ++ox)
				pixels[textureWidthHeight * py + ox] = color;
			break;

		case dungeon::Direction::East:
			px += worldToTextureScale;
			for (uint32_t oy = py; oy < py + worldToTextureScale; ++oy)
				pixels[textureWidthHeight * oy + px] = color;
			break;

		case dungeon::Direction::West:
			for (uint32_t oy = py; oy < py + worldToTextureScale; ++oy)
				pixels[textureWidthHeight * oy + px] = color;
			break;

		default:
			break;
		}
	};

	// 下の階層から描画する
	const float paintRatio = 1.f / std::max(1.f, static_cast<float>(currentLevel));
	for (uint32_t z = 0; z <= currentLevel; ++z)
	{
		constexpr float floorColorRange = 0.6f;
		constexpr float wallColorRange = 0.6f;

		float floorRatio, wallRatio;
		if (currentLevel == 0)
		{
			floorRatio = 0.f + 1.f * paintRatio * floorColorRange;
			wallRatio = (1.f - wallColorRange) + 1.f * paintRatio * wallColorRange;
		}
		else
		{
			floorRatio = 0.f + static_cast<float>(z) * paintRatio * floorColorRange;
			wallRatio = (1.f - wallColorRange) + static_cast<float>(z) * paintRatio * wallColorRange;
		}
		const uint8_t floorColor = static_cast<uint8_t>(255.f * floorRatio);
		const uint8_t wallColor = static_cast<uint8_t>(255.f * wallRatio);

		for (uint32_t y = 0; y < voxel->GetDepth(); ++y)
		{
			for (uint32_t x = 0; x < voxel->GetWidth(); ++x)
			{
				const auto& grid = voxel->Get(x, y, z);
				// slope
				if (grid.IsKindOfSlopeType())
				{
					rect(x, y, floorColor);
				}
				// floor
				else if (grid.CanBuildFloor(voxel->Get(x, y, z - 1), false))
				{
					rect(x, y, floorColor);
				}

				// wall
				static const dungeon::Grid underGrid(dungeon::Grid::Type::Empty);
				if (grid.CanBuildWall(voxel->Get(x, y - 1, z), underGrid, dungeon::Direction::North, parameter->MergeRooms))
				{
					line(x, y, dungeon::Direction::North, wallColor);
				}
				if (grid.CanBuildWall(voxel->Get(x, y + 1, z), underGrid, dungeon::Direction::South, parameter->MergeRooms))
				{
					line(x, y, dungeon::Direction::South, wallColor);
				}
				if (grid.CanBuildWall(voxel->Get(x + 1, y, z), underGrid, dungeon::Direction::East, parameter->MergeRooms))
				{
					line(x, y, dungeon::Direction::East, wallColor);
				}
				if (grid.CanBuildWall(voxel->Get(x - 1, y, z), underGrid, dungeon::Direction::West, parameter->MergeRooms))
				{
					line(x, y, dungeon::Direction::West, wallColor);
				}
			}
		}
	}

	UTexture2D* generateTexture = UTexture2D::CreateTransient(textureWidthHeight, textureWidthHeight, PF_G8);
	generateTexture->Filter = TextureFilter::TF_Nearest;
	{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		FTexture2DMipMap& mips = generateTexture->PlatformData->Mips[0];
#else
		FTexture2DMipMap& mips = generateTexture->GetPlatformData()->Mips[0];
#endif
		auto lockedBulkData = mips.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(lockedBulkData, pixels.get(), totalBufferSize);
		mips.BulkData.Unlock();
	}
	generateTexture->AddToRoot();
#if WITH_EDITOR
	generateTexture->Source.Init(textureWidthHeight, textureWidthHeight, 1, 1, ETextureSourceFormat::TSF_G8, pixels.get());
#endif
	generateTexture->UpdateResource();

	return generateTexture;
}

uint32_t CDungeonGeneratorCore::CalculateCRC32() const noexcept
{
	return mGenerator ? mGenerator->CalculateCRC32() : 0;
}

std::shared_ptr<const dungeon::Generator> CDungeonGeneratorCore::GetGenerator() const
{
	return mGenerator;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CDungeonGeneratorCore::IsStreamLevelRequested(const FSoftObjectPath& levelPath) const
{
	const auto requestStreamLevel = std::find_if(mRequestLoadStreamLevels.begin(), mRequestLoadStreamLevels.end(), [&levelPath](const LoadStreamLevelParameter& requestStreamLevel)
		{
			return requestStreamLevel.mPath == levelPath;
		}
	);
	return requestStreamLevel != mRequestLoadStreamLevels.end();
}

void CDungeonGeneratorCore::RequestStreamLevel(const FSoftObjectPath& levelPath, const FVector& levelLocation)
{
	mRequestLoadStreamLevels.emplace_back(levelPath, levelLocation);
}

void CDungeonGeneratorCore::FlushLoadStreamLevels()
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		while (!mRequestLoadStreamLevels.empty())
		{
			AsyncLoadStreamLevels(true);
		}
		world->UpdateLevelStreaming();
	}
}

void CDungeonGeneratorCore::AsyncLoadStreamLevels(const bool bShouldBlockOnLoad)
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		if (!mRequestLoadStreamLevels.empty())
		{
			LoadStreamLevelParameter requestStreamLevel = mRequestLoadStreamLevels.front();
			mRequestLoadStreamLevels.pop_front();

			bool bSuccess = false;
#if UE_VERSION_NEWER_THAN(5, 1, 0)
			const FTransform transform(FRotator::ZeroRotator, requestStreamLevel.mLocation);
			ULevelStreamingDynamic::FLoadLevelInstanceParams parameter(world, requestStreamLevel.mPath.GetLongPackageName(), transform);
			parameter.OptionalLevelStreamingClass = UDungeonLevelStreamingDynamic::StaticClass();
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(parameter, bSuccess);
#elif UE_VERSION_NEWER_THAN(5, 0, 0)
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(
				world,
				requestStreamLevel.mPath.GetLongPackageName(),
				requestStreamLevel.mLocation,
				FRotator::ZeroRotator,
				bSuccess,
				TEXT(""),
				UDungeonLevelStreamingDynamic::StaticClass(),
				false);
#else
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(
				world,
				requestStreamLevel.mPath.GetLongPackageName(),
				requestStreamLevel.mLocation,
				FRotator::ZeroRotator,
				bSuccess);
#endif
			if (bSuccess && IsValid(levelStreaming))
			{
				mLoadedStreamLevels.Add(levelStreaming);

				if (bShouldBlockOnLoad)
				{
					levelStreaming->bShouldBlockOnLoad = true;
					world->UpdateStreamingLevelShouldBeConsidered(levelStreaming);
#if UE_VERSION_NEWER_THAN(5, 2, 0)
					if (levelStreaming->GetLevelStreamingState() == ELevelStreamingState::Removed)
#endif
					{
						world->AddStreamingLevel(levelStreaming);
					}
				}

				DUNGEON_GENERATOR_LOG(TEXT("Load Level (%s)"), *requestStreamLevel.mPath.GetLongPackageName());
			}
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Failed to Load Level (%s)"), *requestStreamLevel.mPath.GetLongPackageName());
			}
		}
	}
}

#if WITH_EDITOR
void CDungeonGeneratorCore::SyncLoadStreamLevels()
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		TArray<AActor*> moveActors;
		int32 index = 0;

		for (const auto& requestStreamLevel : mRequestLoadStreamLevels)
		{
			bool bSuccess = false;

#if UE_VERSION_NEWER_THAN(5, 1, 0)
			const FTransform transform(FRotator::ZeroRotator, requestStreamLevel.mLocation);
			ULevelStreamingDynamic::FLoadLevelInstanceParams parameter(world, requestStreamLevel.mPath.GetLongPackageName(), transform);
			parameter.OptionalLevelStreamingClass = UDungeonLevelStreamingDynamic::StaticClass();
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(parameter, bSuccess);
#elif UE_VERSION_NEWER_THAN(5, 0, 0)
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(
				world,
				requestStreamLevel.mPath.GetLongPackageName(),
				requestStreamLevel.mLocation,
				FRotator::ZeroRotator,
				bSuccess,
				TEXT(""),
				UDungeonLevelStreamingDynamic::StaticClass(),
				false);
#else
			ULevelStreamingDynamic* levelStreaming = ULevelStreamingDynamic::LoadLevelInstance(
				world,
				requestStreamLevel.mPath.GetLongPackageName(),
				requestStreamLevel.mLocation,
				FRotator::ZeroRotator,
				bSuccess);
#endif
			if (bSuccess && IsValid(levelStreaming))
			{
				levelStreaming->bShouldBlockOnLoad = true;
				world->FlushLevelStreaming();
				//world->UpdateLevelStreaming();

				ULevel* loadedLevel = levelStreaming->GetLoadedLevel();
				if (IsValid(loadedLevel))
				{
					FString folderName = levelStreaming->PackageNameToLoad.ToString();
					folderName.RemoveFromStart("/Game/", ESearchCase::IgnoreCase);
					folderName.RemoveFromStart("Map/", ESearchCase::IgnoreCase);
					folderName.RemoveFromStart("Maps/", ESearchCase::IgnoreCase);
					folderName.RemoveFromStart("Level/", ESearchCase::IgnoreCase);
					folderName.RemoveFromStart("Levels/", ESearchCase::IgnoreCase);
					folderName.RemoveFromStart("/", ESearchCase::IgnoreCase);
					const FString folderPath = TEXT("Dungeon/Levels/") + folderName;
					const FName folderPathName(folderPath, index);
				
					for (AActor* actor : loadedLevel->Actors)
					{
						actor->Tags.Add(GetDungeonGeneratorTag());
						actor->SetFolderPath(folderPathName);
					}

					moveActors.Append(loadedLevel->Actors);
					++index;
				}

				mLoadedStreamLevels.Add(levelStreaming);

				DUNGEON_GENERATOR_LOG(TEXT("Load Level (%s)"), *requestStreamLevel.mPath.GetLongPackageName());
			}
			else
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Failed to Load Level (%s)"), *requestStreamLevel.mPath.GetLongPackageName());
			}
		}

		if (moveActors.Num() > 0)
		{
			EditorLevelUtils::MoveActorsToLevel(moveActors, world->PersistentLevel);
		}

		mRequestLoadStreamLevels.clear();
	}
}
#endif

void CDungeonGeneratorCore::UnloadStreamLevels()
{
	mRequestLoadStreamLevels.clear();

	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		for (const TSoftObjectPtr<ULevelStreamingDynamic>& streamLevel : mLoadedStreamLevels)
		{
			world->RemoveStreamingLevel(streamLevel.Get());
		}
		mLoadedStreamLevels.Empty();
	}
}

void CDungeonGeneratorCore::LoadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const FTransform& transform)
{
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	const FName& longPackageName = path.GetLongPackageFName();
#else
	const FName& longPackageName = FName(path.GetLongPackageName());
#endif
	ULevelStreaming* levelStreaming;

	levelStreaming = UGameplayStatics::GetStreamingLevel(world, longPackageName);
	if (IsValid(levelStreaming))
	{
		UnloadStreamLevelImplement(world, path, true);
	}

	FLatentActionInfo LatentInfo;
	UGameplayStatics::LoadStreamLevel(world, longPackageName, false, false, LatentInfo);

	levelStreaming = UGameplayStatics::GetStreamingLevel(world, longPackageName);
	if (IsValid(levelStreaming))
	{
		levelStreaming->LevelTransform = transform;
		levelStreaming->SetShouldBeVisible(true);
	}
}

void CDungeonGeneratorCore::UnloadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const bool shouldBlockOnUnload)
{
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	const FName& longPackageName = path.GetLongPackageFName();
#else
	const FName& longPackageName = FName(path.GetLongPackageName());
#endif
	FLatentActionInfo LatentInfo;
	UGameplayStatics::UnloadStreamLevel(world, longPackageName, LatentInfo, shouldBlockOnUnload);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
void CDungeonGeneratorCore::DrawDebugInformation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const
{
	// 部屋と接続情報のデバッグ情報を表示します
	if (showRoomAisleInfomation)
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

void CDungeonGeneratorCore::DrawRoomAisleInformation() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
		return;

	check(mGenerator);

	mGenerator->ForEach([this, parameter](const std::shared_ptr<const dungeon::Room>& room)
		{
			UWorld* world = mWorld.Get();
			if (IsValid(world))
			{
				UKismetSystemLibrary::DrawDebugBox(
					world,
					room->GetCenter() * parameter->GetGridSize(),
					room->GetExtent() * parameter->GetGridSize(),
					FColor::Magenta,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);

				UKismetSystemLibrary::DrawDebugSphere(
					world,
					room->GetGroundCenter() * parameter->GetGridSize(),
					10.f,
					12,
					FColor::Magenta,
					0.f,
					2.f
				);
			}
		}
	);

	mGenerator->EachAisle([this, parameter](const dungeon::Aisle& edge)
		{
			UWorld* world = mWorld.Get();
			if (IsValid(world))
			{
				UKismetSystemLibrary::DrawDebugLine(
					world,
					*edge.GetPoint(0) * parameter->GetGridSize(),
					*edge.GetPoint(1) * parameter->GetGridSize(),
					FColor::Red,
					0.f,
					5.f
				);

				const FVector start(static_cast<int32>(edge.GetPoint(0)->X), static_cast<int32>(edge.GetPoint(0)->Y), static_cast<int32>(edge.GetPoint(0)->Z));
				const FVector goal(static_cast<int32>(edge.GetPoint(1)->X), static_cast<int32>(edge.GetPoint(1)->Y), static_cast<int32>(edge.GetPoint(1)->Z));
				UKismetSystemLibrary::DrawDebugSphere(
					world,
					start * parameter->GetGridSize() + FVector(parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f),
					10.f,
					12,
					FColor::Green,
					0.f,
					5.f
				);
				UKismetSystemLibrary::DrawDebugSphere(
					world,
					goal * parameter->GetGridSize() + FVector(parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f),
					10.f,
					12,
					FColor::Red,
					0.f,
					5.f
				);
			}
		}
	);
}

void CDungeonGeneratorCore::DrawVoxelGridType() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
		return;

	check(mGenerator);

	mGenerator->GetVoxel()->Each([this, parameter](const FIntVector& location, const dungeon::Grid& grid)
		{
			UWorld* world = mWorld.Get();
			if (IsValid(world))
			{
				//if (grid.GetType() == dungeon::Grid::Aisle || grid.GetType() == dungeon::Grid::Slope)
				if (grid.GetType() != dungeon::Grid::Type::Empty && grid.GetType() != dungeon::Grid::Type::OutOfBounds)
				{
					const FVector halfGrid(parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f, parameter->GetGridSize() / 2.f);
					UKismetSystemLibrary::DrawDebugBox(
						world,
						FVector(location.X, location.Y, location.Z) * parameter->GetGridSize() + halfGrid,
						halfGrid * 0.95,
						grid.GetTypeColor(),
						FRotator::ZeroRotator,
						0.f,
						5.f
					);
				}
			}

			return true;
		}
	);
}
#endif

std::shared_ptr<dungeon::Random> CDungeonGeneratorCore::GetRandom() noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}

std::shared_ptr<dungeon::Random> CDungeonGeneratorCore::GetRandom() const noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}
