/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Helper/DungeonRandom.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include "Parameter/DungeonGridSize.h"

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <EngineUtils.h>
#include <Containers/Array.h>

#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

#include "DungeonGenerateBase.generated.h"

class UDungeonAisleGridMap;
// Forward declaration
class ADungeonDoorBase;
class ADungeonRoomSensorBase;
class ADungeonGenerateBase;
class UDungeonGenerateParameter;
class UDungeonComponentActivatorComponent;
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationSuccessSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationFailureSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGenerateBaseOnBeginGenerateSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonGenerateBaseOnEndGenerateSignature, UDungeonRandom*, synchronizedRandom, const UDungeonAisleGridMap*, aisleGridMap);

/**
 * This is a collection of dungeon generation functions common to both runtime and editor.
 * Cannot be spawned or placed because it is a virtual class.
 * Please use derived classes to generate instances.
 * 
 * ランタイム、エディタ共通のダンジョン生成機能をまとめています。
 * 仮想クラスなのでスポーンや配置をする事ができません。
 * インスタンスを生成する場合は派生クラスを利用して下さい。
 */
UCLASS(Abstract, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonGenerateBase : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Get tag name
	 * タグ名を取得します
	 */
	static const FName& GetDungeonGeneratorTag();

	/**
	 * Get tag names of actors and components for terrain
	 * 地形用アクターとコンポーネントのタグ名を取得します
	 */
	static const FName& GetDungeonGeneratorTerrainTag();

	/**
	 * constructor
	 * コンストラクタ
	 */
	explicit ADungeonGenerateBase(const FObjectInitializer& initializer);

	/**
	 * destructor
	 * デストラクタ
	 */
	virtual ~ADungeonGenerateBase() override = default;

protected:
	/**
	 * Begin Generate dungeon
	 * After generation is complete, be sure to call EndDungeonGeneration.
	 * 
	 * ダンジョン生成開始
	 * 生成完了後、必ずEndDungeonGenerationを呼び出してください。
	 * 
	 * @param[in]	parameter		UDungeonGenerateParameter
	 * @param[in]	hasAuthority	HasAuthority
	 * @return		If false, generation fails
	 */
	bool BeginDungeonGeneration(const UDungeonGenerateParameter* parameter, const bool hasAuthority);

	/**
	 * End Generate dungeon
	 * ダンジョン生成を終了
	 */
	void EndDungeonGeneration();

public:
	/**
	 * ダンジョンを生成済みか取得します
	 * @return trueなら生成済み
	 */
	bool IsGenerated() const noexcept;

	/**
	 * Dispose dungeon
	 */
	virtual void Dispose(const bool flushStreamLevels);

	/**
	 * Get start position
	 */
	FVector GetStartLocation() const;

	/**
	 * スタート部屋のバウンディングボックスを取得します
	 */
	FBox GetStartBoundingBox() const;

	/**
	 * Get goal position
	 */
	FVector GetGoalLocation() const;

	/**
	 * Get a bounding box covering the entire generated dungeon
	 */
	FBox CalculateBoundingBox() const;

	/**
	 * Get the length of the longest straight line
	 * 最も長い直線の長さを取得します
	 */
	FVector2D GetLongestStraightPath() const noexcept;

	////////////////////////////////////////////////////////////////////////////
	// 乱数
private:
	std::shared_ptr<dungeon::Random> GetSynchronizedRandom() const noexcept;
	const std::shared_ptr<dungeon::Random>& GetRandom() const noexcept;

	////////////////////////////////////////////////////////////////////////////
	// アクターのスポーンと破棄
public:
	/**
	 * アクターをスポーンします。
	 * DungeonGeneratorというタグを追加します。
	 * スポーンしたアクターはDestroySpawnedActorsで破棄されます。
	 */
	static AActor* SpawnActorImpl(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters);

	/**
	 * アクターの負荷制御コンポーネントを追加します
	 */
	static UDungeonComponentActivatorComponent* FindOrAddComponentActivatorComponent(AActor* actor);

private:
	AActor* SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const;
	template<typename T = AActor> T* SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	void DestroySpawnedActors() const;

protected:
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

	/**
	Notification when a dungeon is successfully created
	ダンジョンの生成に成功した時の通知
	*/
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationSuccessSignature OnGenerationSuccess;

	/**
	Notification when dungeon creation fails
	ダンジョンの生成に失敗した時の通知
	*/
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationFailureSignature OnGenerationFailure;

	////////////////////////////////////////////////////////////////////////////
	/**
	 * This event is called at the start of the Create function
	 * Create関数開始時に呼び出されるイベントです
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator")
	void BeginGeneration();

	/**
	 * This event is called at the start of the Create function
	 * Create関数開始時に呼び出されるイベントです
	 */
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGenerateBaseOnBeginGenerateSignature OnBeginGeneration;

	/**
	 * Event to query the creation of an aisle.
	 * If true is returned, the roof and aisle meshes are not generated.
	 * 通路の生成を問い合わせイベント
	 * trueを返すと屋根と通路のメッシュを生成しません
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator")
	bool OnQueryAisleGeneration(const FVector& center, const int32 identifier, const EDungeonDirection direction);

	/**
	 * Event called at the end of the Create function
	 * synchronizedRandom is a random number that is synchronized between clients. It must always be called the same number of times on server and client.
	 * aisleGridMap is a container for the generated aisle grid
	 *
	 * Create関数終了時に呼び出されるイベントです
	 * synchronizedRandomは、クライアント間で同期する乱数です。かならずサーバーとクライアントで同じ回数を呼び出す必要があります
	 * aisleGridMapは、生成された通路グリッドのコンテナ
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator")
	void EndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap);

	/**
	 * Event called at the end of the Create function
	 * synchronizedRandom is a random number that is synchronized between clients. It must always be called the same number of times on server and client.
	 * aisleGridMap is a container for the generated aisle grid
	 *
	 * Create関数終了時に呼び出されるイベントです
	 * synchronizedRandomは、クライアント間で同期する乱数です。かならずサーバーとクライアントで同じ回数を呼び出す必要があります
	 * aisleGridMapは、生成された通路グリッドのコンテナ
	 */
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGenerateBaseOnEndGenerateSignature OnEndGeneration;

	////////////////////////////////////////////////////////////////////////////
	// dungeon::Generator::Generate前イベント
	virtual void OnPreDungeonGeneration();
	// dungeon::Generator::Generate後イベント
	virtual void OnPostDungeonGeneration(const bool result);

	////////////////////////////////////////////////////////////////////////////
	// Terrain
	// event
	using AddStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;
	using AddPillarStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;

	void OnAddFloor(const AddStaticMeshEvent& function);
	void OnAddSlope(const AddStaticMeshEvent& function);
	void OnAddWall(const AddStaticMeshEvent& function);
	void OnAddRoof(const AddStaticMeshEvent& function);
	void OnAddPillar(const AddPillarStaticMeshEvent& function);
	void OnAddCatwalk(const AddStaticMeshEvent& function);

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
	struct ReservedWallInfo final
	{
		UStaticMesh* mStaticMesh;
		FTransform mTransform;

		ReservedWallInfo(UStaticMesh* staticMesh, const FTransform& transform)
			: mStaticMesh(staticMesh)
			, mTransform(transform)
		{}
	};
	
	void CreateImplement_QueryAisleGeneration(const bool hasAuthority);
	void CreateImplement_AddTerrain(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority);
	void CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp) const;
	void CreateImplement_ReserveWall(const CreateImplementParameter& cp);
	void CreateImplement_AddWall();
	void CreateImplement_AddRoof(const CreateImplementParameter& cp) const;
	void CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;
	bool CanAddDoor(const ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const;
	void CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;

	// Room sensor
	void CreateImplement_PrepareSpawnRoomSensor(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const;
	static void CreateImplement_FinishSpawnRoomSensor(const RoomAndRoomSensorMap& roomSensorCache);

	// Navigation
	void CreateImplement_Navigation(const bool hasAuthority);
	void CheckRecastNavMesh() const;

protected:
	virtual void FitNavMeshBoundsVolume();

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
	AActor* SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod, const bool castShadow) const;
	ADungeonRoomSensorBase* SpawnRoomSensorActorDeferred(
		UClass* actorClass,
		const dungeon::Identifier& identifier,
		const FVector& center,
		const FVector& extents,
		EDungeonRoomParts parts,
		EDungeonRoomItem item,
		uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart) const;
	static void FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor);

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
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	////////////////////////////////////////////////////////////////////////////
	// member variables
	// Vegetation
