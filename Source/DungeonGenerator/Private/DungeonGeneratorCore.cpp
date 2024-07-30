/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorCore.h"
#include "Core/Generator.h"
#include "Core/Debug/Debug.h"
#include "Core/Debug/Config.h"
#include "Core/Helper/Direction.h"
#include "Core/Helper/Identifier.h"
#include "Core/Helper/Stopwatch.h"
#include "Core/Math/Math.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Voxel.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "Mission/DungeonRoomProps.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include "SubActor/DungeonDoorBase.h"
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

#include <numeric>
#include <unordered_map>

#if WITH_EDITOR
// UnrealEd
#include <EditorActorFolders.h>
#include <EditorLevelUtils.h>
#include <Builders/CubeBuilder.h>
#endif

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
#define BEGIN_STOPWATCH()	dungeon::Stopwatch stopwatch
#define END_STOPWATCH(VAR)	VAR += stopwatch.Lap()
#else
#define BEGIN_STOPWATCH()	((void)0)
#define END_STOPWATCH(VAR)	((void)0)
#endif

namespace
{
	inline static bool operator==(const EDungeonRoomItem left, const dungeon::Room::Item right)
	{
		return static_cast<uint8_t>(left) == static_cast<uint8_t>(right);
	}
}

CDungeonGeneratorCore::CDungeonGeneratorCore(const TWeakObjectPtr<UWorld>& world)
	: mWorld(world)
{
	AddStaticMeshEvent addFloorStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("DungeonGenerator/Meshes/Floor"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_WARNING(TEXT("addFloorStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#endif
		};
	AddStaticMeshEvent addSlopeStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("DungeonGenerator/Meshes/Slope"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_WARNING(TEXT("addSlopeStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#endif
		};
	AddStaticMeshEvent addWallStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("DungeonGenerator/Meshes/Wall"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_WARNING(TEXT("addWallStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#endif
		};
	AddStaticMeshEvent addRoofStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("DungeonGenerator/Meshes/Roof"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_WARNING(TEXT("addRoofStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#endif
		};
	AddPillarStaticMeshEvent addPillarStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
	{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("DungeonGenerator/Meshes/Pillars"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
			{
				// 通信同期用のデバッグ情報を出力
				uint32_t x, y, z, w;
				GetSynchronizedRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_WARNING(TEXT("addPillarStaticMeshEvent %s: %x, %x, %x, %x: CRC32=%x"), *actor->GetName(), x, y, z, w, mCrc32AtCreation);
			}
#endif
		};

	mOnAddFloor = addFloorStaticMeshEvent;
	mOnAddSlope = addSlopeStaticMeshEvent;
	mOnAddWall = addWallStaticMeshEvent;
	mOnAddRoof = addRoofStaticMeshEvent;
	mOnAddPillar = addPillarStaticMeshEvent;
}

CDungeonGeneratorCore::~CDungeonGeneratorCore()
{
	DestroySpawnedActors();
	Clear();
}

/*
hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
*/
bool CDungeonGeneratorCore::Create(const UDungeonGenerateParameter* parameter, const FVector& origin, const bool hasAuthority)
{
#if WITH_EDITOR
	dungeon::CreateDebugDirectory();
#endif

	DUNGEON_GENERATOR_LOG(TEXT("version '%s', license '%s', uuid '%s', commit '%s'"),
		TEXT(DUNGENERATOR_PLUGIN_VERSION_NAME),
		TEXT(JENKINS_LICENSE),
		TEXT(JENKINS_UUID),
		TEXT(JENKINS_GIT_COMMIT)
	);

	// CRC32の値を初期化
	mCrc32AtCreation = ~0;

	// Conversion from UDungeonGenerateParameter to dungeon::GenerateParameter
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the dungeon generation parameters"));
		Clear();
		return false;
	}


	// ダンジョン生成パラメータを生成
	dungeon::GenerateParameter generateParameter;
	{
		int32 randomSeed;
		if (hasAuthority)
		{
			// Server
			randomSeed = parameter->GetRandomSeed();
			if (randomSeed == 0)
				randomSeed = static_cast<int32>(time(nullptr));
			const_cast<UDungeonGenerateParameter*>(parameter)->SetGeneratedRandomSeed(randomSeed);
		}
		else
		{
			// Client
			randomSeed = parameter->GetGeneratedRandomSeed();
		}
		generateParameter.GetRandom()->SetSeed(randomSeed);
		generateParameter.SetNumberOfCandidateRooms(parameter->NumberOfCandidateRooms);
		generateParameter.SetMinRoomWidth(parameter->RoomWidth.Min);
		generateParameter.SetMaxRoomWidth(parameter->RoomWidth.Max);
		generateParameter.SetMinRoomDepth(parameter->RoomDepth.Min);
		generateParameter.SetMaxRoomDepth(parameter->RoomDepth.Max);
		generateParameter.SetMinRoomHeight(parameter->RoomHeight.Min);
		generateParameter.SetMaxRoomHeight(parameter->RoomHeight.Max);
		generateParameter.SetMergeRooms(parameter->MergeRooms);
		generateParameter.SetMissionGraph(parameter->EnableMissionGraph());
		generateParameter.SetAisleComplexity(parameter->GetAisleComplexity());

		if (parameter->MergeRooms)
		{
			generateParameter.SetHorizontalRoomMargin(0);
			generateParameter.SetVerticalRoomMargin(0);
			generateParameter.SetNumberOfCandidateFloors(1);
		}
		else if (parameter->Flat)
		{
			generateParameter.SetHorizontalRoomMargin(parameter->RoomMargin);
			generateParameter.SetVerticalRoomMargin(0);
			generateParameter.SetNumberOfCandidateFloors(1);
		}
		else
		{
			generateParameter.SetHorizontalRoomMargin(parameter->RoomMargin);
			generateParameter.SetVerticalRoomMargin(parameter->VerticalRoomMargin);
			generateParameter.SetNumberOfCandidateFloors(
					static_cast<uint8_t>(std::min(
					static_cast<uint16_t>(generateParameter.GetMaxRoomHeight() + 2),
					static_cast<uint16_t>(std::numeric_limits<uint8_t>::max())
				))
			);
		}

	}

	// クライアント用乱数生成器を初期化
	mLocalRandom = std::make_shared<dungeon::Random>(generateParameter.GetRandom()->Get<uint32_t>());

	// UDungeonGenerateParameterを保存
	mParameter = parameter;

	// ダンジョン生成コアの初期設定
	mGenerator = std::make_shared<dungeon::Generator>();
	if (mGenerator == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("System initialization failed."));
		Clear();
		return false;
	}
#if WITH_EDITOR
	// 通信同期用に現在の乱数の種を出力する
	{
		uint32_t x, y, z, w;
		generateParameter.GetRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation start: Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, %s"),
			x, y, z, w, mCrc32AtCreation, hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		if (hasAuthority)
		{
			GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("generation start:       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, Server"),
				x, y, z, w, mCrc32AtCreation
			);
		}
	}
#endif

	// ダンジョンを生成
	mGenerator->Generate(generateParameter);

	// デバッグ情報を出力
#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
	{
		// TODO:外部からファイル名を与えられるように変更して下さい
		mGenerator->DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/dungeon_diagram.md");
	}
#endif

	// 生成エラーを確認する
	dungeon::Generator::Error generatorError = mGenerator->GetLastError();
	if (dungeon::Generator::Error::Success != generatorError)
	{
#if WITH_EDITOR
		// デバッグに必要な情報（デバッグ生成パラメータ）を出力する
		//parameter->DumpToJson();
#endif
		return false;
	}
	else
	{
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
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

		// メッシュの生成
		{
			std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*> roomSensorCache;
			CreateImplement_PrepareSpawnRoomSensor(origin, roomSensorCache);
			CreateImplement_AddTerrain(origin, roomSensorCache, hasAuthority);
			CreateImplement_FinishSpawnRoomSensor(roomSensorCache);
			CreateImplement_Navigation(origin);
		}

#if WITH_EDITOR
		// 通信同期用に現在の乱数の種を出力する
		if (mGenerator)
		{
			uint32_t x, y, z, w;
			generateParameter.GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("generation end  : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
				x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
			);
			if (hasAuthority)
			{
				GetRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_LOG(TEXT("generation end  :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, Server"),
					x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0)
				);
			}
		}
#endif
		return true;
	}
}

/*
ボクセル情報にしたがってメッシュを生成します

hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
*/
void CDungeonGeneratorCore::CreateImplement_AddTerrain(const FVector& origin, std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache, const bool hasAuthority)
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set. Please set it."));
		return;
	}

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p1   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		if (hasAuthority)
		{
			GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("generation p1   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, Server"),
				x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0)
			);
		}
	}
#endif

	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		double floorAndSlopeStopwatch = 0;
		double wallStopwatch = 0;
		double pillarAndTorchStopwatch = 0;
		double doorStopwatch = 0;
		double roofStopwatch = 0;
		dungeon::Stopwatch stopwatch;
		mGenerator->GetVoxel()->Each([this, &origin, &roomSensorCache, &floorAndSlopeStopwatch, &wallStopwatch, &pillarAndTorchStopwatch, &doorStopwatch, &roofStopwatch, hasAuthority]
#else
		mGenerator->GetVoxel()->Each([this, &origin, &roomSensorCache, hasAuthority]
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

				const UDungeonGenerateParameter* parameter = mParameter.Get();
				if (!IsValid(parameter))
					return true;

				const FVector position = origin + parameter->ToWorld(location);
				const FVector gridSize = parameter->GetGridSize().To3D();
				const FVector gridHalfSize = gridSize / 2.;
				const FVector centerPosition = position + FVector(gridHalfSize.X, gridHalfSize.Y, 0);
				const CreateImplementParameter createImplementParameter =
				{
					mParameter.Get(),
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
					BEGIN_STOPWATCH();
					CreateImplement_AddFloorAndSlope(createImplementParameter);
					END_STOPWATCH(floorAndSlopeStopwatch);
				}

				// Generate wall mesh
				{
					BEGIN_STOPWATCH();
					CreateImplement_AddWall(createImplementParameter);
					END_STOPWATCH(wallStopwatch);
				}

				// Generate mesh for pillars and torches
				{
					BEGIN_STOPWATCH();
					CreateImplement_AddPillarAndTorch(createImplementParameter, dungeonRoomSensorBase, hasAuthority);
					END_STOPWATCH(pillarAndTorchStopwatch);
				}

				// Generate door mesh
				{
					BEGIN_STOPWATCH();
					CreateImplement_AddDoor(createImplementParameter, dungeonRoomSensorBase, hasAuthority);
					END_STOPWATCH(doorStopwatch);
				}

				// Generate roof mesh
				{
					BEGIN_STOPWATCH();
					CreateImplement_AddRoof(createImplementParameter);
					END_STOPWATCH(roofStopwatch);
				}

				return true;
			}
		);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		DUNGEON_GENERATOR_LOG(TEXT("Spawn meshes and actors: %lf seconds"), stopwatch.Lap());
		DUNGEON_GENERATOR_LOG(TEXT("  floor and slope meshes: %lf seconds"), floorAndSlopeStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT("  wall meshes: %lf seconds"), wallStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT("  roof meshes: %lf seconds"), roofStopwatch);
		DUNGEON_GENERATOR_LOG(TEXT("  door actors: %lf seconds"), doorStopwatch);
#endif
	}

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p2   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		if (hasAuthority)
		{
			GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("generation p2   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, Server"),
				x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0)
			);
		}
	}
