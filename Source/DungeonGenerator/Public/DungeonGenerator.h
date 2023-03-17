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
#include <memory>

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
class DUNGEONGENERATOR_API CDungeonGenerator
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
	CDungeonGenerator();

	/**
	destructor
	*/
	virtual ~CDungeonGenerator() = default;

	// event
	void OnQueryParts(std::function<void(const std::shared_ptr<const dungeon::Room>&)> func);
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
	\return		falseならば生成に失敗
	*/
	bool Create(const UDungeonGenerateParameter* asset);

	/**
	Add terrain
	*/
	void AddTerraine();

	/**
	Add an object
	*/
	void AddObject();

	/**
	Clear added terrain and objects
	*/
	void Clear();

	/**
	Get start position
	\return		スタート位置の座標
	*/
	FVector GetStartLocation() const;

	/**
	Get start Transform
	\return		スタート位置のトランスフォーム
	*/
	FTransform GetStartTransform() const;

	/**
	Get goal position
	\return		ゴール位置の座標
	*/
	FVector GetGoalLocation() const;

	/**
	Get goal Transform
	\return		ゴール位置のトランスフォーム
	*/
	FTransform GetGoalTransform() const;

	/**
	Get a bounding box covering the entire generated dungeon
	\return			バウンディングボックス
	*/
	FBox CalculateBoundingBox() const;

	/**
	Move APlayerStart to the starting point
	*/
	void MovePlayerStart();

	/**
	生成先ワールドを取得します
	*/	
	static UWorld* GetWorld();

	/**
	Generates a minimap texture of the generated dungeon
	\param[out]		horizontalScale		Scale value to convert from world coordinates to texture coordinates
	\param[in]		textureWidthHeight	Width of texture to be generated (height equals width)
	\param[in]		currentLevel		Criteria for floor height to be generated
	\return			texture object
	*/
	UTexture2D* GenerateMiniMapTexture(uint32_t& horizontalScale, uint32_t textureWidthHeight, uint32_t currentLevel) const;

	/**
	ダンジョン生成オブジェクトを取得します
	\return		dungeon::Generator
	*/
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

#if WITH_EDITOR
	void DrawDebugInfomation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const;
#endif

private:
	static AActor* SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	template<typename T>
	static T* SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	static AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	static void SpawnActorOnFloor(UClass* actorClass, const FTransform& transform);
	template<typename T = AActor>
	static T* SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod);
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
	static void DestorySpawnedActors();

	template<typename T = AActor>
	T* FindActor();

	template<typename T = AActor>
	const T* FindActor() const;

#if WITH_EDITOR
	void DrawRoomAisleInfomation() const;
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

	// friend class
	friend class ADungeonGenerateActor;
	friend class FDungeonGenerateEditorModule;
};

template<typename T>
inline T* CDungeonGenerator::SpawnActor(const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod)
{
	AActor* actor = SpawnActor(T::StaticClass(), folderPath, transform, spawnActorCollisionHandlingMethod);
	return Cast<T>(actor);
}

template<typename T>
inline T* CDungeonGenerator::SpawnActorDeferred(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod)
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
T* CDungeonGenerator::FindActor()
{
	const TActorIterator<T> iterator(GetWorld());
	return iterator ? *iterator : nullptr;
}

template<typename T>
const T* CDungeonGenerator::FindActor() const
{
	const TActorIterator<T> iterator(GetWorld());
	return iterator ? *iterator : nullptr;
}