protected:

	UPROPERTY(Transient)
	const UDungeonGenerateParameter* mParameter;

	UPROPERTY(Transient)
	UDungeonAisleGridMap* mAisleGridMap;

private:
	std::shared_ptr<dungeon::Generator> mGenerator;
	std::shared_ptr<dungeon::Random> mLocalRandom;

	AddStaticMeshEvent mOnAddFloor;
	AddStaticMeshEvent mOnAddSlope;
	AddStaticMeshEvent mOnAddWall;
	AddStaticMeshEvent mOnAddRoof;
	AddPillarStaticMeshEvent mOnAddPillar;
	AddStaticMeshEvent mOnAddCatwalk;

	std::vector<ReservedWallInfo> mReservedWallInfo;
	

	// 生成時のCRC32
	mutable uint32_t mCrc32AtCreation = ~0;

	// 生成済みフラグ
	bool mGenerated = false;

	// friend class
};

inline const FName& ADungeonGenerateBase::GetDungeonGeneratorTag()
{
	static const FName DungeonGeneratorTag(TEXT("DungeonGenerator"));
	return DungeonGeneratorTag;
}

inline const FName& ADungeonGenerateBase::GetDungeonGeneratorTerrainTag()
{
	static const FName DungeonGeneratorTerrainTag(TEXT("DungeonGeneratorTerrain"));
	return DungeonGeneratorTerrainTag;
}

