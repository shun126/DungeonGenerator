/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.

ADungeonActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
ADungeonGenerateActorは配置可能(Placeable)、ADungeonActorは配置不可能(NotPlaceable)にするため、
継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
*/

#include "DungeonGenerateBase.h"

#include <FoliageInstancedStaticMeshComponent.h>

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
#include "PluginInformation.h"
#include <TextureResource.h>
#include <Components/StaticMeshComponent.h>
#include <GameFramework/PlayerStart.h>
#include <Engine/LevelStreamingDynamic.h>
#include <Engine/PlayerStartPIE.h>
#include <Engine/StaticMeshActor.h>
#include <Engine/Texture2D.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <NavMesh/NavMeshBoundsVolume.h>
#include <NavMesh/RecastNavMesh.h>

#include <Components/BrushComponent.h>
#include <Engine/Polys.h>
#include <UObject/Package.h>

#include <algorithm>
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
	bool operator==(const EDungeonRoomItem left, const dungeon::Room::Item right)
	{
		return static_cast<uint8_t>(left) == static_cast<uint8_t>(right);
	}
}

ADungeonGenerateBase::ADungeonGenerateBase(const FObjectInitializer& initializer)
	: Super(initializer)
{
	// Create root scene component
	RootComponent = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("Scene"), true);
	check(RootComponent);

	const AddStaticMeshEvent addFloorStaticMeshEvent = [this](UStaticMesh* staticMesh, const FTransform& transform)
		{
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Floor"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Slope"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Wall"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
			AStaticMeshActor* actor = SpawnStaticMeshActor(staticMesh, TEXT("Meshes/Roof"), transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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

/*
同期用の乱数を取得します。必ずサーバーとクライアントが同じ回数呼び出すようにして下さい。
*/
std::shared_ptr<dungeon::Random> ADungeonGenerateBase::GetSynchronizedRandom() const noexcept
{
	return mGenerator->GetGenerateParameter().GetRandom();
}

/*
ローカル用の乱数を取得します。レプリケーションで同期する事を想定しています。
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
			component = NewObject<UDungeonComponentActivatorComponent>(actor, TEXT("ComponentActivator"));
			actor->AddInstanceComponent(component);
			component->RegisterComponent();
		}
	}
	return component;
}

AActor* ADungeonGenerateBase::SpawnActorImpl(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters)
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
アクターをスポーンします。
DungeonGeneratorというタグを追加します。
スポーンしたアクターはDestroySpawnedActorsで破棄されます。
*/
AActor* ADungeonGenerateBase::SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const
{
	UWorld* world = GetWorld();
	if (IsValid(world) == false)
		return nullptr;

	return SpawnActorImpl(world, actorClass, folderPath, transform, actorSpawnParameters);
}

/*
スポーンしたアクターを全て破棄します
DungeonGeneratorというタグが付いたアクターが対象です。
*/
void ADungeonGenerateBase::DestroySpawnedActors() const
{
	DestroySpawnedActors(GetWorld());
}

/*
スポーンしたアクターを全て破棄します
DungeonGeneratorというタグが付いたアクターが対象です。
*/
void ADungeonGenerateBase::DestroySpawnedActors(UWorld* world)
{
	if (!IsValid(world))
		return;

	TArray<AActor*> actors;
	UGameplayStatics::GetAllActorsWithTag(world, GetDungeonGeneratorTag(), actors);

#if WITH_EDITOR
	TArray<FFolder> deleteFolders;

	// TagにDungeonGeneratorTagがついているアクターのフォルダを回収
	for (const AActor* actor : actors)
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

std::shared_ptr<const dungeon::Generator> ADungeonGenerateBase::GetGenerator() const
{
	return mGenerator;
}

void ADungeonGenerateBase::OnPreDungeonGeneration()
{
}

void ADungeonGenerateBase::OnPostDungeonGeneration(const bool result)
{
}














bool ADungeonGenerateBase::IsCreated() const noexcept
{
	return mCreated;
}

void ADungeonGenerateBase::Dispose(const bool flushStreamLevels)
{
	// 生成済みなら破棄する
	if (mCreated == true)
	{

		// スポーン済みアクターを破棄
		DestroySpawnedActors();

		// ジェネレータを解放
		mGenerator.reset();

		// 生成したパラメータを解放
		mParameter = nullptr;

		// 生成済みフラグをリセットする
		mCreated = false;
	}
}

/*
hasAuthorityによって処理を分岐する場合は、乱数の同期が確実に行われている事に注意して実装して下さい。
例えばリプリケートするアクターはサーバー側でのみ実行されるため乱数の同期ずれが発生します。
*/
bool ADungeonGenerateBase::Create(const UDungeonGenerateParameter* parameter, const bool hasAuthority)
{
	check(mCreated == false);
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
	if (IsValid(parameter) == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the dungeon generation parameters"));
		return false;
	}

	// UDungeonGenerateParameterを保存
	mParameter = parameter;


	// ダンジョン生成パラメータを生成
	dungeon::GenerateParameter generateParameter;
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
		generateParameter.SetNumberOfCandidateRooms(mParameter->NumberOfCandidateRooms);
		generateParameter.SetMinRoomWidth(mParameter->RoomWidth.Min);
		generateParameter.SetMaxRoomWidth(mParameter->RoomWidth.Max);
		generateParameter.SetMinRoomDepth(mParameter->RoomDepth.Min);
		generateParameter.SetMaxRoomDepth(mParameter->RoomDepth.Max);
		generateParameter.SetMinRoomHeight(mParameter->RoomHeight.Min);
		generateParameter.SetMaxRoomHeight(mParameter->RoomHeight.Max);
		generateParameter.SetMergeRooms(mParameter->MergeRooms);
		generateParameter.SetMissionGraph(mParameter->IsUseMissionGraph());
		generateParameter.SetAisleComplexity(mParameter->GetAisleComplexity());
		generateParameter.SetGenerateSlopeInRoom(mParameter->GenerateSlopeInRoom);
		if (mParameter->MergeRooms)
		{
			generateParameter.SetHorizontalRoomMargin(0);
			generateParameter.SetVerticalRoomMargin(0);
			generateParameter.SetNumberOfCandidateFloors(0);
			generateParameter.SetMissionGraph(false);
			generateParameter.SetAisleComplexity(0);
		}
		else
		{
			generateParameter.SetHorizontalRoomMargin(mParameter->RoomMargin);
			if (mParameter->Flat)
			{
				generateParameter.SetVerticalRoomMargin(0);
				generateParameter.SetNumberOfCandidateFloors(0);
			}
			else
			{
				generateParameter.SetVerticalRoomMargin(mParameter->VerticalRoomMargin);
				generateParameter.SetNumberOfCandidateFloors(mParameter->NumberOfCandidateFloors);
			}
		}


		check(generateParameter.GetMinRoomWidth() <= generateParameter.GetMaxRoomWidth());
		check(generateParameter.GetMinRoomDepth() <= generateParameter.GetMaxRoomDepth());
		check(generateParameter.GetMinRoomHeight() <= generateParameter.GetMaxRoomHeight());
	}

	// クライアント用乱数生成器を初期化
	mLocalRandom = std::make_shared<dungeon::Random>(generateParameter.GetRandom()->Get<uint32_t>());

	// ダンジョン生成コアの初期設定
	mGenerator = std::make_shared<dungeon::Generator>();
	if (mGenerator == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("System initialization failed."));
		return false;
	}
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
	dungeon::Generator::Error generatorError = mGenerator->GetLastError();
	OnPostDungeonGeneration(dungeon::Generator::Error::Success == generatorError);

	// 生成エラーを確認する
	if (dungeon::Generator::Error::Success != generatorError)
	{
#if WITH_EDITOR
		// デバッグに必要な情報（デバッグ生成パラメータ）を出力する
		//mParameter->DumpToJson();
#endif
		return false;
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

	// メッシュの生成
	{
		RoomAndRoomSensorMap roomSensorCache;
		CreateImplement_PrepareSpawnRoomSensor(roomSensorCache);
		CreateImplement_AddTerrain(roomSensorCache, hasAuthority);
		CreateImplement_FinishSpawnRoomSensor(roomSensorCache);
		CreateImplement_AddWall();
		CreateImplement_Navigation(hasAuthority);
	}


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

	mCreated = true;
	return true;
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
					BEGIN_STOPWATCH();
					CreateImplement_AddFloorAndSlope(createImplementParameter);
					END_STOPWATCH(floorAndSlopeStopwatch);
				}
#if 1
				// Generate wall mesh
				{
					BEGIN_STOPWATCH();
					CreateImplement_ReserveWall(createImplementParameter);
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
#endif
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
		/*
		スロープのメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		//if (cp.mGrid.IsKindOfRoomType())
		if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
			dungeonMeshSetDatabase = mParameter->GetDungeonRoomMeshPartsDatabase();
		else
			dungeonMeshSetDatabase = mParameter->GetDungeonAisleMeshPartsDatabase();
		if (dungeonMeshSetDatabase)
		{
			if (const FDungeonMeshParts* parts = mParameter->SelectSlopeParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddSlope(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
			}
		}
		else
		{
			if (const FDungeonMeshParts* parts = mParameter->SelectSlopeParts(cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddSlope(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
			}
		}
	}
	else if (mOnAddFloor && cp.mGrid.CanBuildFloor(true))
	{
		/*
		床のメッシュを生成
		メッシュは原点からX軸とY軸方向に伸びており、面はZ軸が上面になっています。
		*/
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		//if (cp.mGrid.IsKindOfRoomType())
		if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
			dungeonMeshSetDatabase = mParameter->GetDungeonRoomMeshPartsDatabase();
		else
			dungeonMeshSetDatabase = mParameter->GetDungeonAisleMeshPartsDatabase();
		if (dungeonMeshSetDatabase)
		{
			if (cp.mGrid.IsCatwalk())
			{
				if (const FDungeonMeshParts* parts = mParameter->SelectCatwalkParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
				{
					mOnAddCatwalk(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetCatwalkDirection()));
				}
			}
			else
			{
				if (const FDungeonMeshParts* parts = mParameter->SelectFloorParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
				{
					mOnAddFloor(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
				}
			}
		}
		else
		{
			const UDungeonTemporaryMeshSetDatabase* dungeonTemporaryMeshSetDatabase;
			if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonRoomPartsDatabase();
			else
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonAislePartsDatabase();

			if (const FDungeonMeshParts* parts = mParameter->SelectFloorParts(dungeonTemporaryMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddFloor(parts->StaticMesh, parts->CalculateWorldTransform(cp.mCenterPosition, cp.mGrid.GetDirection()));
			}
		}
	}
}

/*
壁はレプリケーションする必要が無いのでサーバーとクライアント両方でアクターをスポーンする。
*/
void ADungeonGenerateBase::CreateImplement_ReserveWall(const CreateImplementParameter& cp)
{
	/*
	壁のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面（北側の壁）になっています。
	*/
	const FDungeonMeshSet* meshSet = nullptr;
	const FDungeonMeshParts* parts = nullptr;
	EDungeonPartsSelectionMethod dungeonPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
			dungeonMeshSetDatabase = mParameter->GetDungeonRoomMeshPartsDatabase();
		else
			dungeonMeshSetDatabase = mParameter->GetDungeonAisleMeshPartsDatabase();
		if (dungeonMeshSetDatabase)
		{
			meshSet = mParameter->SelectParts(dungeonMeshSetDatabase, cp.mGrid);
			if (meshSet != nullptr)
				dungeonPartsSelectionMethod = meshSet->GetWallPartsSelectionMethod();

			// グリッドによるパーツ選択を行う場合はここで抽選する
			if (dungeonPartsSelectionMethod != EDungeonPartsSelectionMethod::GridIndex)
				parts = mParameter->SelectWallPartsByGrid(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom());
		}
		else
		{
			const UDungeonTemporaryMeshSetDatabase* dungeonTemporaryMeshSetDatabase;
			if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonRoomPartsDatabase();
			else
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonAislePartsDatabase();
			parts = mParameter->SelectWallParts(dungeonTemporaryMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom());
		}
	}

	if (dungeonPartsSelectionMethod == EDungeonPartsSelectionMethod::GridIndex || parts != nullptr)
	{
		// 北側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z), dungeon::Direction::North, mParameter->IsMergeRooms()))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (dungeonPartsSelectionMethod == EDungeonPartsSelectionMethod::GridIndex)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::North));
			}

			FVector wallPosition = cp.mCenterPosition;
			wallPosition.Y -= cp.mGridHalfSize.Y;
			mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 0.f));

		}
		// 南側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z), dungeon::Direction::South, mParameter->IsMergeRooms()))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (dungeonPartsSelectionMethod == EDungeonPartsSelectionMethod::GridIndex)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::South));
			}

			FVector wallPosition = cp.mCenterPosition;
			wallPosition.Y += cp.mGridHalfSize.Y;
			mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 180.f));

		}
		// 東側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::East, mParameter->IsMergeRooms()))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (dungeonPartsSelectionMethod == EDungeonPartsSelectionMethod::GridIndex)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::East));
			}

			FVector wallPosition = cp.mCenterPosition;
			wallPosition.X += cp.mGridHalfSize.X;
			mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, 90.f));

		}
		// 西側の壁
		if (cp.mGrid.CanBuildWall(mGenerator->GetVoxel()->Get(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::West, mParameter->IsMergeRooms()))
		{
			// 面によるパーツ選択を行う場合はここで抽選する
			if (dungeonPartsSelectionMethod == EDungeonPartsSelectionMethod::GridIndex)
			{
				parts = mParameter->SelectWallPartsByFace(meshSet, cp.mGridLocation, dungeon::Direction(dungeon::Direction::West));
			}

			FVector wallPosition = cp.mCenterPosition;
			wallPosition.X -= cp.mGridHalfSize.X;
			mReservedWallInfo.emplace_back(parts->StaticMesh, parts->CalculateWorldTransform(wallPosition, -90.f));

		}
	}
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

	if (cp.mGrid.CanBuildRoof(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y, cp.mGridLocation.Z + 1), true))
	{
		const UDungeonMeshSetDatabase* dungeonMeshSetDatabase;
		//if (cp.mGrid.IsKindOfRoomType())
		if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
			dungeonMeshSetDatabase = mParameter->GetDungeonRoomMeshPartsDatabase();
		else
			dungeonMeshSetDatabase = mParameter->GetDungeonAisleMeshPartsDatabase();
		if (dungeonMeshSetDatabase)
		{
			/*
			壁のメッシュを生成
			メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
			*/
			const FTransform transform(cp.mCenterPosition);
			if (const FDungeonMeshPartsWithDirection* parts = mParameter->SelectRoofParts(dungeonMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddRoof(
					parts->StaticMesh,
					parts->CalculateWorldTransform(GetSynchronizedRandom(), transform)
				);
			}
		}
		else
		{
			const UDungeonTemporaryMeshSetDatabase* dungeonTemporaryMeshSetDatabase;
			//if (cp.mGrid.IsKindOfRoomType())
			if (dungeon::Identifier(cp.mGrid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false)
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonRoomPartsDatabase();
			else
				dungeonTemporaryMeshSetDatabase = mParameter->GetDungeonAislePartsDatabase();

			/*
			壁のメッシュを生成
			メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
			*/
			const FTransform transform(cp.mCenterPosition);
			if (const FDungeonMeshPartsWithDirection* parts = mParameter->SelectRoofParts(dungeonTemporaryMeshSetDatabase, cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
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
void ADungeonGenerateBase::CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const
{
	if (hasAuthority == false)
		return;

	if (CanAddDoor(dungeonRoomSensorBase, cp.mGridLocation, cp.mGrid))
	{
		if (const FDungeonDoorActorParts* parts = mParameter->SelectDoorParts(cp.mGridIndex, cp.mGrid, GetRandom()))
		{
			const EDungeonRoomProps props = static_cast<EDungeonRoomProps>(cp.mGrid.GetProps());
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y - 1, cp.mGridLocation.Z), dungeon::Direction::North, mParameter->IsMergeRooms()))
			{
				// 北側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 0.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X, cp.mGridLocation.Y + 1, cp.mGridLocation.Z), dungeon::Direction::South, mParameter->IsMergeRooms()))
			{
				// 南側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize * 0.5f;
				doorPosition.Y += mParameter->GetGridSize().HorizontalSize;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 180.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X + 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::East, mParameter->IsMergeRooms()))
			{
				// 東側の扉
				FVector doorPosition = cp.mPosition;
				doorPosition.X += mParameter->GetGridSize().HorizontalSize;
				doorPosition.Y += mParameter->GetGridSize().HorizontalSize * 0.5f;
				SpawnDoorActor(parts->ActorClass, parts->CalculateWorldTransform(doorPosition, 90.f), dungeonRoomSensorBase, props);
			}
			if (cp.mGrid.CanBuildGate(mGenerator->GetVoxel()->Get(cp.mGridLocation.X - 1, cp.mGridLocation.Y, cp.mGridLocation.Z), dungeon::Direction::West, mParameter->IsMergeRooms()))
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
void ADungeonGenerateBase::CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const
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

	/*
	柱のメッシュを生成
	メッシュは原点からY軸とZ軸方向に伸びており、面はX軸が正面になっています。
	*/
	FIntVector checkLocation = cp.mGridLocation;
	for (const auto& wallChecker : WallCheckers)
	{
		const FIntVector& direction = dungeon::Direction::GetVector(wallChecker.mDirection);
		const auto& fromGrid = mGenerator->GetVoxel()->Get(checkLocation);
		checkLocation += direction;
		const auto& toGrid = mGenerator->GetVoxel()->Get(checkLocation);

		// 柱必要か調べます
		if (fromGrid.CanBuildWall(toGrid, wallChecker.mDirection, mParameter->IsMergeRooms()) == true)
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
			if (mParameter->GetFrequencyOfTorchlightGeneration() >= EFrequencyOfGeneration::Occasionally)
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
			if (const FDungeonMeshParts* pillarParts = mParameter->SelectPillarParts(cp.mGridIndex, cp.mGrid, GetSynchronizedRandom()))
			{
				mOnAddPillar(pillarParts->StaticMesh, pillarParts->CalculateWorldTransform(rootTransform));
			}
		}

		// 燭台を生成（サーバーのみアクターをスポーンする）
		bool spawnTorchActor = false;
		switch (mParameter->GetFrequencyOfTorchlightGeneration())
		{
		case EFrequencyOfGeneration::Normally:
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

				if (const FDungeonActorParts* torchParts = mParameter->SelectTorchParts(cp.mGridIndex, cp.mGrid, GetRandom()))
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
void ADungeonGenerateBase::CreateImplement_PrepareSpawnRoomSensor(RoomAndRoomSensorMap& roomSensorCache) const
{
	check(IsValid(mParameter));

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	dungeon::Stopwatch stopwatch;
#endif

	// RoomSensorActorを生成
	mGenerator->ForEach([this, &roomSensorCache](const std::shared_ptr<const dungeon::Room>& room)
		{
			ADungeonRoomSensorBase* roomSensorActor = SpawnRoomSensorActorDeferred(
				mParameter->GetRoomSensorClass(),
				room->GetIdentifier(),
				room->GetCenter() * mParameter->GetGridSize().To3D() + GetActorLocation(),
				room->GetExtent() * mParameter->GetGridSize().To3D(),
				static_cast<EDungeonRoomParts>(room->GetParts()),
				static_cast<EDungeonRoomItem>(room->GetItem()),
				room->GetBranchId(),
				room->GetDepthFromStart(),
				mGenerator->GetDeepestDepthFromStart()
			);
			roomSensorCache[room.get()] = roomSensorActor;
		}
	);

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	DUNGEON_GENERATOR_LOG(TEXT("Prepare spawn DungeonRoomSensor actors: %lf seconds"), stopwatch.Lap());
#endif
}

/*
ADungeonRoomSensorBaseはリプリケートされない前提のアクターなので
必ず同期乱数(GetSynchronizedRandom)を使ってください。
*/
void ADungeonGenerateBase::CreateImplement_FinishSpawnRoomSensor(const RoomAndRoomSensorMap& roomSensorCache)
{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	dungeon::Stopwatch stopwatch;
#endif

	for (const auto& roomSensorActor : roomSensorCache)
	{
		if (IsValid(roomSensorActor.second))
		{
			FinishRoomSensorActorSpawning(roomSensorActor.second);
		}
	}

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	DUNGEON_GENERATOR_LOG(TEXT("Finish spawn DungeonRoomSensor actors: %lf seconds"), stopwatch.Lap());
#endif
}


void ADungeonGenerateBase::CreateImplement_Navigation(const bool hasAuthority)
{
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

	// NavMeshBoundsVolumeをフィットさせる
	FitNavMeshBoundsVolume();
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
		const FVector& boundingCenter = bounding.GetCenter() + GetActorLocation();
		const FVector& boundingExtent = bounding.GetExtent();

		if (USceneComponent* rootComponent = navMeshBoundsVolume->GetRootComponent())
		{
			const EComponentMobility::Type mobility = rootComponent->Mobility;
			rootComponent->SetMobility(EComponentMobility::Movable);

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
	// TODO: メニュー内の「ここから開始」で問題が起きるかもしれません
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

	if (IsValid(mParameter) == false)
		return;

	// PlayerStartをスタート位置へ移動しないならここで終了します
	if (mParameter->IsMovePlayerStartToStartingPoint() == false)
		return;

	// Calculate the position to shift APlayerStart
	const double halfHorizontalSize = mParameter->GetGridSize().HorizontalSize / 2;
	const double halfVerticalSize = mParameter->GetGridSize().VerticalSize / 2;
	const double placementRadius = halfHorizontalSize;
	const double placementAngle = (3.1415926535897932384626433832795 * 2.) / static_cast<double>(startPoints.Num());

	const FVector& startRoomLocation = GetStartLocation();
	const FVector& goalRoomLocation = GetGoalLocation();

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
				FVector location = startRoomLocation;
				location.X += placementRadius * std::cos(placementAngle * static_cast<double>(index));
				location.Y += placementRadius * std::sin(placementAngle * static_cast<double>(index));

				// Grounding the APlayerStart
				FHitResult hitResult;
				const FVector startLocation = location + FVector(0, 0, halfVerticalSize);
				const FVector endLocation = location - FVector(0, 0, halfVerticalSize);
				if (playerStart->GetWorld()->LineTraceSingleByChannel(hitResult, startLocation, endLocation, ECollisionChannel::ECC_Pawn))
				{
					location = hitResult.ImpactPoint;
				}
				location.Z += cylinderHalfHeight + heightMargin;

				// ゴールの部屋の方向を向く
				auto rotator = playerStart->GetActorRotation();
				rotator.Yaw = dungeon::math::ToDegree(
						std::atan2(
							goalRoomLocation.Y - location.Y,
							goalRoomLocation.X - location.X
						)
					);

				playerStart->SetActorTransform(FTransform(rotator, location));

#if WITH_EDITOR
				FCollisionShape collisionShape;
				collisionShape.SetCapsule(cylinderRadius, cylinderHalfHeight);
				if (playerStart->GetWorld()->OverlapBlockingTestByChannel(location, playerStart->GetActorQuat(), ECollisionChannel::ECC_Pawn, collisionShape))
				{
					DUNGEON_GENERATOR_ERROR(TEXT("PlayerStart(%s: %f, %f, %f) is in contact with something")
						, *playerStart->GetName()
						, location.X
						, location.Y
						, location.Z
					);
				}
#endif
			}
			// Undo EComponentMobility
			rootComponent->SetMobility(mobility);
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("PlayerStart's RootComponent was not set and could not be moved (%s)"), *playerStart->GetName());
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/*
StaticMeshActorを使って地形をスポーンします。
生成したアクターにDungeonComponentActivatorComponentを追加して処理負荷制御を行います。
CRC32の計算を行うのでサーバーとクライアントの同期ずれを検出する事ができます。
*/
AStaticMeshActor* ADungeonGenerateBase::SpawnStaticMeshActor(UStaticMesh* staticMesh, const FString& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AStaticMeshActor* actor = SpawnActorDeferredImpl<AStaticMeshActor>(folderPath, transform, nullptr, spawnActorCollisionHandlingMethod);
	if (IsValid(actor) == false)
		return nullptr;

	UStaticMeshComponent* staticMeshComponent = actor->GetStaticMeshComponent();
	if (IsValid(staticMeshComponent) == true)
	{
		if (const UWorld* world = actor->GetWorld())
		{
			if (world->HasBegunPlay() == true)
				actor->SetMobility(EComponentMobility::Movable);
		}

		staticMeshComponent->SetStaticMesh(staticMesh);
		staticMeshComponent->ComponentTags.AddUnique(GetDungeonGeneratorTerrainTag());
	}

	// 負荷制御コンポーネントを追加する
	if (auto* dungeonComponentActivatorComponent = FindOrAddComponentActivatorComponent(actor))
		dungeonComponentActivatorComponent->SetEnableCollisionEnableControl(false);

	actor->FinishSpawning(transform, true);

	// CRC32を記録（必ずサーバーとクライアント両方で計算しないとCRC32が一致しなくなる）
	mCrc32AtCreation = ADungeonVerifiableActor::GenerateCrc32(transform, mCrc32AtCreation);

	return actor;
}

/*
DungeonDoorBaseをスポーンします。
*/
ADungeonDoorBase* ADungeonGenerateBase::SpawnDoorActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, EDungeonRoomProps props) const
{
	ADungeonDoorBase* actor = SpawnActorDeferredImpl<ADungeonDoorBase>(actorClass, TEXT("Actors/Doors"), transform, ownerActor, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(actor))
	{
		actor->InvokeInitialize(GetRandom(), props);
		actor->FinishSpawning(transform, true);

		// 負荷制御コンポーネントを追加する
		FindOrAddComponentActivatorComponent(actor);

		if (IsValid(ownerActor))
			ownerActor->AddDungeonDoor(actor);
	}
	return actor;
}

/*
燭台アクターをスポーンします。
*/
AActor* ADungeonGenerateBase::SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	AActor* actor = SpawnActorImpl(actorClass, TEXT("Actors/Torches"), transform, actorSpawnParameters);

	// 負荷制御コンポーネントを追加する
	FindOrAddComponentActivatorComponent(actor);

	if (IsValid(ownerActor))
		ownerActor->AddDungeonTorch(actor);

	return actor;
}

/*
DungeonRoomSensorBaseをスポーンします

ADungeonRoomSensorBaseはリプリケートされる前提のアクターなので
同期乱数(GetSynchronizedRandom)を使ってはならない。
*/
ADungeonRoomSensorBase* ADungeonGenerateBase::SpawnRoomSensorActorDeferred(UClass* actorClass, const dungeon::Identifier& identifier, const FVector& center, const FVector& extent, EDungeonRoomParts parts, EDungeonRoomItem item, uint8 branchId, const uint8 depthFromStart, const uint8 deepestDepthFromStart) const
{
	const FTransform transform(center);
	ADungeonRoomSensorBase* actor = SpawnActorDeferredImpl<ADungeonRoomSensorBase>(actorClass, TEXT("Sensors"), transform, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
void ADungeonGenerateBase::FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor)
{
	if (IsValid(dungeonRoomSensor))
	{
		dungeonRoomSensor->InvokeInitialize();
		dungeonRoomSensor->FinishSpawning(FTransform::Identity, true);
	}
};

FTransform ADungeonGenerateBase::GetStartTransform() const
{
	return FTransform(GetStartLocation());
}

FTransform ADungeonGenerateBase::GetGoalTransform() const
{
	return FTransform(GetGoalLocation());
}

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
		boundingBox.Min.Z -= mParameter->GetGridSize().VerticalSize;
		boundingBox.Max.Z += mParameter->GetGridSize().VerticalSize;
		return boundingBox;
	}

	return FBox(EForceInit::ForceInitToZero);
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
