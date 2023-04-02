/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomProps.h"
#include <EngineUtils.h>
#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include "DungeonGenerator.generated.h"

// Forward declaration
class UDungeonGenerateParameter;
class UStaticMesh;
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
UCLASS()
class DUNGEONGENERATOR_API UDungeonGenerator : public UObject
{
	GENERATED_BODY()

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
	UDungeonGenerator();

	/**
	destructor
	*/
	virtual ~UDungeonGenerator() = default;

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
	\param[out]		horizontalScale		Scale value to convert from world coordinates to texture coordinates
	\param[in]		textureWidthHeight	Width of texture to be generated (height equals width)
	\param[in]		currentLevel		Criteria for floor height to be generated
	\return			texture object
	*/
	UTexture2D* GenerateMiniMapTexture(uint32_t& horizontalScale, uint32_t textureWidthHeight, uint32_t currentLevel) const;

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
	UWorld* FindWorld() const;
	static UWorld* GetWorldFromGameViewport();

	////////////////////////////////////////////////////////////////////////////
	AActor* SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	template<typename T = AActor> T* SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;

	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn) const;
	void SpawnActorOnFloor(UClass* actorClass, const FTransform& transform) const;
	void SpawnDoorActor(UClass* actorClass, const FTransform& transform, EDungeonRoomProps props) const;
	void SpawnRoomSensorActor(
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
	bool RequestStreamLevel(const FSoftObjectPath& levelPath, const FVector& levelLocation);
	void AsyncLoadStreamLevels();
	void SyncLoadStreamLevels();
	void UnloadStreamLevels();
	void LoadStreamLevel(const FSoftObjectPath& levelPath, const FVector& levelLocation);
	void UnloadStreamLevel(const FSoftObjectPath& levelPath);

	////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
	void DrawRoomAisleInformation() const;
	void DrawVoxelGridType() const;
#endif

private:
	std::shared_ptr<dungeon::Generator> mGenerator;
	TWeakObjectPtr<const UDungeonGenerateParameter> mParameter;

	AddStaticMeshEvent mOnAddFloor;
	AddStaticMeshEvent mOnAddSlope;
	AddStaticMeshEvent mOnAddWall;
	AddStaticMeshEvent mOnAddRoomRoof;
	AddStaticMeshEvent mOnAddAisleRoof;
	AddPillarStaticMeshEvent mOnResetPillar;

	ResetActorEvent mOnResetTorch;
	//ResetActorEvent mOnResetChandelier;
	ResetDoorEvent mOnResetDoor;

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
	TSet<FSoftObjectPath> mLoadedStreamLevels;

	// friend class
	friend class ADungeonGenerateActor;
	friend class FDungeonGenerateEditorModule;
};

inline void UDungeonGenerator::OnAddFloor(const AddStaticMeshEvent& func)
{
	mOnAddFloor = func;
}

inline void UDungeonGenerator::OnAddSlope(const AddStaticMeshEvent& func)
{
	mOnAddSlope = func;
}

inline void UDungeonGenerator::OnAddWall(const AddStaticMeshEvent& func)
{
	mOnAddWall = func;
}

inline void UDungeonGenerator::OnAddRoomRoof(const AddStaticMeshEvent& func)
{
	mOnAddRoomRoof = func;
}

inline void UDungeonGenerator::OnAddAisleRoof(const AddStaticMeshEvent& func)
{
	mOnAddAisleRoof = func;
}

inline void UDungeonGenerator::OnAddPillar(const AddPillarStaticMeshEvent& func)
{
	mOnResetPillar = func;
}

inline void UDungeonGenerator::OnResetTorch(const ResetActorEvent& func)
{
	mOnResetTorch = func;
}

#if 0
inline void UDungeonGenerator::OnAddChandelier(const ResetActorEvent& func)
{
	mOnResetChandelier = func;
}
#endif

inline void UDungeonGenerator::OnResetDoor(const ResetDoorEvent& func)
{
	mOnResetDoor = func;
}

template<typename T>
inline T* UDungeonGenerator::SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	AActor* actor = SpawnActor(T::StaticClass(), folderPath, transform, spawnActorCollisionHandlingMethod);
	return Cast<T>(actor);
}

template<typename T>
inline T* UDungeonGenerator::SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	UWorld* world = GetWorld();
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
inline T* UDungeonGenerator::FindActor()
{
	const TActorIterator<T> iterator(GetWorld());
	return iterator ? *iterator : nullptr;
}

template<typename T>
inline const T* UDungeonGenerator::FindActor() const
{
	const TActorIterator<T> iterator(GetWorld());
	return iterator ? *iterator : nullptr;
}