inline void ADungeonGenerateBase::OnAddFloor(const AddStaticMeshEvent& function)
{
	mOnAddFloor = function;
}

inline void ADungeonGenerateBase::OnAddSlope(const AddStaticMeshEvent& function)
{
	mOnAddSlope = function;
}

inline void ADungeonGenerateBase::OnAddWall(const AddStaticMeshEvent& function)
{
	mOnAddWall = function;
}

inline void ADungeonGenerateBase::OnAddRoof(const AddStaticMeshEvent& function)
{
	mOnAddRoof = function;
}

inline void ADungeonGenerateBase::OnAddPillar(const AddPillarStaticMeshEvent& function)
{
	mOnAddPillar = function;
}

inline void ADungeonGenerateBase::OnAddCatwalk(const AddStaticMeshEvent& function)
{
	mOnAddCatwalk = function;
}

template<typename T>
inline T* ADungeonGenerateBase::SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorImpl<T>(T::StaticClass(), folderPath, transform, ownerActor, spawnActorCollisionHandlingMethod);
}

template<typename T>
inline T* ADungeonGenerateBase::SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorDeferredImpl<T>(T::StaticClass(), folderPath, transform, ownerActor, spawnActorCollisionHandlingMethod);
}

template<typename T>
inline T* ADungeonGenerateBase::SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* ADungeonGenerateBase::SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	actorSpawnParameters.bDeferConstruction = true;
	return Cast<T>(SpawnActorImpl(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
inline T* ADungeonGenerateBase::FindActor()
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
inline const T* ADungeonGenerateBase::FindActor() const
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
inline void ADungeonGenerateBase::EachActors(const std::function<bool(T*)>& function)
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
inline void ADungeonGenerateBase::EachActors(const std::function<bool(const T*)>& function) const
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
