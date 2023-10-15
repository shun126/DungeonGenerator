/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Decorator/DungeonInteriorDecorator.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include "SubLevel/DungeonRoomRegister.h"
#include <EngineUtils.h>
#include <algorithm>
#include <functional>
#include <list>
#include <memory>

// Forward declaration
class CDungeonInteriorUnplaceableBounds;
class UDungeonGenerateParameter;
class ULevelStreamingDynamic;
class UStaticMesh;
class ADungeonRoomSensor;
class ANavMeshBoundsVolume;
class AStaticMeshActor;

namespace dungeon
{
	class Identifier;
	class Generator;
	class Random;
	class Room;
}

/*
Dungeon generate class
*/
class DUNGEONGENERATOR_API CDungeonGeneratorCore final : public std::enable_shared_from_this<CDungeonGeneratorCore>
{
public:
	using AddStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;
	using AddPillarStaticMeshEvent = std::function<void(uint32_t, UStaticMesh*, const FTransform&)>;
	using ResetActorEvent = std::function<void(UClass*, const FTransform&)>;
	using ResetDoorEvent = std::function<void(AActor*, EDungeonRoomProps)>;

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
	void OnResetTorch(const ResetActorEvent& func);
	//void OnAddChandelier(const ResetActorEvent& func);
	void OnResetDoor(const ResetDoorEvent& func);


	/**
	Generate dungeon
	\param[in]	asset	UDungeonGenerateParameter
	\return		If false, generation fails
	*/
	bool Create(const UDungeonGenerateParameter* asset);

	/**
	Clear added terrain and objects
	*/
	void Clear();

	/**
	Get start position
	\return		Coordinates of start position
	*/
	FVector GetStartLocation() const;

	/**
	Get start Transform
	\return		Transform of starting position
	*/
	FTransform GetStartTransform() const;

	/**
	Get goal position
	\return		Goal position coordinates
	*/
	FVector GetGoalLocation() const;

	/**
	Get goal Transform
	\return		Transform of goal position
	*/
	FTransform GetGoalTransform() const;

	/**
	Get a bounding box covering the entire generated dungeon
	\return		bounding box
	*/
	FBox CalculateBoundingBox() const;

	/**
	Move APlayerStart to the starting point
	*/
	void MovePlayerStart();

	/**
	Generates a minimap texture of the generated dungeon
	\param[out]		worldToTextureScale		Scale value to convert from world coordinates to texture coordinates
	\param[in]		textureWidthHeight		Width of texture to be generated (height equals width)
	\param[in]		currentLevel			Criteria for floor height to be generated
	\return			texture object
	*/
	UTexture2D* GenerateMiniMapTextureWithSize(uint32_t& worldToTextureScale, uint32_t textureWidthHeight, uint32_t currentLevel) const;

	/**
	Generates a minimap texture of the generated dungeon
	\param[out]		worldToTextureScale		Scale value to convert from world coordinates to texture coordinates
	\param[in]		textureScale			Scale of texture to be generated (height equals width)
	\param[in]		currentLevel			Criteria for floor height to be generated
	\return			texture object
	*/
	UTexture2D* GenerateMiniMapTextureWithScale(uint32_t& worldToTextureScale, uint32_t textureScale, uint32_t currentLevel) const;

	/**
	Calculate CRC32
	\return		CRC32
	*/
	uint32_t CalculateCRC32() const noexcept;

	/**
	Get dungeon generation core object.
	\return		dungeon::Generator
	*/
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

#if WITH_EDITOR
	void DrawDebugInformation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const;
#endif

private:
	bool CreateImplement_AddRoomAsset(const UDungeonGenerateParameter* parameter, const std::shared_ptr<dungeon::Room>& room);
	bool CreateImplement_AddRoomAsset(const FDungeonRoomRegister& roomRegister, const std::shared_ptr<dungeon::Room>& room, const float gridSize);
	void CreateImplement_AddTerrain();
	void CreateImplement_AddFloorAndSlope(const UDungeonGenerateParameter* parameter, const FIntVector& location);
	void CreateImplement_AddWall(const UDungeonGenerateParameter* parameter, const FIntVector& location);
	void CreateImplement_AddPillarAndTorch(std::vector<FSphere>& spawnedTorchBounds, const UDungeonGenerateParameter* parameter, const FIntVector& location);
	void CreateImplement_AddDoor(const UDungeonGenerateParameter* parameter, const FIntVector& location);
	void CreateImplement_AddRoof(const UDungeonGenerateParameter* parameter, const FIntVector& location);

	////////////////////////////////////////////////////////////////////////////
	// Interior

