/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGridSize.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include <CoreMinimal.h>
#include <EngineUtils.h>
#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

// Forward declaration
class ADungeonDoorBase;
class ADungeonRoomSensorBase;
class UDungeonGenerateParameter;
class ANavMeshBoundsVolume;
class APlayerStart;
class AStaticMeshActor;
class ULevelStreamingDynamic;
class UStaticMesh;

namespace dungeon
{
	class Identifier;
	class Generator;
	class Grid;
	class Random;
	class Room;
	class Voxel;
}

/**
Dungeon generate
ダンジョン生成
*/
class DUNGEONGENERATOR_API CDungeonGeneratorCore final : public std::enable_shared_from_this<CDungeonGeneratorCore>
{
public:
	using AddStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;
	using AddPillarStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;
	using ResetActorEvent = std::function<void(UClass*, const FTransform&)>;

public:
	/**
	Get tag name
	*/
	static const FName& GetDungeonGeneratorTag();

public:
	/**
	constructor
	*/
	explicit CDungeonGeneratorCore(const TWeakObjectPtr<UWorld>& world);
	CDungeonGeneratorCore(const CDungeonGeneratorCore&) = delete;
	CDungeonGeneratorCore& operator=(const CDungeonGeneratorCore&) = delete;

	/**
	destructor
	*/
	~CDungeonGeneratorCore();

	// event
	void OnAddFloor(const AddStaticMeshEvent& func);
	void OnAddSlope(const AddStaticMeshEvent& func);
	void OnAddWall(const AddStaticMeshEvent& func);
	void OnAddRoof(const AddStaticMeshEvent& func);
	void OnAddPillar(const AddPillarStaticMeshEvent& func);

	/**
	Generate dungeon
	@param[in]	parameter		UDungeonGenerateParameter
	@param[in]	origin			原点にする座標
	@param[in]	hasAuthority	HasAuthority
	@return		If false, generation fails
	*/
	bool Create(const UDungeonGenerateParameter* parameter, const FVector& origin, const bool hasAuthority);

	/**
	Clear added terrain and objects
	*/
	void Clear();

	/**
	Get start position
	@return		Coordinates of start position
	*/
	FVector GetStartLocation() const;

	/**
	Get start Transform
	@return		Transform of starting position
	*/
	FTransform GetStartTransform() const;

	/**
	Get goal position
	@return		Goal position coordinates
	*/
	FVector GetGoalLocation() const;

	/**
	Get goal Transform
	@return		Transform of goal position
	*/
	FTransform GetGoalTransform() const;

	/**
	Get a bounding box covering the entire generated dungeon
	@return		bounding box
	*/
	FBox CalculateBoundingBox() const;

	/**
	Move APlayerStart to the starting point
	*/
	void MovePlayerStart(TArray<APlayerStart*>& startPoints);

	void MovePlayerStartPIE(const FVector& origin, const TArray<APlayerStart*>& startPoints);


	/**
	Calculate CRC32
	@return		CRC32
	*/
	uint32_t CalculateCRC32() const noexcept;

	/**
	Get dungeon generation core object.
	@return		dungeon::Generator
	*/
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

#if WITH_EDITOR
	void DrawDebugInformation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const;
#endif

private:
	////////////////////////////////////////////////////////////////////////////
	// Terrain
	void CreateImplement_AddTerrain(const FVector& origin, std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache, const bool hasAuthority);
		struct CreateImplementParameter final
	{
		const UDungeonGenerateParameter* mParameter;
		const FIntVector& mGridLocation;
		size_t mGridIndex;
		const dungeon::Grid& mGrid;
		const FVector& mPosition;
		const FVector& mGridSize;
		const FVector& mGridHalfSize;
		const FVector& mCenterPosition;
	};
	void CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp);
	void CreateImplement_AddWall(const CreateImplementParameter& cp);
	void CreateImplement_AddRoof(const CreateImplementParameter& cp);
	void CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority);
	void CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority);
	bool CanAddDoor(ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const;

	////////////////////////////////////////////////////////////////////////////
	// Room sensor
	void CreateImplement_PrepareSpawnRoomSensor(const FVector& origin, std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache);
	void CreateImplement_FinishSpawnRoomSensor(std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>& roomSensorCache);

	////////////////////////////////////////////////////////////////////////////
	// Interior

	////////////////////////////////////////////////////////////////////////////
	// Navigation
	void CreateImplement_Navigation(const FVector& origin);


	////////////////////////////////////////////////////////////////////////////
	// アクターのスポーンと破棄
	static AActor* SpawnActorImpl(UWorld* world, UClass* actorClass, const FName& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters);

	AActor* SpawnActorImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const;
	template<typename T = AActor> T* SpawnActorImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	AActor* SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const;

	ADungeonDoorBase* SpawnDoorActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const EDungeonRoomProps props) const;

	AActor* SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	ADungeonRoomSensorBase* SpawnRoomSensorActorDeferred(
		UClass* actorClass,
		const dungeon::Identifier& identifier,
		const FVector& center,
		const FVector& extent,
		EDungeonRoomParts parts,
		EDungeonRoomItem item,
		uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart) const;
	static void FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor);

	void DestroySpawnedActors() const;
	static void DestroySpawnedActors(UWorld* world);

	void SpawnRecastNavMesh();

	////////////////////////////////////////////////////////////////////////////
	// アクターの検索と更新
	template<typename T = AActor> T* FindActor();
	template<typename T = AActor> const T* FindActor() const;
	template<typename T = AActor> void EachActors(std::function<bool(T*)> function);
	template<typename T = AActor> void EachActors(std::function<bool(const T*)> function) const;

	////////////////////////////////////////////////////////////////////////////
	// Streaming Level

	////////////////////////////////////////////////////////////////////////////
	// ミニマップ

	////////////////////////////////////////////////////////////////////////////
	// 乱数
	std::shared_ptr<dungeon::Random> GetSynchronizedRandom() noexcept;
	std::shared_ptr<dungeon::Random> GetSynchronizedRandom() const noexcept;
	const std::shared_ptr<dungeon::Random>& GetRandom() noexcept;
	const std::shared_ptr<dungeon::Random>& GetRandom() const noexcept;

	////////////////////////////////////////////////////////////////////////////
	// デバッグ情報
