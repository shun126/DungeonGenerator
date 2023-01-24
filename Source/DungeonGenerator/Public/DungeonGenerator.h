/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomProps.h"
#include <functional>
#include <memory>

// Forward declaration
class UDungeonGenerateParameter;
class UStaticMesh;
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
	static const FName& GetDungeonGeneratorTag();

public:
	/*!
	コンストラクタ
	*/
	CDungeonGenerator();

	/*!
	デストラクタ
	*/
	virtual ~CDungeonGenerator() = default;

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

	void Create(const UDungeonGenerateParameter* asset);
	void AddTerraine();
	void AddObject();
	void Clear();

	FVector GetStartLocation() const;
	FTransform GetStartTransform() const;
	FVector GetGoalLocation() const;
	FTransform GetGoalTransform() const;

	FBox CalculateBoundingBox() const;




	static UWorld* GetWorld();


	UTexture2D* GenerateMiniMapTexture(uint32_t& horizontalScale, uint32_t textureWidth, uint32_t currentLevel, uint32_t lowerLevel) const;


	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

#if WITH_EDITOR
	void DrawDebugInfomation(const bool showRoomAisleInfomation, const bool showVoxelGridType) const;
#endif

private:
	static AActor* SpawnActor(UClass* actorClass, const FName& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
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
	static void DestorySpawnedActors();

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
