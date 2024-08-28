/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include "Parameter/DungeonGridSize.h"

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <EngineUtils.h>
#include <Containers/Array.h>

#include <algorithm>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>

#include "DungeonActor.generated.h"

// Forward declaration
class ADungeonDoorBase;
class ADungeonRoomSensorBase;
class ADungeonActor;
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
ダンジョン内の植生アクター
*/
UCLASS()
class DUNGEONGENERATOR_API ADungeonActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	Get tag name
	*/
	static const FName& GetDungeonGeneratorTag();

public:
	/**
	constructor
	コンストラクタ
	*/
	explicit ADungeonActor(const FObjectInitializer& initializer);

	/**
	destructor
	デストラクタ
	*/
	virtual ~ADungeonActor() override = default;

public:
	/**
	Generate dungeon
	@param[in]	parameter		UDungeonGenerateParameter
	@param[in]	hasAuthority	HasAuthority
	@return		If false, generation fails
	*/
	bool Create(const UDungeonGenerateParameter* parameter, const bool hasAuthority);

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

	////////////////////////////////////////////////////////////////////////////
	// 乱数
private:
	std::shared_ptr<dungeon::Random> GetSynchronizedRandom() const noexcept;
	const std::shared_ptr<dungeon::Random>& GetRandom() const noexcept;

	////////////////////////////////////////////////////////////////////////////
	// アクターのスポーンと破棄
public:
	/**
	アクターをスポーンします。
	DungeonGeneratorというタグを追加します。
	スポーンしたアクターはDestroySpawnedActorsで破棄されます。
	*/
	static AActor* SpawnActorImpl(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters);

private:
	AActor* SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const;
	template<typename T = AActor> T* SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	void DestroySpawnedActors() const;
	static void DestroySpawnedActors(UWorld* world);

	////////////////////////////////////////////////////////////////////////////
	// アクターの検索と更新
private:
	template<typename T = AActor> T* FindActor();
	template<typename T = AActor> const T* FindActor() const;
	template<typename T = AActor> void EachActors(const std::function<bool(T*)>& function);
	template<typename T = AActor> void EachActors(const std::function<bool(const T*)>& function) const;

	////////////////////////////////////////////////////////////////////////////
protected:
	/**
	Get dungeon generation core object.
	@return		dungeon::Generator
	*/
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

	////////////////////////////////////////////////////////////////////////////
	// Terrain
protected:
	// event
	using AddStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;
	using AddPillarStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;

	void OnAddFloor(const AddStaticMeshEvent& function);
	void OnAddSlope(const AddStaticMeshEvent& function);
	void OnAddWall(const AddStaticMeshEvent& function);
	void OnAddRoof(const AddStaticMeshEvent& function);
	void OnAddPillar(const AddPillarStaticMeshEvent& function);