#if WITH_EDITOR
	void DrawRoomAisleInformation() const;
	void DrawVoxelGridType() const;
#endif

private:
	TWeakObjectPtr<UWorld> mWorld;
	TWeakObjectPtr<const UDungeonGenerateParameter> mParameter;
	std::shared_ptr<dungeon::Generator> mGenerator;
	std::shared_ptr<dungeon::Random> mLocalRandom;

	AddStaticMeshEvent mOnAddFloor;
	AddStaticMeshEvent mOnAddSlope;
	AddStaticMeshEvent mOnAddWall;
	AddStaticMeshEvent mOnAddRoof;
	AddPillarStaticMeshEvent mOnAddPillar;


	// 生成時のCRC32
	mutable uint32_t mCrc32AtCreation = ~0;

	// friend class
	friend class ADungeonGenerateActor;
	friend class FDungeonGenerateEditorModule;
};

inline const FName& CDungeonGeneratorCore::GetDungeonGeneratorTag()
{
	static const FName DungeonGeneratorTag(TEXT("DungeonGenerator"));
	return DungeonGeneratorTag;
}

inline void CDungeonGeneratorCore::OnAddFloor(const AddStaticMeshEvent& func)
{
	mOnAddFloor = func;
}

inline void CDungeonGeneratorCore::OnAddSlope(const AddStaticMeshEvent& func)
{
	mOnAddSlope = func;
}

inline void CDungeonGeneratorCore::OnAddWall(const AddStaticMeshEvent& func)
{
	mOnAddWall = func;
}

inline void CDungeonGeneratorCore::OnAddRoof(const AddStaticMeshEvent& func)
{
	mOnAddRoof = func;
}

inline void CDungeonGeneratorCore::OnAddPillar(const AddPillarStaticMeshEvent& func)
{
	mOnAddPillar = func;
}

inline AActor* CDungeonGeneratorCore::SpawnActorImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const
{
	return SpawnActorImpl(mWorld.Get(), actorClass, folderPath, transform, actorSpawnParameters);
}

template<typename T>
inline T* CDungeonGeneratorCore::SpawnActorImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* CDungeonGeneratorCore::SpawnActorDeferredImpl(UClass* actorClass, const FName& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	actorSpawnParameters.bDeferConstruction = true;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* CDungeonGeneratorCore::FindActor()
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		const TActorIterator<T> iterator(world);
		if (iterator)
			return *iterator;
	}
	return nullptr;
}

template<typename T>
inline const T* CDungeonGeneratorCore::FindActor() const
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		const TActorIterator<T> iterator(world);
		if (iterator)
			return *iterator;
	}
	return nullptr;
}

template<typename T>
inline void CDungeonGeneratorCore::EachActors(std::function<bool(T*)> function)
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		for (TActorIterator<T> iterator(world); iterator; ++iterator)
		{
			if (!function(*iterator))
				break;
		}
	}
}

template<typename T>
inline void CDungeonGeneratorCore::EachActors(std::function<bool(const T*)> function) const
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		for (TActorIterator<const T> iterator(world); iterator; ++iterator)
		{
			if (!function(*iterator))
				break;
		}
	}
}
