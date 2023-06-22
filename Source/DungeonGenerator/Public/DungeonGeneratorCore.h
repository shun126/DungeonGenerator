/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorDecorator.h"
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomProps.h"
#include <EngineUtils.h>
#include <algorithm>
#include <functional>
#include <list>
#include <memory>

// Forward declaration
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
	~CDungeonGeneratorCore() = default;

	// event
	void OnAddFloor(const AddStaticMeshEvent& func);
	void OnAddSlope(const AddStaticMeshEvent& func);
	void OnAddWall(const AddStaticMeshEvent& func);
	void OnAddRoomRoof(const AddStaticMeshEvent& func);
	void OnAddAisleRoof(const AddStaticMeshEvent& func);
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
	Get dungeon generation core object.
	\return		dungeon::Generator
	*/
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

#if WITH_EDITOR
	void DrawDebugInformation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const;
#endif

private:
	bool CreateImpl_AddRoomAsset(const UDungeonGenerateParameter* parameter, const std::shared_ptr<dungeon::Room>& room);
	void AddTerrain();
	void AddObject();

	////////////////////////////////////////////////////////////////////////////
	// Interior

	////////////////////////////////////////////////////////////////////////////
	AActor* SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	void SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const;
	void SpawnDoorActor(UClass* actorClass, const FTransform& transform, EDungeonRoomProps props) const;
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

	template<typename T = AActor> T* FindActor();
	template<typename T = AActor> const T* FindActor() const;

	////////////////////////////////////////////////////////////////////////////
	bool IsStreamLevelRequested(const FSoftObjectPath& levelPath) const;
	void RequestStreamLevel(const FSoftObjectPath& levelPath, const FVector& levelLocation);
	void AsyncLoadStreamLevels();
#if WITH_EDITOR
	void SyncLoadStreamLevels();
#endif
	void UnloadStreamLevels();
	TSoftObjectPtr<const ULevelStreamingDynamic> FindLoadedStreamLevel(const FSoftObjectPath& levelPath) const;

	void LoadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const FTransform& transform);
	void UnloadStreamLevelImplement(UWorld* world, const FSoftObjectPath& path, const bool shouldBlockOnUnload);

	////////////////////////////////////////////////////////////////////////////
	UTexture2D* GenerateMiniMapTexture(uint32_t worldToTextureScale, uint32_t textureWidthHeight, uint32_t currentLevel) const;

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
	AddStaticMeshEvent mOnAddRoomRoof;
	AddStaticMeshEvent mOnAddAisleRoof;
	AddPillarStaticMeshEvent mOnResetPillar;

	ResetActorEvent mOnResetTorch;
	ResetDoorEvent mOnResetDoor;

	FDungeonInteriorDecorator mAisleInteriorDecorator;
	FDungeonInteriorDecorator mSlopeInteriorDecorator;
	FDungeonInteriorDecorator mRoomInteriorDecorator;

	struct LoadStreamLevelParameter
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

inline void CDungeonGeneratorCore::OnAddRoomRoof(const AddStaticMeshEvent& func)
{
	mOnAddRoomRoof = func;
}

inline void CDungeonGeneratorCore::OnAddAisleRoof(const AddStaticMeshEvent& func)
{
	mOnAddAisleRoof = func;
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