private:
	using RoomAndRoomSensorMap = std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>;
	struct CreateImplementParameter final
	{
		const FIntVector& mGridLocation;
		size_t mGridIndex;
		const dungeon::Grid& mGrid;
		const FVector& mPosition;
		const FVector& mGridSize;
		const FVector& mGridHalfSize;
		const FVector& mCenterPosition;
	};
	void CreateImplement_AddTerrain(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const;
	void CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp) const;
	void CreateImplement_AddWall(const CreateImplementParameter& cp) const;
	void CreateImplement_AddRoof(const CreateImplementParameter& cp) const;
	void CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;
	bool CanAddDoor(const ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const;
	void CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;

	// Room sensor
	void CreateImplement_PrepareSpawnRoomSensor(RoomAndRoomSensorMap& roomSensorCache) const;
	static void CreateImplement_FinishSpawnRoomSensor(const RoomAndRoomSensorMap& roomSensorCache);

	// Navigation
	void CreateImplement_Navigation(const bool hasAuthority);
	void CheckRecastNavMesh() const;
	void FitNavMeshBoundsVolume();

protected:
	/**
	PlayerStartPIEアクターを除くPlayerStartアクターを収集してstartPointsに記録します
	*/
	void CollectPlayerStartExceptPlayerStartPIE(TArray<APlayerStart*>& startPoints);

	/**
	PlayerStartアクターを移動します
	*/
	void MovePlayerStart(const TArray<APlayerStart*>& startPoints);

private:
	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FString& folderPath, const FTransform& transform, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
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
	static ADungeonActor* SpawnDungeonActor(UWorld* world, const FVector& location);

	////////////////////////////////////////////////////////////////////////////
	// Streaming Level

	////////////////////////////////////////////////////////////////////////////
	// MiniMap
public:

	////////////////////////////////////////////////////////////////////////////
	// Interior
private:

	////////////////////////////////////////////////////////////////////////////
	// Vegetation
public:

	////////////////////////////////////////////////////////////////////////////
	// Debug
public:
	/**
	Calculate CRC32
	@return		CRC32
	*/
	uint32_t CalculateCRC32() const noexcept;

#if WITH_EDITOR
	void DrawDebugInformation(const bool showRoomAisleInformation, const bool showVoxelGridType) const;
#endif

private:
#if WITH_EDITOR
	void DrawRoomAisleInformation() const;
	void DrawVoxelGridType() const;
#endif

	////////////////////////////////////////////////////////////////////////////
	// member variables
	// Vegetation
protected:

	UPROPERTY(Transient)
	const UDungeonGenerateParameter* mParameter;

private:
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
	friend class FDungeonGenerateEditorModule;
};

inline const FName& ADungeonActor::GetDungeonGeneratorTag()
{
	static const FName DungeonGeneratorTag(TEXT("DungeonGenerator"));
	return DungeonGeneratorTag;
}

inline void ADungeonActor::OnAddFloor(const AddStaticMeshEvent& function)
{
	mOnAddFloor = function;
}

inline void ADungeonActor::OnAddSlope(const AddStaticMeshEvent& function)
{
	mOnAddSlope = function;
}

inline void ADungeonActor::OnAddWall(const AddStaticMeshEvent& function)
{
	mOnAddWall = function;
}

inline void ADungeonActor::OnAddRoof(const AddStaticMeshEvent& function)
{
	mOnAddRoof = function;
}

inline void ADungeonActor::OnAddPillar(const AddPillarStaticMeshEvent& function)
{
	mOnAddPillar = function;
}

template<typename T>
inline T* ADungeonActor::SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorImpl<T>(T::StaticClass(), folderPath, transform, ownerActor, spawnActorCollisionHandlingMethod);
}

template<typename T>
inline T* ADungeonActor::SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorImpl(T::StaticClass(), folderPath, transform, spawnActorCollisionHandlingMethod);
}

template<typename T>
inline T* ADungeonActor::SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* ADungeonActor::SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	actorSpawnParameters.bDeferConstruction = true;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* ADungeonActor::FindActor()
{
	UWorld* world = GetWorld();
	if (IsValid(world))
	{
		const TActorIterator<T> iterator(world);
		if (iterator)
			return *iterator;
	}
	return nullptr;
}

template<typename T>
inline const T* ADungeonActor::FindActor() const
{
	UWorld* world = GetWorld();
	if (IsValid(world))
	{
		const TActorIterator<T> iterator(world);
		if (iterator)
			return *iterator;
	}
	return nullptr;
}

template<typename T>
inline void ADungeonActor::EachActors(const std::function<bool(T*)>& function)
{
	UWorld* world = GetWorld();
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
inline void ADungeonActor::EachActors(const std::function<bool(const T*)>& function) const
{
	UWorld* world = GetWorld();
	if (IsValid(world))
	{
		for (TActorIterator<const T> iterator(world); iterator; ++iterator)
		{
			if (!function(*iterator))
				break;
		}
	}
}
