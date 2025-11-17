/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "SubActor/DungeonVerifiableActor.h"
#include "Helper/DungeonRandom.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
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
奈落落下 動的マルチキャストデリゲートイベントの宣言
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonRoomSensorBaseEventSignature);

/**
Room Area Sensor Actor
DungeonRoomSensorBase is an actor that is not intended to be replicated.
Please be very careful with server-client synchronization.

部屋の領域センサーアクター
DungeonRoomSensorBaseはレプリケーションされない前提のアクターです。
サーバーとクライアントの同期に十分注意して下さい。
*/
UCLASS(Abstract, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonRoomSensorBase : public ADungeonVerifiableActor
{
	GENERATED_BODY()

public:
	explicit ADungeonRoomSensorBase(const FObjectInitializer& initializer);
	virtual ~ADungeonRoomSensorBase() override = default;

	/**
	Get the room identifier
	部屋の識別子を取得します
	*/
	int32 GetIdentifier() const noexcept;

	/**
	Preparation for initialization immediately after spawning
	If OnPrepare succeeds, OnInitialize is called and the failing DungeonRoomSensorBase is deleted.
	スポーン直後の初期化準備
	OnPrepareが成功するとOnInitializeが呼び出され、失敗するDungeonRoomSensorBaseは破棄されます。
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	bool OnPrepare(const float depthFromStartRatio);
	virtual bool OnPrepare_Implementation(const float depthFromStartRatio);

	/**
	Function called during initialization after object creation
	オブジェクト生成後に呼び出される初期化用関数
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnInitialize(const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 depthFromStart, const float depthFromStartRatio);

	/**
	Finalize function called before object destruction
	オブジェクト破棄前に呼び出される終了用関数
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnFinalize(const bool finish);

	/**
	Function called on reset
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnReset(const bool fallToAbyss);

	/**
	Function called on resume
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnResume();

	/**
	Get the bounding of the room entry sensor
	部屋の進入センサーの範囲を取得します
	*/
	UBoxComponent* GetBounding();
	
	/**
	Get the bounding of the room entry sensor
	部屋の進入センサーの範囲を取得します
	*/
	const UBoxComponent* GetBounding() const;

	/**
	Get room size
	部屋の範囲を取得します
	*/
	const FBox& GetRoomSize() const noexcept;

	/**
	Get the ideal number of people in the bounding
	部屋に含める理想的な人数を求めます
	@param[in]		areaRequiredPerPerson (default : 500cm)
	@param[in]		maxNumberOfActor (default : 5 actors)
	@return			ideal number of people in the bounding
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 IdealNumberOfActor(const float areaRequiredPerPerson = 500.f, const int32 maxNumberOfActor = 5) const;

	/**
	Get a random position in the bounding
	If useLocalRandom is set to false, be sure to call it the same number of times for all clients.

	部屋の範囲内のランダムな位置を取得します
	useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool RandomPoint(FVector& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	Get random transforms in bounding
	If useLocalRandom is set to false, be sure to call it the same number of times for all clients.

	部屋の範囲内のランダムなトランスフォームを取得します
	useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool RandomTransform(FTransform& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	Get floor height position
	部屋の高さ取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	bool GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight = 0.f) const;

	/**
	Utility function to spawn an actor.
	Makes the spawned actor's own owner-actor and adds a dungeon generator to the tag.
	アクターをスポーンするユーティリティ関数です。
	スポーンされたアクターの自身をオーナーアクターにして、タグにダンジョンジェネレータを追加します。
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	AActor* SpawnActorFromClass(TSubclassOf<class AActor> actorClass, const FTransform transform, const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined, APawn* instigator_ = nullptr, const bool transient = false);

	/**
	Calculate the depth ratio from the start
	スタート部屋からゴール部屋の部屋数からこの部屋の深さの割合を計算します
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	float GetDepthRatioFromStart() const;

	/**
	Gets the probability (0%-100%) of adding a door. Settings after OnInitialize are invalid.
	ドアを追加する確率(0%～100%)を取得します。	OnInitialize以降の設定は無効です。
	*/
	uint8 GetDoorAddingProbability() const noexcept;

	/**
	Sets the probability (0% to 100%) that a door will be added. Settings after OnInitialize are invalid.
	ドアを追加する確率(0%～100%)を設定します。	OnInitialize以降の設定は無効です。
	*/
	void SetDoorAddingProbability(const uint8 doorAddingProbability) noexcept;

	/**
	Add room door
	部屋にドアを追加します
	*/
	void AddDungeonDoor(ADungeonDoorBase* dungeonDoorBase);

	/**
	Update room doors
	部屋のドアを更新します
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
	Does the room have a locked door?
	部屋に鍵付きドアがあるか？
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	bool HasLockedDoor() const;

	/**
	Add torchlight in a room
	部屋に燭台を追加します
	*/
	void AddDungeonTorch(AActor* actor);

	/**
	Update the torchlight in the room.
	部屋の燭台を更新します
	*/
	template<typename Function>
	void EachDungeonTorch(Function&& function) const noexcept
	{
		for (AActor* actor : DungeonTorches)
		{
			std::forward<Function>(function)(actor);
		}
	}

	/**
	Gets the tag of the interior of the room to be generated.
	For example, if the creator returns the tag `kitchen`, the interior with the kitchen tag will be selected.
	If multiple tags are set, actors containing all tags will be selected as spawn candidates.

	生成する部屋のインテリアのタグを取得します。
	例えば、クリエイターが `kitchen` というタグを返した場合、キッチンのタグを持つインテリアが選択されます。
	複数のタグが設定されている場合、すべてのタグを含むアクターがスポーン候補として選択されます。
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	TArray<FString> GetInquireInteriorTags() const;

	/**
	Get the DungeonGenerator tag name.
	DungeonGeneratorのタグを取得します
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	static const FName& GetDungeonGeneratorTag();

	/**
	Get the DungeonGenerator tag name.
	DungeonGeneratorのタグを取得します
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	static const TArray<FName>& GetDungeonGeneratorTags();

	/**
	Get a random object for synchronization
	Make sure that the remote and client get the same number of random numbers.
	If the counts are different, synchronization of dungeon creation will fail.
	
	同期用のランダムオブジェクトを取得します
	必ずリモートとクライアントが同じ回数乱数を取得するようにしてください。
	回数が違った場合はダンジョンの生成の同期に失敗します。
	*/
	CDungeonRandom& GetSynchronizedRandom();

	/**
	Get a random object that can be used locally
	Use when using random numbers only remotely.

	ローカルで使用できるランダムオブジェクトを取得します
	リモートのみで乱数を使用する時に使用して下さい。
	*/
	CDungeonRandom& GetLocalRandom();

	// overrides
	virtual uint32_t GenerateCrc32(uint32_t crc = 0xffffffffU) const noexcept override;

	// overrides
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
#endif

protected:
	/**
	遅延スポーン直後の初期化準備関数
	Prepareが成功するとInitializeが呼び出され、失敗するDungeonRoomSensorBaseは破棄されます。
	*/
	virtual bool OnNativePrepare(const FVector& center);

	/**
	Functions for initialization after object creation
	オブジェクト生成後の初期化用関数
	*/
	virtual void OnNativeInitialize();

	/**
	Function called before object destruction
	オブジェクト破棄前の終了用関数
	*/
	virtual void OnNativeFinalize(const bool finish);

	/**
	Reset room sensor
	ルームセンサーのリセット
	*/
	virtual void OnNativeReset(const bool fallToAbyss);

	/**
	Reset room sensor
	ルームセンサーの再開
	*/
	virtual void OnNativeResume();

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	bool InvokePrepare(
		const std::shared_ptr<dungeon::Random>& random,
		const int32 identifier,
		const FVector& center,
		const FVector& extents,
		const float horizontalGridSize,
		const EDungeonRoomParts parts,
		const EDungeonRoomItem item,
		const uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart);
	void InvokeInitialize();
	void InvokeFinalize();
	void InvokeReset(const bool fallToAbyss);
	void InvokeResume();
	bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;

	void SpawnActorsInRoomImpl();
	void SpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, const bool force);

protected:
	/**
	Room Bounding Box
	部屋のバウンディングボックスセンサーコンポーネント
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	TObjectPtr<UBoxComponent> Bounding;

	/**
	Room Size
	部屋の大きさ
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Bounding")
	FBox RoomSize;

	/**
	Horizontal margin of room bounding box sensor
	部屋のバウンディングボックスセンサーの水平マージン
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Bounding", meta = (ClampMin = "0"))
	float HorizontalMargin = 0.f;

	/**
	Vertical margin of room bounding box sensor
	部屋のバウンディングボックスセンサーの垂直マージン
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Bounding", meta = (ClampMin = "0"))
	float VerticalMargin = 0.f;

	/**
	Room Identifier
	部屋の識別子
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	int32 Identifier = -1;

	/**
	Type of room parts (start, goal, hall, etc.)
	部屋パーツの種類（スタート、ゴール、ホール等）
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	/**
	Types of items that should be placed in the room
	部屋に配置すべきアイテムの種類
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	/**
	ミッショングラフが生成したブランチ識別子
	Branch identifier generated by the mission graph
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	uint8 BranchId = 0;

	/**
	 * Room depth from the starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 *
	 * 	スタート部屋から部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	uint8 DepthFromStart = 0;

	/**
	 * Depth of deepest room from starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 *
	 * スタート部屋から最も深い部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	uint8 DeepestDepthFromStart = 0;

	/**
	Automatically calls OnReset when the player leaves
	プレイヤーが離れたら自動的にOnResetを呼ぶ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Bounding")
	bool AutoReset = true;

#if WITH_EDITORONLY_DATA
	/**
	Display debugging information
	デバッグ情報を表示
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug")
	bool ShowDebugInformation = false;
#endif

	/**
	Notification when exiting from the bottom of the sensor (falling into the abyss)
	センサーの底面から出た時の通知（奈落落下）
	*/
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FDungeonRoomSensorBaseEventSignature OnFallToAbyss;

	/**
	the probability (0%-100%) of adding a door
	ドアを追加する確率(0%～100%)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "100"))
	uint8 DoorAddingProbability = 100;

	/**
	Room door
	部屋のドア
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	TArray<TObjectPtr<ADungeonDoorBase>> DungeonDoors;

	/**
	Torchlight in a room
	部屋の燭台
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	TArray<TObjectPtr<AActor>> DungeonTorches;


	/**
	 * Number of people per area to find the ideal number of people to spawn in a room.
	 * If the SpawnActors array is empty, it will not spawn.
	 * 
	 * 部屋にスポーンする理想的な人数を求めるための面積毎の人数
	 * SpawnActors配列が空の場合はスポーンしません
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom")
	float AreaRequiredPerPerson = 500.f;

	/**
	 * Maximum number of people to find the ideal number of people to spawn in a room
	 * If the SpawnActors array is empty, it will not spawn.
	 * 
	 * 部屋にスポーンする理想的な人数を求めるための最大人数
	 * SpawnActors配列が空の場合はスポーンしません
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom")
	int32 MaxNumberOfActor = 5;

	/**
	 * Actor spawning in a room
	 * For finer control, spawn actors individually using OnNativeInitialize or OnInitialize
	 *
	 * 部屋にスポーンするアクター
	 * 細やかな制御をおこなう場合はOnNativeInitializeやOnInitializeでアクターを個別にスポーンして下さい
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (AllowedClasses = "/Script/Engine.Blueprint"))
	TArray<FSoftObjectPath> SpawnActors;

	/**
	 * Key actor in the mission graph.
	 * Spawns at a random location in the room if specified.
	 * If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.
	 *
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
	 * 
	 * ミッショングラフのユニーク鍵アクター
	 * 指定した場合は部屋のランダムな位置にスポーンします。
	 * 宝箱や敵を倒した時に鍵を入手させたい場合は独自に実装して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|MissionGraph", meta = (AllowedClasses = "/Script/Engine.Blueprint"))
	FSoftObjectPath SpawnUniqueKeyActor;

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