#endif
}

/*
床とスロープはレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void CDungeonGeneratorCore::CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp)
{
	if (mOnAddSlope && cp.mGrid.CanBuildSlope())
	{
		/*
		スロープのメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		if (const FDungeonMeshParts* parts = cp.mParameter->SelectSlopeParts(cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
		{
			mOnAddSlope(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
		}
	}
	else if (mOnAddFloor && cp.mGrid.CanBuildFloor(true))
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		if (cp.mGrid.IsKindOfRoomType())
			dungeonMeshSetDatabase = cp.mParameter->GetDungeonRoomPartsDatabase();
		else
			dungeonMeshSetDatabase = cp.mParameter->GetDungeonAislePartsDatabase();

		if (const FDungeonMeshParts* parts = cp.mParameter->SelectFloorParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
		{
			/*
			床のメッシュを生成
			メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
			*/
			mOnAddFloor(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
		}
	}
}

/*
壁はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void CDungeonGeneratorCore::CreateImplement_AddWall(const CreateImplementParameter& cp)
{
	if (mOnAddWall == nullptr)
		return;

	/*
	壁のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面（北側の壁）になっています。
	*/
	const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
	if (cp.mGrid.IsKindOfRoomType())
		dungeonMeshSetDatabase = cp.mParameter->GetDungeonRoomPartsDatabase();
	else
		dungeonMeshSetDatabase = cp.mParameter->GetDungeonAislePartsDatabase();

	if (const FDungeonMeshParts* parts = cp.mParameter->SelectWallParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
	{
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z), dungeon::Direction::North, cp.mParameter->IsMergeRooms()))
		{
			// 北側の壁
			FVector wallPosition = cp.mCenterPosition;
			wallPosition.Y -= cp.mGridHalfSize.Y;
			mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 0.f));

		}
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z), dungeon::Direction::South, cp.mParameter->IsMergeRooms()))
		{
			// 南側の壁
			FVector wallPosition = cp.mCenterPosition;
			wallPosition.Y += cp.mGridHalfSize.Y;
			mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 180.f));

		}
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::East, cp.mParameter->IsMergeRooms()))
		{
			// 東側の壁
			FVector wallPosition = cp.mCenterPosition;
			wallPosition.X += cp.mGridHalfSize.X;
			mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 90.f));

		}
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::West, cp.mParameter->IsMergeRooms()))
		{
			// 西側の壁
			FVector wallPosition = cp.mCenterPosition;
			wallPosition.X -= cp.mGridHalfSize.X;
			mOnAddWall(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, -90.f));

		}
	}
}