	////////////////////////////////////////////////////////////////////////////
	AActor* SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	void SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const;
	void SpawnDoorActor(UClass* actorClass, const FTransform& transform, EDungeonRoomProps props) const;
	AActor* SpawnTorchActor(std::vector<FSphere>& spawnedTorchBounds, const FVector& wallNormal, UClass* actorClass, const FName& folderPath, const FTransform& transform, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	ADungeonRoomSensor* SpawnRoomSensorActor(
		UClass* actorClass,
		const dungeon::Identifier& identifier,
		const FVector& center,
		const FVector& extent,
		EDungeonRoomParts parts,
		EDungeonRoomItem item,
		uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart) const;

	void SpawnRecastNavMesh();

	void DestroySpawnedActors() const;
	static void DestroySpawnedActors(UWorld* world);

	////////////////////////////////////////////////////////////////////////////
	template<typename T = AActor> T* FindActor();
	template<typename T = AActor> const T* FindActor() const;

	////////////////////////////////////////////////////////////////////////////
	template<typename T = AActor> void EachActors(std::function<void(T*)> function);
	template<typename T = AActor> void EachActors(std::function<void(const T*)> function) const;

	////////////////////////////////////////////////////////////////////////////
	bool IsStreamLevelRequested(const FSoftObjectPath& levelPath) const;
	void RequestStreamLevel(const FSoftObjectPath& levelPath, const FVector& levelLocation);
	void FlushLoadStreamLevels();
	void AsyncLoadStreamLevels(const bool bShouldBlockOnLoad = false);
#if WITH_EDITOR
	void SyncLoadStreamLevels();
#endif
	void UnloadStreamLevels();

	void LoadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const FTransform& transform);
	void UnloadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const bool shouldBlockOnUnload);

	////////////////////////////////////////////////////////////////////////////
	UTexture2D* GenerateMiniMapTexture(uint32_t worldToTextureScale, uint32_t textureWidthHeight, uint32_t currentLevel) const;

	////////////////////////////////////////////////////////////////////////////
	std::shared_ptr<dungeon::Random> GetRandom() noexcept;
	std::shared_ptr<dungeon::Random> GetRandom() const noexcept;

	////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
	void DrawRoomAisleInformation() const;
	void DrawVoxelGridType() const;
#endif

private:
	TWeakObjectPtr<UWorld> mWorld;
	TWeakObjectPtr<const UDungeonGenerateParameter> mParameter;
	std::shared_ptr<dungeon::Generator> mGenerator;

	AddStaticMeshEvent mOnAddFloor;
	AddStaticMeshEvent mOnAddSlope;
	AddStaticMeshEvent mOnAddWall;
	AddStaticMeshEvent mOnAddRoof;
	AddPillarStaticMeshEvent mOnResetPillar;

	ResetActorEvent mOnResetTorch;
	ResetDoorEvent mOnResetDoor;

	std::shared_ptr<CDungeonInteriorUnplaceableBounds> mInteriorUnplaceableBounds;
	FDungeonInteriorDecorator mAisleInteriorDecorator;
	FDungeonInteriorDecorator mSlopeInteriorDecorator;
	FDungeonInteriorDecorator mRoomInteriorDecorator;

	struct LoadStreamLevelParameter final
	{
		FSoftObjectPath mPath;
		FVector mLocation;

		LoadStreamLevelParameter() = default;
		LoadStreamLevelParameter(const FSoftObjectPath& path, const FVector& location)
			: mPath(path), mLocation(location)
		{
		}
	};

	std::list<LoadStreamLevelParameter> mRequestLoadStreamLevels;
	TArray<TSoftObjectPtr<ULevelStreamingDynamic>> mLoadedStreamLevels;


	// friend class
	friend class ADungeonGenerateActor;
	friend class FDungeonGenerateEditorModule;
};

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
	mOnResetPillar = func;
}

inline void CDungeonGeneratorCore::OnResetTorch(const ResetActorEvent& func)
{
	mOnResetTorch = func;
}

#if 0
inline void CDungeonGeneratorCore::OnAddChandelier(const ResetActorEvent& func)
{
	mOnResetChandelier = func;
}
#endif

inline void CDungeonGeneratorCore::OnResetDoor(const ResetDoorEvent& func)
{
	mOnResetDoor = func;
}

template<typename T>
inline T* CDungeonGeneratorCore::SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return Cast<T>(SpawnActor(T::StaticClass(), folderPath, transform, spawnActorCollisionHandlingMethod));
}

template<typename T>
inline T* CDungeonGeneratorCore::SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	UWorld* world = mWorld.Get();
	if (!IsValid(world))
		return nullptr;

	T* actor = world->SpawnActorDeferred<T>(actorClass, transform, nullptr, nullptr, spawnActorCollisionHandlingMethod);
	if (!IsValid(actor))
		return nullptr;

#if WITH_EDITOR
	actor->SetFolderPath(folderPath);
#endif

	actor->Tags.Add(GetDungeonGeneratorTag());

	return actor;
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
inline void CDungeonGeneratorCore::EachActors(std::function<void(T*)> function)
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		for (TActorIterator<T> iterator(world); iterator; ++iterator)
		{
			function(*iterator);
		}
	}
}

template<typename T>
inline void CDungeonGeneratorCore::EachActors(std::function<void(const T*)> function) const
{
	UWorld* world = mWorld.Get();
	if (IsValid(world))
	{
		for (TActorIterator<const T> iterator(world); iterator; ++iterator)
		{
			function(*iterator);
		}
	}
}
