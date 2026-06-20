/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "SubActor/DungeonVerifiableActor.h"
#include "Helper/DungeonRandom.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Parameter/DungeonLayoutTypes.h"
#include "DungeonGameplayRoleEnemySpawnMultipliers.h"
#include "DungeonStructuralRoleEnemySpawnMultipliers.h"
#include "DungeonGeneratedRoomInfo.h"
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <memory>
#include "DungeonRoomSensorBase.generated.h"

// forward declaration
class ADungeonDoorBase;
class UBoxComponent;
class UPrimitiveComponent;

namespace dungeon
{
	class Random;
}

/**
 * Dynamic multicast delegate event for falling into the abyss.
 * 奈落落下を通知する動的マルチキャストデリゲートイベントです。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonRoomSensorBaseEventSignature);

/**
 * Room Area Sensor Actor
 * DungeonRoomSensorBase is an actor that is not intended to be replicated.
 * Please be very careful with server-client synchronization.
 * 部屋の領域センサーアクター
 * DungeonRoomSensorBaseはレプリケーションされない前提のアクターです。
 * サーバーとクライアントの同期に十分注意して下さい。
 *
 */
UCLASS(Abstract, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonRoomSensorBase : public ADungeonVerifiableActor
{
	GENERATED_BODY()

public:
	explicit ADungeonRoomSensorBase(const FObjectInitializer& initializer);
	virtual ~ADungeonRoomSensorBase() override = default;

	/**
	 * Get the room identifier
	 * 部屋の識別子を取得します
	 */
	int32 GetIdentifier() const noexcept;

	/**
	 * Preparation for initialization immediately after spawning
	 * If OnPrepare succeeds, OnInitialize is called and the failing DungeonRoomSensorBase is deleted.
	 * スポーン直後の初期化準備
	 * OnPrepareが成功するとOnInitializeが呼び出され、失敗するDungeonRoomSensorBaseは破棄されます。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	bool OnPrepare(const float depthFromStartRatio);
	virtual bool OnPrepare_Implementation(const float depthFromStartRatio);

	/**
	 * Function called during initialization after object creation.
	 * オブジェクト生成後に呼び出される初期化用関数です。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnInitialize(const FDungeonGeneratedRoomInfo& GeneratedRoomInfo);

	/**
	 * Gets generated room information prepared for this sensor.
	 * このセンサー用に準備された生成部屋情報を取得します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	FDungeonGeneratedRoomInfo GetGeneratedRoomInfo() const noexcept;

	/**
	 * Finalize function called before object destruction
	 * オブジェクト破棄前に呼び出される終了用関数
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnFinalize(const bool finish);

	/**
	 * Function called on reset
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnReset(const bool fallToAbyss);

	/**
	 * Function called on resume
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnResume();

	/**
	 * Get the bounding of the room entry sensor
	 * 部屋の進入センサーの範囲を取得します
	 */
	UBoxComponent* GetBounding();
	
	/**
	 * Get the bounding of the room entry sensor
	 * 部屋の進入センサーの範囲を取得します
	 */
	const UBoxComponent* GetBounding() const;

	/**
	 * Get room size
	 * 部屋の範囲を取得します
	 */
	const FBox& GetRoomSize() const noexcept;

	/**
	 * Gets the ideal actor count after applying area, gameplay role, and structural role enemy spawn multipliers.
	 * 部屋に含める理想的な人数を求めます
	 * @param[in]		areaRequiredPerPerson (default : 500cm)
	 * @param[in]		maxNumberOfActor (default : 5 actors)
	 * @return			ideal actor count after the same role scaling used by the Helper spawn settings
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 IdealNumberOfActor(const float areaRequiredPerPerson = 500.f, const int32 maxNumberOfActor = 5) const;

	/**
	 * Get a random position in the bounding
	 * If useLocalRandom is set to false, be sure to call it the same number of times for all clients.
	 * 部屋の範囲内のランダムな位置を取得します
	 * useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	 * 部屋の範囲内のランダムなトランスフォームを取得します
	 *
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool RandomPoint(FVector& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	 * Get random transforms in bounding
	 * If useLocalRandom is set to false, be sure to call it the same number of times for all clients.
	 * 部屋の範囲内のランダムな位置を取得します
	 * useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	 * 部屋の範囲内のランダムなトランスフォームを取得します
	 *
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool RandomTransform(FTransform& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	 * Get floor height position
	 * 部屋の高さ取得します
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight = 0.f) const;

	/**
	 * Utility function to spawn an actor.
	 * Makes the spawned actor's own owner-actor and adds a dungeon generator to the tag.
	 * アクターをスポーンするユーティリティ関数です。
	 * スポーンされたアクターの自身をオーナーアクターにして、タグにダンジョンジェネレータを追加します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	AActor* SpawnActorFromClass(TSubclassOf<class AActor> actorClass, const FTransform transform, const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined, APawn* instigator_ = nullptr);

	/**
	 * Calculate the depth ratio from the start
	 * スタート部屋からゴール部屋の部屋数からこの部屋の深さの割合を計算します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	float GetDepthRatioFromStart() const;

	/**
	 * Gets the probability (0%-100%) of adding a door. Settings after OnInitialize are invalid.
	 * ドアを追加する確率(0%～100%)を取得します。	OnInitialize以降の設定は無効です。
	 */
	uint8 GetDoorAddingProbability() const noexcept;

	/**
	 * Sets the probability (0% to 100%) that a door will be added. Settings after OnInitialize are invalid.
	 * ドアを追加する確率(0%～100%)を設定します。	OnInitialize以降の設定は無効です。
	 */
	void SetDoorAddingProbability(const uint8 doorAddingProbability) noexcept;

	/**
	 * Add room door
	 * 部屋にドアを追加します
	 */
	void AddDungeonDoor(ADungeonDoorBase* dungeonDoorBase);

	/**
	 * Update room doors
	 * 部屋のドアを更新します
	 */
	template<typename Function>
	void EachDungeonDoors(Function&& function) const noexcept
	{
		for (ADungeonDoorBase* dungeonDoorBase : DungeonDoors)
		{
			std::forward<Function>(function)(dungeonDoorBase);
		}
	}

	/**
	 * Does the room have a locked door?
	 * 部屋に鍵付きドアがあるかを返します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	bool HasLockedDoor() const;

	/**
	 * Add torchlight in a room
	 * 部屋に燭台を追加します
	 */
	void AddDungeonTorch(AActor* actor);

	/**
	 * Add chandelier in a room
	 * 部屋にシャンデリアを追加します
	 */
	void AddDungeonChandelier(AActor* actor);

	/**
	 * Update the torchlight in the room.
	 * 部屋の燭台を更新します
	 */
	template<typename Function>
	void EachDungeonTorch(Function&& function) const noexcept
	{
		for (AActor* actor : DungeonTorches)
		{
			std::forward<Function>(function)(actor);
		}
	}

	template<typename Function>
	void EachDungeonChandelier(Function&& function) const noexcept
	{
		for (AActor* actor : DungeonChandeliers)
		{
			std::forward<Function>(function)(actor);
		}
	}

	/**
	 * Gets the tag of the interior of the room to be generated.
	 * For example, if the creator returns the tag `kitchen`, the interior with the kitchen tag will be selected.
	 * If multiple tags are set, actors containing all tags will be selected as spawn candidates.
	 * 生成する部屋のインテリアのタグを取得します。
	 * 例えば、クリエイターが `kitchen` というタグを返した場合、キッチンのタグを持つインテリアが選択されます。
	 * 複数のタグが設定されている場合、すべてのタグを含むアクターがスポーン候補として選択されます。
	 *
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	TArray<FString> GetInquireInteriorTags() const;

	/**
	 * Get the DungeonGenerator tag name.
	 * DungeonGeneratorのタグを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	static const FName& GetDungeonGeneratorTag();

	/**
	 * Get the DungeonGenerator tag name.
	 * DungeonGeneratorのタグを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	static const TArray<FName>& GetDungeonGeneratorTags();

	/**
	 * Get a random object for synchronization
	 * Make sure that the remote and client get the same number of random numbers.
	 * If the counts are different, synchronization of dungeon creation will fail.
	 * 同期用のランダムオブジェクトを取得します
	 * 必ずリモートとクライアントが同じ回数乱数を取得するようにしてください。
	 * 回数が違った場合はダンジョンの生成の同期に失敗します。
	 *
	 */
	CDungeonRandom& GetSynchronizedRandom();

	/**
	 * Get a random object that can be used locally
	 * Use when using random numbers only remotely.
	 * ローカルで使用できるランダムオブジェクトを取得します
	 * リモートのみで乱数を使用する時に使用して下さい。
	 *
	 */
	CDungeonRandom& GetLocalRandom();

	// overrides
	virtual uint32_t GenerateCrc32(uint32_t crc = 0xffffffffU) const noexcept override;

	// overrides
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
#endif

protected:
	/**
	 * Native preparation called immediately after deferred spawning.
	 * 遅延スポーン直後に呼び出されるネイティブ初期化準備です。
	 */
	virtual bool OnNativePrepare(const FVector& center);

	/**
	 * Functions for initialization after object creation
	 * オブジェクト生成後の初期化用関数
	 */
	virtual void OnNativeInitialize();

	/**
	 * Function called before object destruction
	 * オブジェクト破棄前の終了用関数
	 */
	virtual void OnNativeFinalize(const bool finish);

	/**
	 * Reset room sensor
	 * ルームセンサーのリセット
	 */
	virtual void OnNativeReset(const bool fallToAbyss);

	/**
	 * Reset room sensor
	 * ルームセンサーの再開
	 */
	virtual void OnNativeResume();

private:
	/**
	 * Handles the player entering this room sensor overlap.
	 * プレイヤーがこのルームセンサーの重なり範囲へ入ったときの処理です。
	 */
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/**
	 * Handles the player leaving this room sensor overlap.
	 * プレイヤーがこのルームセンサーの重なり範囲から出たときの処理です。
	 */
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	bool InvokePrepare(
		const std::shared_ptr<dungeon::Random>& random,
		const int32 identifier,
		const FVector& center,
		const FVector& extents,
		const float horizontalGridSize,
		const EDungeonRoomParts parts,
		const EDungeonRoomItem item,
		const FDungeonGeneratedRoomInfo& roomInfo,
		const uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart
	);
	void InvokeInitialize();
	void InvokeFinalize();
	void InvokeReset(const bool fallToAbyss);
	void InvokeResume();
	bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;

	static float GetDefaultGameplayRoleEnemySpawnMultiplier(EDungeonRoomGameplayRole role) noexcept;
	static float GetDefaultStructuralRoleEnemySpawnMultiplier(EDungeonRoomStructuralRole role) noexcept;
	static float FindGameplayRoleEnemySpawnMultiplier(const FDungeonGameplayRoleEnemySpawnMultipliers& multipliers, EDungeonRoomGameplayRole role) noexcept;
	static float FindStructuralRoleEnemySpawnMultiplier(const FDungeonStructuralRoleEnemySpawnMultipliers& multipliers, EDungeonRoomStructuralRole role) noexcept;
	static int32 CalculateSpawnActorsInRoomCount(
		int32 baseCount,
		const FDungeonGeneratedRoomInfo& roomInfo,
		const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
		const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept;
	static int32 CalculateAreaBasedActorCount(const FBox& bounds, float areaRequiredPerPerson, int32 maxNumberOfActor) noexcept;
	float GetGameplayRoleEnemySpawnMultiplier(EDungeonRoomGameplayRole role) const noexcept;
	float GetStructuralRoleEnemySpawnMultiplier(EDungeonRoomStructuralRole role) const noexcept;
	int32 CalculateSpawnActorsInRoomCount() const;
	void SpawnActorsInRoomImpl();
	AActor* SpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod);
	
#if WITH_DEV_AUTOMATION_TESTS
public:
	/*
	 * Calculates the helper spawn count from a supplied base count for automation tests.
	 * 自動テスト用に、指定された基礎数から Helper スポーン数を計算します。
	 */
	static int32 CalculateSpawnActorsInRoomCountForTest(
		int32 baseCount,
		const FDungeonGeneratedRoomInfo& roomInfo,
		const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
		const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept;

	/*
	 * Calculates the same role-scaled count as IdealNumberOfActor from supplied bounds for automation tests.
	 * 自動テスト用に、指定された範囲から IdealNumberOfActor と同じ役割倍率反映後の数を計算します。
	 */
	static int32 CalculateIdealNumberOfActorForTest(
		const FBox& bounds,
		float areaRequiredPerPerson,
		int32 maxNumberOfActor,
		const FDungeonGeneratedRoomInfo& roomInfo,
		const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
		const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept;
#endif

protected:
	/**
	 * Room Bounding Box
	 * 部屋のバウンディングボックスセンサーコンポーネント
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	TObjectPtr<UBoxComponent> Bounding;

	/**
	 * the probability (0%-100%) of adding a door
	 * ドアを追加する確率(0%～100%)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 DoorAddingProbability = 100;

	/**
	 * Automatically calls OnReset when the player leaves
	 * プレイヤーが離れたら自動的にOnResetを呼ぶ
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool AutoReset = true;

#if WITH_EDITORONLY_DATA
	/**
	 * Display debugging information
	 * デバッグ情報を表示
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
	bool ShowDebugInformation = false;
#endif

	/**
	 * Number of people per area to find the ideal number of people to spawn in a room.
	 * If the SpawnActors array is empty, it will not spawn.
	 * 部屋にスポーンする理想的な人数を求めるための面積毎の人数
	 * SpawnActors配列が空の場合はスポーンしません
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (ClampMin = "0", ToolTip = "Area required for one enemy actor before GameplayRole and StructuralRole enemy spawn multipliers are applied."))
	float AreaRequiredPerPerson = 500.f;

	/**
	 * Maximum number of people to find the ideal number of people to spawn in a room
	 * If the SpawnActors array is empty, it will not spawn.
	 * 部屋にスポーンする理想的な人数を求めるための最大人数
	 * SpawnActors配列が空の場合はスポーンしません
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (ClampMin = "0", ToolTip = "Maximum number of enemy actors before GameplayRole and StructuralRole enemy spawn multipliers are applied."))
	int32 MaxNumberOfActor = 10;

	/**
	 * Actor spawning in a room
	 * For finer control, spawn actors individually using OnNativeInitialize or OnInitialize
	 * 部屋にスポーンするアクター
	 * 細やかな制御をおこなう場合はOnNativeInitializeやOnInitializeでアクターを個別にスポーンして下さい
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (AllowedClasses = "/Script/Engine.Blueprint", ToolTip = "Enemy actor Blueprints spawned in this room. The final spawn count is adjusted by GameplayRole and StructuralRole."))
	TArray<FSoftObjectPath> SpawnActors;

	/*
	 * Enemy spawn count multipliers by gameplay role.
	 * ゲームプレイ役割ごとの敵スポーン数倍率です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (ToolTip = "Enemy spawn count multipliers by GameplayRole. Edit the field for each role directly; 0 disables enemy spawning for that role."))
	FDungeonGameplayRoleEnemySpawnMultipliers GameplayRoleEnemySpawnMultipliers;

	/*
	 * Enemy spawn count multipliers by structural role.
	 * 構造役割ごとの敵スポーン数倍率です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (ToolTip = "Enemy spawn count multipliers by StructuralRole. Edit the field for each role directly; 0 disables enemy spawning for that role."))
	FDungeonStructuralRoleEnemySpawnMultipliers StructuralRoleEnemySpawnMultipliers;

	/**
	 * Key actor in the mission graph.
	 * Spawns at a random location in the room if specified.
	 * If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.
	 * ミッショングラフの鍵アクター
	 * 指定した場合は部屋のランダムな位置にスポーンします。
	 * 宝箱や敵を倒した時に鍵を入手させたい場合は独自に実装して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|MissionGraph", meta = (AllowedClasses = "/Script/Engine.Blueprint"))
	FSoftObjectPath SpawnKeyActor;

	/**
	 * Unique key actor in the mission graph.
	 * Spawns at a random location in the room if specified.
	 * If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.
	 * ミッショングラフのユニーク鍵アクター
	 * 指定した場合は部屋のランダムな位置にスポーンします。
	 * 宝箱や敵を倒した時に鍵を入手させたい場合は独自に実装して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|MissionGraph", meta = (AllowedClasses = "/Script/Engine.Blueprint"))
	FSoftObjectPath SpawnUniqueKeyActor;

	/**
	 * Room Size
	 * 部屋の大きさ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding")
	FBox RoomSize;

	/**
	 * Horizontal margin of room bounding box sensor
	 * 部屋のバウンディングボックスセンサーの水平マージン
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding", meta = (ClampMin = "0"))
	float HorizontalMargin = 0.f;

	/**
	 * Vertical margin of room bounding box sensor
	 * 部屋のバウンディングボックスセンサーの垂直マージン
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding", meta = (ClampMin = "0"))
	float VerticalMargin = 0.f;

	/**
	 * Room Identifier
	 * 部屋の識別子
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Identifier or RoomInfo.Identifier instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	int32 Identifier = 0;

	/**
	 * Type of room parts (start, goal, hall, etc.)
	 * 部屋パーツの種類（スタート、ゴール、ホール等）
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Parts or RoomInfo.Parts instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	/**
	 * Types of items that should be placed in the room
	 * 部屋に配置すべきアイテムの種類
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Item or RoomInfo.Item instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	/**
	 * Branch identifier generated by the mission graph
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().BranchId or RoomInfo.BranchId instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 BranchId = 0;

	/**
	 * Room depth from the starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 * スタート部屋から部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().DepthFromStart or RoomInfo.DepthFromStart instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 DepthFromStart = 0;

	/**
	 * Depth of deepest room from starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 * スタート部屋から最も深い部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().DeepestDepthFromStart or RoomInfo.DeepestDepthFromStart instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 DeepestDepthFromStart = 0;

	/**
	 * Generated room information exposed to gameplay Blueprints.
	 * ゲームプレイBlueprintへ公開する生成部屋情報です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information")
	FDungeonGeneratedRoomInfo RoomInfo;

	/**
	 * Notification when exiting from the bottom of the sensor (falling into the abyss)
	 * センサーの底面から出た時の通知（奈落落下）
	 */
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FDungeonRoomSensorBaseEventSignature OnFallToAbyss;

	/**
	 * Room door
	 * 部屋のドア
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors")
	TArray<TObjectPtr<ADungeonDoorBase>> DungeonDoors;

	/**
	 * Torchlight in a room
	 * 部屋の燭台
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors")
	TArray<TObjectPtr<AActor>> DungeonTorches;

	/**
	 * Chandelier in a room
	 * 部屋のシャンデリア
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors")
	TArray<TObjectPtr<AActor>> DungeonChandeliers;

	/**
	 * Room objects (excluding doors, candlesticks, and chandeliers)
	 * 部屋のアクター（ドア、燭台、シャンデリアを除く）
	 */
	UPROPERTY(BlueprintReadWrite, Category = "DungeonGenerator|Information|Actors")
	TArray<TObjectPtr<AActor>> DungeonRoomSpawnedActors;	

private:
	CDungeonRandom mSynchronizedRandom;
	CDungeonRandom mLocalRandom;
	float mHorizontalGridSize = 0.f;
	uint16_t mOverlapCount = 0;

	enum class State : uint8_t
	{
		Invalid,
		Initialized,
		Finalized,
	};
	State mState = State::Invalid;
	bool mEntered = true;

	friend class ADungeonGenerateBase;
};