/*
天井はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void CDungeonGeneratorCore::CreateImplement_AddRoof(const CreateImplementParameter& cp)
{
	if (mOnAddRoof == nullptr)
		return;

	if (cp.mGrid.CanBuildRoof(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y, cp.mGridLocation.Z + 1), true))
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		if (cp.mGrid.IsKindOfRoomType())
			dungeonMeshSetDatabase = cp.mParameter->GetDungeonRoomPartsDatabase();
		else
			dungeonMeshSetDatabase = cp.mParameter->GetDungeonAislePartsDatabase();

		/*
		壁のメッシュを生成
		メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
		*/
		const FTransform transform(cp.mCenterPosition);
		if (const FDungeonMeshPartsWithDirection* parts = cp.mParameter->SelectRoofParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
		{
			mOnAddRoof(
				parts->StaticMesh,
				parts->CalculateWorldTransform(GetSynchronizedRandom(), transform)
			);
		}
	}
}

/*
ADungeonDoorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
void CDungeonGeneratorCore::CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority)
{
	if (CanAddDoor(dungeonRoomSensorBase, cp.mGridLocation, cp.mGrid) && hasAuthority)
	{
		if (const FDungeonDoorActorParts* parts = cp.mParameter->SelectDoorParts(cp.mGridIndex, cp.mGrid, GetRandom()))
		{
			const EDungeonRoomProps props = static_cast<EDungeonRoomProps>(cp.mGrid.GetProps());
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z), dungeon::Direction::North, cp.mParameter->IsMergeRooms()))
			{
				// 北側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += cp.mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 0.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z), dungeon::Direction::South, cp.mParameter->IsMergeRooms()))
			{
				// 南側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += cp.mParameter->GetGridSize().HorizontalSize * 0.5f;
				doorPosition.Y += cp.mParameter->GetGridSize().HorizontalSize;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 180.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::East, cp.mParameter->IsMergeRooms()))
			{
				// 東側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += cp.mParameter->GetGridSize().HorizontalSize;
				doorPosition.Y += cp.mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 90.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::West, cp.mParameter->IsMergeRooms()))
			{
				// 西側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.Y += cp.mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, -90.f), dungeonRoomSensorBase, props);
			}
		}
	}
}

/*
ADungeonDoorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
bool CDungeonGeneratorCore::CanAddDoor(ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const
{
	if (grid.Is(dungeon::Grid::Type::Gate) == false)
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
		if (mGenerator->GetVoxel()->Get(e).Is(dungeon::Grid::Type::Gate))
			return false;

		const FIntVector w(location.X - 1, location.Y, location.Z);
		if (mGenerator->GetVoxel()->Get(w).Is(dungeon::Grid::Type::Gate))
			return false;
	}
	else
	{
		const FIntVector n(location.X, location.Y - 1, location.Z);
		if (mGenerator->GetVoxel()->Get(n).Is(dungeon::Grid::Type::Gate))
			return false;

		const FIntVector s(location.X, location.Y + 1, location.Z);
		if (mGenerator->GetVoxel()->Get(s).Is(dungeon::Grid::Type::Gate))
			return false;
	}

	return true;
}

/*
柱はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする

燭台アクターはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
void CDungeonGeneratorCore::CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority)
{
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
	std::vector<TorchChecker> torchCheckers;
	uint8_t validGridCount = 0;

	struct WallChecker final
	{
		dungeon::Direction::Index mDirection;

		constexpr explicit WallChecker(dungeon::Direction::Index direction)
			: mDirection(direction) { }
	};
	static const std::array<WallChecker, 8> wallCheckers = { {
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

	/*
	柱のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
	*/
	FIntVector checkLocation = cp.mGridLocation;
	for (const auto& wallChecker : wallCheckers)
	{
		const FIntVector& direction = dungeon::Direction::GetVector(wallChecker.mDirection);
		const auto& fromGrid = mGenerator->GetVoxel()->Get(checkLocation);
		checkLocation += direction;
		const auto& toGrid = mGenerator->GetVoxel()->Get(checkLocation);

		// 柱必要か調べます
		if (fromGrid.CanBuildWall(toGrid, wallChecker.mDirection, cp.mParameter->IsMergeRooms()) == true)
		{
			++validGridCount;

			static const auto registerer = [](std::vector<TorchChecker>& torchCheckers, const uint16_t identifier, const FIntVector& normal)
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

			bool generationPermit = true;
			if (cp.mParameter->GetFrequencyOfTorchlightGeneration() >= EFrequencyOfGeneration::Occasionally)
			{
				generationPermit = fromGrid.Is(dungeon::Grid::Type::Floor) == false;
			}

			// 燭台を生成できないグリッドを含んでいる？
			if (
				generationPermit &&
				fromGrid.Is(dungeon::Grid::Type::DownSpace) == false &&
				fromGrid.Is(dungeon::Grid::Type::Slope) == false &&
				fromGrid.IsKindOfSpatialType() == false)
			{
				registerer(torchCheckers, fromGrid.GetIdentifier(), direction * -1);
			}
		}
	}

	if (1 <= validGridCount)
	{
		const FTransform rootTransform(cp.mPosition);

		// 柱を生成
		if (mOnAddPillar)
		{
			if (const FDungeonMeshParts* pillarParts = cp.mParameter->SelectPillarParts(cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddPillar(pillarParts->StaticMesh, pillarParts->CalculateWorldTransform(rootTransform));
			}
		}

		// 燭台を生成（サーバーのみアクターをスポーンする）
		bool spawnTorchActor = false;
		switch (cp.mParameter->GetFrequencyOfTorchlightGeneration())
		{
		case EFrequencyOfGeneration::Normaly:
			spawnTorchActor = true;
			break;
		case EFrequencyOfGeneration::Sometime:
		case EFrequencyOfGeneration::Occasionally:
			{
				const int32 column = cp.mGridLocation.X & 1;
				const int32 row = cp.mGridLocation.Y & 1;
				spawnTorchActor = (cp.mGridLocation.Z & 1) ? row == column : row != column;
			}
			break;
		case EFrequencyOfGeneration::Rarely:
			if (GetRandom()->Get<bool>())
			{
				const int32 column = cp.mGridLocation.X & 1;
				const int32 row = cp.mGridLocation.Y & 1;
				spawnTorchActor = (cp.mGridLocation.Z & 1) ? row == column : row != column;
			}
			break;
		case EFrequencyOfGeneration::AlmostNever:
			if ((GetRandom()->Get<uint32_t>() & 7) == 0)
			{
				const int32 column = cp.mGridLocation.X & 1;
				const int32 row = cp.mGridLocation.Y & 1;
				spawnTorchActor = (cp.mGridLocation.Z & 1) ? row == column : row != column;
			}
			break;
		case EFrequencyOfGeneration::Never:
		default:
			break;
		}
		if (spawnTorchActor)
		{
			for (auto& torchChecker : torchCheckers)
			{
				if (!torchChecker.IsValid())
					continue;

				if (const FDungeonActorParts* torchParts = cp.mParameter->SelectTorchParts(cp.mGridIndex, cp.mGrid, GetRandom()))
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
						SpawnTorchActor(torchParts->ActorClass, worldTransform, dungeonRoomSensorBase, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
void CDungeonGeneratorCore::CreateImplement_PrepareSpawnRoomSensor(const FVector& origin, std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache)
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (!IsValid(parameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("DungeonGenerateParameter is not set. Please set it."));
		return;
	}

	// RoomSensorActorを生成
	mGenerator->ForEach([this, parameter, &origin, &roomSensorCache](const std::shared_ptr<const dungeon::Room>& room)
		{
			// RoomSensor
			ADungeonRoomSensorBase* roomSensorActor = SpawnRoomSensorActorDeferred(
				parameter->GetRoomSensorClass(),
				room->GetIdentifier(),
				room->GetCenter() * parameter->GetGridSize().To3D() + origin,
				room->GetExtent() * parameter->GetGridSize().To3D(),
				static_cast<EDungeonRoomParts>(room->GetParts()),
				static_cast<EDungeonRoomItem>(room->GetItem()),
				room->GetBranchId(),
				room->GetDepthFromStart(),
				mGenerator->GetDeepestDepthFromStart()	//!< TODO:適切な関数名に変えて下さい
			);
			roomSensorCache[room.get()] = roomSensorActor;
		}
	);
}

/*
ADungeonRoomSensorBaseはリプリケートされない前提のアクターなので
必ず同期乱数(GetSynchronizedRandom)を使ってください。
*/
void CDungeonGeneratorCore::CreateImplement_FinishSpawnRoomSensor(std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache)
{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	double roomSensorStopwatch = 0;
#endif
	for (auto& roomSensorIterator : roomSensorCache)
	{
		auto* roomSensorActor = roomSensorIterator.second;
		if (IsValid(roomSensorActor))
		{
			BEGIN_STOPWATCH();
			FinishRoomSensorActorSpawning(roomSensorActor);
			END_STOPWATCH(roomSensorStopwatch);
		}
	}
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	DUNGEON_GENERATOR_LOG(TEXT("Spawn DungeonRoomSensor actors: %lf seconds"), roomSensorStopwatch);
#endif
}


void CDungeonGeneratorCore::CreateImplement_Navigation(const FVector& origin)
{
#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
	// 通信同期用に現在の乱数の種を出力する
	if (mGenerator)
	{
		uint32_t x, y, z, w;
		GetSynchronizedRandom()->GetSeeds(x, y, z, w);
		DUNGEON_GENERATOR_LOG(TEXT("generation p3   : Synchronize RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, %s"),
			x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0), hasAuthority ? TEXT("Server") : TEXT("Client")
		);
		if (hasAuthority)
		{
			GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("generation p3   :       Local RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x, CRC32(voxel)=%x, Server"),
				x, y, z, w, mCrc32AtCreation, mGenerator->CalculateCRC32(~0)
			);
		}
	}
#endif

	// RecastNavMeshを生成
	SpawnRecastNavMesh();

	// NavMeshBoundsVolumeを初期化
#if 0
	//SpawnRecastNavMesh();
	SpawnNavMeshBoundsVolume(parameter);

#else

	if (ANavMeshBoundsVolume* navMeshBoundsVolume = FindActor<ANavMeshBoundsVolume>())
	{
		const FBox& bounding = CalculateBoundingBox();
		const FVector& boundingCenter = origin + bounding.GetCenter();
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


	mParameter = nullptr;
}

FTransform CDungeonGeneratorCore::GetStartTransform() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		return FTransform(*mGenerator->GetStartPoint() * parameter->GetGridSize().To3D());
	}
	return FTransform::Identity;
}

FTransform CDungeonGeneratorCore::GetGoalTransform() const
{
	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) && mGenerator != nullptr && mGenerator->GetLastError() == dungeon::Generator::Error::Success)
	{
		return FTransform(*mGenerator->GetGoalPoint() * parameter->GetGridSize().To3D());
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
			boundingBox.Min.Z -= parameter->GetGridSize().VerticalSize;
			boundingBox.Max.Z += parameter->GetGridSize().VerticalSize;
			return boundingBox;
		}
	}

	return FBox(EForceInit::ForceInitToZero);
}

