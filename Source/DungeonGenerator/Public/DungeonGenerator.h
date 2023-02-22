/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomProps.h"
#include <EngineUtils.h>
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
	タグ名を取得します
	*/
	static const FName& GetDungeonGeneratorTag();

public:
	/**
	コンストラクタ
	*/
	CDungeonGenerator();

	/**
	デストラクタ
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
	パラメータを元にダンジョンを生成します
	\param[in]	asset	UDungeonGenerateParameter
	\return		falseならば生成に失敗
	*/
	bool Create(const UDungeonGenerateParameter* asset);

	/**
	地形を追加します
	*/
	void AddTerraine();

	/**
	物体を追加します
	*/
	void AddObject();

	/**
	追加した地形と物体をクリアします
	*/
	void Clear();

	/**
	スタート位置の座標を取得します
	\return		スタート位置の座標
	*/
	FVector GetStartLocation() const;

	/**
	スタート位置のトランスフォームを取得します
	\return		スタート位置のトランスフォーム
	*/
	FTransform GetStartTransform() const;

	/**
	ゴール位置の座標を取得します
	\return		ゴール位置の座標
	*/
	FVector GetGoalLocation() const;

	/**
	ゴール位置のトランスフォームを取得します
	\return		ゴール位置のトランスフォーム
	*/
	FTransform GetGoalTransform() const;

	/**
	生成したダンジョン全体を覆うバウンディングボックスを取得します
	\return			バウンディングボックス
	*/
	FBox CalculateBoundingBox() const;

	/**
	APlayerStartをスタート地点に移動します
	*/
	void MovePlayerStart();

	/**
	生成先ワールドを取得します
	*/	
	static UWorld* GetWorld();

	/**
	生成したダンジョンのミニマップテクスチャを生成します
	\param[out]		horizontalScale		ワールド座標からテクスチャ座標へ変換する為のスケール値
	\param[in]		textureWidthHeight	生成するテクスチャの幅（高さは幅と同じ）
	\param[in]		currentLevel		生成するフロアの高さの基準
	\return			テクスチャオブジェクト
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