/*
APlayerStartPIEを除くAPlayerStartを収集してstartPointsに記録します
*/
void CDungeonGeneratorCore::MovePlayerStart(TArray<APlayerStart*>& startPoints)
{
	EachActors<APlayerStart>([&startPoints](APlayerStart* playerStart)
		{
			// "Play from Here" PlayerStart, if we find one while in PIE mode
			if (Cast<APlayerStartPIE>(playerStart))
				return true;

			if (USceneComponent* rootComponent = playerStart->GetRootComponent())
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
void CDungeonGeneratorCore::MovePlayerStartPIE(const FVector& origin, const TArray<APlayerStart*>& startPoints)
{
	if (startPoints.Num() <= 0)
	{
		// APlayerStartが無いなら、サブレベル内のAPlayerStartの姿勢をAPlayerStartPIEに設定する
		TArray<APlayerStart*> startPointsInSubLevels;
		EachActors<APlayerStart>([&startPointsInSubLevels](APlayerStart* playerStart)
			{
				if (Cast<APlayerStartPIE>(playerStart))
					return true;

				if (USceneComponent* rootComponent = playerStart->GetRootComponent())
					startPointsInSubLevels.Add(playerStart);

				return true;
			}
		);
		if (startPointsInSubLevels.Num() > 0)
		{
			EachActors<APlayerStartPIE>([&startPointsInSubLevels](APlayerStartPIE* playerStartPIE)
				{
					APlayerStart* playerStart = startPointsInSubLevels[FMath::RandRange(0, startPointsInSubLevels.Num() - 1)];
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
	// TODO:メニュー内の「ここから開始」で問題が起きるかもしれません
	EachActors<APlayerStartPIE>([&startPoints](APlayerStartPIE* playerStartPIE)
		{
			APlayerStart* playerStart = startPoints[FMath::RandRange(0, startPoints.Num() - 1)];
			if (IsValid(playerStart))
			{
				const auto& transform = playerStart->GetActorTransform();
				playerStartPIE->SetActorTransform(transform);
			}
			return true;
		}
	);

	const UDungeonGenerateParameter* parameter = mParameter.Get();
	if (IsValid(parameter) == false)
		return;

	// PlayerStartをスタート位置へ移動しないならここで終了します
	if (parameter->IsMovePlayerStartToStartingPoint() == false)
		return;

	// Calculate the position to shift APlayerStart
	const double placementRadius = parameter->GetGridSize().HorizontalSize;
	const double placementAngle = (3.1415926535897932384626433832795 * 2.) / static_cast<double>(startPoints.Num());

	for (int32 index = 0; index < startPoints.Num(); ++index)
	{
		APlayerStart* playerStart = startPoints[index];

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
				FVector location = origin + GetStartLocation();
				location.X += placementRadius * std::cos(placementAngle * static_cast<double>(index));
				location.Y += placementRadius * std::sin(placementAngle * static_cast<double>(index));

				// Grounding the APlayerStart
				FHitResult hitResult;
				const FVector startLocation = location + FVector(0, 0, parameter->GetGridSize().VerticalSize);
				const FVector endLocation = location - FVector(0, 0, parameter->GetGridSize().VerticalSize);
				if (playerStart->GetWorld()->LineTraceSingleByChannel(hitResult, startLocation, endLocation, ECollisionChannel::ECC_Pawn))
				{
					location = hitResult.ImpactPoint;
				}
				location.Z += cylinderHalfHeight + heightMargin;
				playerStart->SetActorLocation(location);

#if WITH_EDITOR
				FCollisionShape collisionShape;
				collisionShape.SetCapsule(cylinderRadius, cylinderHalfHeight);
				if (playerStart->GetWorld()->OverlapBlockingTestByChannel(location, playerStart->GetActorQuat(), ECollisionChannel::ECC_Pawn, collisionShape))
				{
					DUNGEON_GENERATOR_ERROR(TEXT("PlayerStart(%s) is in contact with something"), *playerStart->GetName());
				}
#endif
			}
			// Undo EComponentMobility
			rootComponent->SetMobility(mobility);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Set RootComponent of PlayerStart(%s)"), *playerStart->GetName());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
AActor* CDungeonGeneratorCore::SpawnActorImpl(UWorld* world, UClass* actorClass, const FName& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters)
{
	if (!IsValid(world))
		return nullptr;

	AActor* actor = world->SpawnActor(actorClass, &transform, actorSpawnParameters);
	if (!IsValid(actor))
		return nullptr;

#if WITH_EDITOR
	actor->SetFolderPath(folderPath);
#endif

	actor->Tags.Reserve(1);
	actor->Tags.Emplace(GetDungeonGeneratorTag());

	return actor;
}

AStaticMeshActor* CDungeonGeneratorCore::SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AStaticMeshActor* actor = SpawnActorImpl<AStaticMeshActor>(AStaticMeshActor::StaticClass(), folderPath, transform, nullptr, spawnActorCollisionHandlingMethod);
	if (!IsValid(actor))
		return nullptr;

	UStaticMeshComponent* mesh = actor->GetStaticMeshComponent();
	if (IsValid(mesh))
		mesh->SetStaticMesh(staticMesh);

	// 処理負荷制御を追加
	UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent = NewObject<UDungeonComponentActivatorComponent>(actor);
	if (IsValid(dungeonComponentActivatorComponent))
	{
		dungeonComponentActivatorComponent->SetEnableCollisionEnableControl(false);

		actor->AddInstanceComponent(dungeonComponentActivatorComponent);
		dungeonComponentActivatorComponent->RegisterComponent();
	}

	// CRC32を記録（必ずサーバーとクライアント両方で計算しないとCRC32が一致しなくなる）
	mCrc32AtCreation = ADungeonActorBase::GenerateCrc32(transform, mCrc32AtCreation);

	return actor;
}

AActor* CDungeonGeneratorCore::SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const
{
	AActor* actor = SpawnActorImpl<AActor>(actorClass, TEXT("DungeonGenerator/Actors"), transform, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		FVector location = actor->GetActorLocation();
		location.Z += actor->GetSimpleCollisionHalfHeight();
		actor->SetActorLocation(location);
	}
	return actor;
}

ADungeonDoorBase* CDungeonGeneratorCore::SpawnDoorActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, EDungeonRoomProps props) const
{
	ADungeonDoorBase* actor = SpawnActorDeferredImpl<ADungeonDoorBase>(actorClass, TEXT("DungeonGenerator/Actors/Doors"), transform, ownerActor, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		actor->InvokeInitialize(GetRandom(), props);

		actor->FinishSpawning(transform);

		if (IsValid(ownerActor))
			ownerActor->AddDungeonDoor(actor);
	}
	return actor;
}

AActor* CDungeonGeneratorCore::SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AActor* actor = SpawnActorImpl(actorClass, TEXT("DungeonGenerator/Actors/Torches"), transform, ownerActor, spawnActorCollisionHandlingMethod);
	if (IsValid(ownerActor))
		ownerActor->AddDungeonTorch(actor);
	return actor;
}

/*
ADungeonRoomSensorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
ADungeonRoomSensorBase* CDungeonGeneratorCore::SpawnRoomSensorActorDeferred(UClass* actorClass, const dungeon::Identifier& identifier, const FVector& center, const FVector& extent, EDungeonRoomParts parts, EDungeonRoomItem item, uint8 branchId, const uint8 depthFromStart, const uint8 deepestDepthFromStart) const
{
	const FTransform transform(center);
	ADungeonRoomSensorBase* actor = SpawnActorDeferredImpl<ADungeonRoomSensorBase>(actorClass, TEXT("DungeonGenerator/Sensors"), transform, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		const bool success = actor->InvokePrepare(
			GetRandom(),
			identifier,
			center,
			extent,
			parts,
			item,
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
void CDungeonGeneratorCore::FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor)
{
	if (IsValid(dungeonRoomSensor))
	{
		dungeonRoomSensor->InvokeInitialize();
		dungeonRoomSensor->FinishSpawning(FTransform::Identity, true);
	}
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
	UGameplayStatics::GetAllActorsWithTag(world, GetDungeonGeneratorTag(), actors);

#if WITH_EDITOR
	TArray<FFolder> deleteFolders;

	// TagにDungeonGeneratorTagがついているアクターのフォルダを回収
	for (AActor* actor : actors)
	{
		if (IsValid(actor))
		{
			const FFolder& folder = actor->GetFolder();

#if UE_VERSION_NEWER_THAN(5, 1, 0)
			if (!folder.IsValid())
				continue;
#endif
			if (folder.GetPath() == folder.GetEmptyPath())
				continue;

#if UE_VERSION_NEWER_THAN(5, 1, 0)
			if (const auto* actorFolder = folder.GetActorFolder())
			{
				if (!actorFolder->IsValid())
					continue;
			}
#endif

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

////////////////////////////////////////////////////////////////////////////////

uint32_t CDungeonGeneratorCore::CalculateCRC32() const noexcept
{
	uint32_t crc32 = mCrc32AtCreation;
	if (mGenerator)
		crc32 = mGenerator->CalculateCRC32(crc32);
	return crc32;
}

std::shared_ptr<const dungeon::Generator> CDungeonGeneratorCore::GetGenerator() const
{
	return mGenerator;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

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
					room->GetCenter() * parameter->GetGridSize().To3D(),
					room->GetExtent() * parameter->GetGridSize().To3D(),
					FColor::Magenta,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);

				UKismetSystemLibrary::DrawDebugSphere(
					world,
					room->GetGroundCenter() * parameter->GetGridSize().To3D(),
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
			if (!IsValid(world))
				return false;

			UKismetSystemLibrary::DrawDebugLine(
				world,
				*edge.GetPoint(0) * parameter->GetGridSize().To3D(),
				*edge.GetPoint(1) * parameter->GetGridSize().To3D(),
				FColor::Red,
				0.f,
				5.f
			);

			const FVector start(static_cast<int32>(edge.GetPoint(0)->X), static_cast<int32>(edge.GetPoint(0)->Y), static_cast<int32>(edge.GetPoint(0)->Z));
			const FVector goal(static_cast<int32>(edge.GetPoint(1)->X), static_cast<int32>(edge.GetPoint(1)->Y), static_cast<int32>(edge.GetPoint(1)->Z));
			UKismetSystemLibrary::DrawDebugSphere(
				world,
				start * parameter->GetGridSize().To3D() + (parameter->GetGridSize().To3D() / 2),
				10.f,
				12,
				FColor::Green,
				0.f,
				5.f
			);
			UKismetSystemLibrary::DrawDebugSphere(
				world,
				goal * parameter->GetGridSize().To3D() + (parameter->GetGridSize().To3D() / 2.),
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
					const FVector halfGrid = parameter->GetGridSize().To3D() / 2;
					UKismetSystemLibrary::DrawDebugBox(
						world,
						FVector(location.X, location.Y, location.Z) * parameter->GetGridSize().To3D() + halfGrid,
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

std::shared_ptr<dungeon::Random> CDungeonGeneratorCore::GetSynchronizedRandom() noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}

std::shared_ptr<dungeon::Random> CDungeonGeneratorCore::GetSynchronizedRandom() const noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}

const std::shared_ptr<dungeon::Random>& CDungeonGeneratorCore::GetRandom() noexcept
{
	return mLocalRandom;
}

const std::shared_ptr<dungeon::Random>& CDungeonGeneratorCore::GetRandom() const noexcept
{
	return mLocalRandom;
}
