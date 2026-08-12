/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
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
#include <Engine/Scene.h>
#include <GameFramework/Actor.h>
#include <memory>
#include "DungeonRoomSensorBase.generated.h"

// forward declaration
class ADungeonDoorBase;
class ADungeonGenerateBase;
class ADungeonRoomLightingActor;
class ADungeonGenerateActor;
class UBoxComponent;
class UDungeonComponentActivatorComponent;
class UPrimitiveComponent;
class UPointLightComponent;
class USpotLightComponent;

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
 * Callback invoked after a Room Sensor deferred Actor spawn request is processed.
 * Room Sensorの遅延Actor生成要求が処理された後に呼び出されるコールバックです。
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FDungeonRoomSensorDeferredActorSpawnedSignature, AActor*, SpawnedActor);

/*
 * Type of landmark emphasized by an automatically generated guidance light.
 * 自動生成された誘導光で強調する目印の種類です。
 */
UENUM(BlueprintType)
enum class EDungeonGuidanceTargetType : uint8
{
	Door UMETA(DisplayName = "Door", ToolTip = "A generated room door."),
	Stair UMETA(DisplayName = "Stair", ToolTip = "The entrance of a generated stair or slope inside a room.")
};

/**
 * World-space landmark information used to generate a guidance light.
 * 誘導光の生成に使用するワールド空間の目印情報です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGuidanceTarget
{
	GENERATED_BODY()

	/**
	 * Type of landmark represented by this target.
	 * このターゲットが表す目印の種類です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ToolTip = "Type of landmark represented by this guidance target."))
	EDungeonGuidanceTargetType Type = EDungeonGuidanceTargetType::Door;

	/**
	 * World-space location at the floor of the landmark entrance.
	 * 目印の入口床面にあるワールド空間位置です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ToolTip = "World-space location at the floor of the door or stair entrance."))
	FVector Location = FVector::ZeroVector;

	/**
	 * Horizontal world-space direction through the landmark.
	 * 目印を通り抜ける水平方向のワールド空間方向です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ToolTip = "Normalized horizontal world-space direction through the door or into the stair."))
	FVector Direction = FVector::ForwardVector;

	/**
	 * World-space floor height at the landmark entrance.
	 * 目印の入口におけるワールド空間の床高さです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ToolTip = "World-space floor height at the guidance target entrance."))
	float FloorHeight = 0.f;
};

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
	UFUNCTION(BlueprintNativeEvent, Category = "DungeonGenerator", meta = (ToolTip = "Preparation for initialization immediately after spawning If OnPrepare succeeds, OnInitialize is called and the failing DungeonRoomSensorBase is deleted.", CallInEditor = "true"))
	bool OnPrepare(const float depthFromStartRatio);
	virtual bool OnPrepare_Implementation(const float depthFromStartRatio);

	/**
	 * Function called during initialization after object creation.
	 * オブジェクト生成後に呼び出される初期化用関数です。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Function called during initialization after object creation.", CallInEditor = "true"))
	void OnInitialize(const FDungeonGeneratedRoomInfo& GeneratedRoomInfo);

	/**
	 * Gets generated room information prepared for this sensor.
	 * このセンサー用に準備された生成部屋情報を取得します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Gets generated room information prepared for this sensor."))
	FDungeonGeneratedRoomInfo GetGeneratedRoomInfo() const noexcept;

	/**
	 * Changes this room's base fill-light intensity on the server and synchronizes it to every client.
	 * この部屋のBase Fill Lightの明るさをサーバーで変更し、すべてのクライアントへ同期します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DungeonGenerator|Lights", meta = (ClampMin = "0", ToolTip = "Changes this room's base fill-light intensity on the server. The clamped current value is synchronized to existing and late-joining clients."))
	void SetBaseFillLightIntensity(float intensity);

	/**
	 * Changes this room's guidance-light base intensity on the server and synchronizes it to every client.
	 * この部屋のGuidance Lightの基準明るさをサーバーで変更し、すべてのクライアントへ同期します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DungeonGenerator|Lights", meta = (ClampMin = "0", ToolTip = "Changes this room's guidance-light base intensity before door or stair multipliers are applied. The current value is synchronized to existing and late-joining clients."))
	void SetGuidanceLightIntensity(float intensity);

	/**
	 * Changes both generated room-light intensities on the server.
	 * 2種類の生成部屋ライトの明るさをサーバーでまとめて変更します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DungeonGenerator|Lights", meta = (ClampMin = "0", ToolTip = "Changes this room's base fill and guidance-light intensities together on the server and synchronizes both current values."))
	void SetRoomLightIntensities(float baseFillIntensity, float guidanceIntensity);

	/** Returns this room's current base fill-light intensity. この部屋の現在のBase Fill Lightの明るさを返します。 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|Lights", meta = (ToolTip = "Returns this room's current synchronized base fill-light intensity."))
	float GetBaseFillLightIntensity() const noexcept;

	/** Returns this room's current guidance-light base intensity. この部屋の現在のGuidance Lightの基準明るさを返します。 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|Lights", meta = (ToolTip = "Returns this room's current synchronized guidance-light base intensity before target-type multipliers."))
	float GetGuidanceLightIntensity() const noexcept;

	/**
	 * Finalize function called before object destruction
	 * オブジェクト破棄前に呼び出される終了用関数
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Finalize function called before object destruction", CallInEditor = "true"))
	void OnFinalize(const bool finish);

	/**
	 * Function called on reset
	 * OnReset を表します。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Function called on reset"))
	void OnReset(const bool fallToAbyss);

	/**
	 * Function called on resume
	 * OnResume を表します。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Function called on resume"))
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
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Gets the ideal actor count after applying area, gameplay role, and structural role enemy spawn multipliers."))
	int32 IdealNumberOfActor(const float areaRequiredPerPerson = 500.f, const int32 maxNumberOfActor = 5) const;

	/**
	 * Get a random position in the bounding
	 * If useLocalRandom is set to false, be sure to call it the same number of times for all clients.
	 * 部屋の範囲内のランダムな位置を取得します
	 * useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	 * 部屋の範囲内のランダムなトランスフォームを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DungeonGenerator", meta = (ToolTip = "Get a random position in the bounding If useLocalRandom is set to false, be sure to call it the same number of times for all clients."))
	bool RandomPoint(FVector& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	 * Get random transforms in bounding
	 * If useLocalRandom is set to false, be sure to call it the same number of times for all clients.
	 * 部屋の範囲内のランダムな位置を取得します
	 * useLocalRandomをfalseにした場合、必ず全てのクライアントで同じ回数呼び出すようにしてください。
	 * 部屋の範囲内のランダムなトランスフォームを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DungeonGenerator", meta = (ToolTip = "Get random transforms in bounding If useLocalRandom is set to false, be sure to call it the same number of times for all clients."))
	bool RandomTransform(FTransform& result, const float offsetHeight = 0.f, const bool useLocalRandom = true) const;

	/**
	 * Get floor height position
	 * 部屋の高さ取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DungeonGenerator", meta = (ToolTip = "Get floor height position"))
	bool GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight = 0.f) const;

	/**
	 * Utility function to spawn an actor.
	 * Makes the spawned actor's own owner-actor and adds a dungeon generator to the tag.
	 * アクターをスポーンするユーティリティ関数です。
	 * スポーンされたアクターの自身をオーナーアクターにして、タグにダンジョンジェネレータを追加します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Utility function to spawn an actor. Makes the spawned actor's own owner-actor and adds a dungeon generator to the tag."))
	AActor* SpawnActorFromClass(TSubclassOf<class AActor> actorClass, const FTransform transform, const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined, APawn* instigator_ = nullptr);

	/**
	 * Queues an Actor spawn through the owning dungeon generator and invokes OnSpawned after processing.
	 * 所有するDungeon GeneratorへActor生成を予約し、処理後にOnSpawnedを呼び出します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Queues an actor spawn through the owning dungeon generator. The spawned actor is registered as this Room Sensor's child before On Spawned is invoked. Requests made during generation delay OnGenerationSuccess until they are processed."))
	void RequestDeferredSpawnActorFromClass(
		TSubclassOf<class AActor> actorClass,
		const FTransform transform,
		ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride,
		APawn* instigatorActor,
		const FDungeonRoomSensorDeferredActorSpawnedSignature& onSpawned);

	/**
	 * Calculate the depth ratio from the start
	 * スタート部屋からゴール部屋の部屋数からこの部屋の深さの割合を計算します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Calculate the depth ratio from the start"))
	float GetDepthRatioFromStart() const;

	/**
	 * Gets the probability (0%-100%) of adding a door. Settings after OnInitialize are invalid.
	 * ドアを追加する確率(0%～100%)を取得します。	OnInitialize以降の設定は無効です。
	 */
	float GetDoorSpawnChance() const noexcept;

	/**
	 * ドアを追加する確率(0%～100%)を設定します。	OnInitialize以降の設定は無効です。
	 */
	/** Sets the door spawn chance before initialization. 初期化前にドアの生成確率を設定します。 */
	void SetDoorSpawnChance(float doorSpawnChance) noexcept;

	/**
	 * Adds a room door and optionally registers it as a guidance-light target.
	 * 部屋にドアを追加し、必要に応じて誘導光のターゲットとして登録します。
	 */
	void AddDungeonDoor(ADungeonDoorBase* dungeonDoorBase, bool addGuidanceTarget = true);

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
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Does the room have a locked door?"))
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
	 * Returns the context tags this room provides to Interior Parts. Every Required Context Tag must be provided for a Part to match.
	 * For example, if returns the tag `kitchen`, the interior with the kitchen tag will be selected.
	 * If multiple tags are set, actors containing all tags will be selected as spawn candidates.
	 * この部屋がInterior Partsへ提供するContext Tagを返します。PartsのRequired Context Tagsをすべて提供した場合だけ一致します。
	 * 例えば、`kitchen` というタグを返した場合、キッチンのタグを持つインテリアが選択されます。
	 * 複数のタグが設定されている場合、すべてのタグを含むアクターがスポーン候補として選択されます。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Context tags this room provides to Interior Parts. Every Required Context Tag must be present for a Part to match."))
	TArray<FString> GetProvidedContextTags() const;

	/**
	 * Get the DungeonGenerator tag name.
	 * DungeonGeneratorのタグを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get the DungeonGenerator tag name."))
	static const FName& GetDungeonGeneratorTag();

	/**
	 * Get the DungeonGenerator tag name.
	 * DungeonGeneratorのタグを取得します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get the DungeonGenerator tag name."))
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
	/** Migrates legacy Blueprint defaults after loading. ロード後に旧Blueprintの既定値を移行します。 */
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
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
	friend class ADungeonRoomLightingActor;
	friend class ADungeonGenerateBase;
	friend class ADungeonGenerateActor;
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
		const float verticalGridSize,
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
	void ShiftGeneratedRoomInfoWorldOffset(const FVector& delta) noexcept;
	bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;
	void RebuildBaseFillLightComponents();
	void DestroyBaseFillLightComponents();
	static FIntPoint CalculateBaseFillLightCounts(const FVector& roomSize, float cellSize, int32 maxCountX, int32 maxCountY) noexcept;
	static float CalculateBaseFillLightHeightAboveFloor(float roomHeight, float heightFromFloorRatio) noexcept;
	static float CalculateBaseFillLightAttenuationRadius(float cellWidth, float cellDepth, float heightAboveFloor, float attenuationScale, float minRadius, float maxRadius) noexcept;
	static void ConfigureBaseFillLightComponent(UPointLightComponent* pointLightComponent, ELightUnits intensityUnits, float intensity, const FLinearColor& color, float attenuationRadius) noexcept;
	void AddGuidanceTarget(EDungeonGuidanceTargetType type, const FVector& location, const FVector& direction, float floorHeight);
	void RebuildGuidanceLightComponents();
	void DestroyGuidanceLightComponents();
	static bool AreGuidanceTargetsEquivalent(const FDungeonGuidanceTarget& left, const FDungeonGuidanceTarget& right, float duplicateDistance) noexcept;
	static int32 CalculateGuidanceLightCount(const TArray<FDungeonGuidanceTarget>& targets, bool enableDoors, bool enableStairs, int32 maxCount) noexcept;
	static FVector CalculateGuidanceLightLocation(const FBox& roomSize, const FDungeonGuidanceTarget& target, float horizontalOffset, float verticalGridSize, float doorHeightOffsetScale, float stairHeightBelowCeiling) noexcept;
	static FRotator CalculateGuidanceLightRotation(const FVector& lightLocation, const FDungeonGuidanceTarget& target, float targetHeightAboveFloor) noexcept;
	static void ConfigureGuidanceLightComponent(USpotLightComponent* spotLightComponent, ELightUnits intensityUnits, float intensity, const FLinearColor& color, float attenuationRadius, float innerConeAngle, float outerConeAngle) noexcept;
	void SetRoomLightingActor(ADungeonRoomLightingActor* lightingActor);

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
	void RequestSpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod);
	void RequestDeferredSpawnActorFromClassImpl(
		TSubclassOf<AActor> actorClass,
		const FTransform& transform,
		ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride,
		APawn* instigatorActor,
		TFunction<void(AActor*)> onSpawned);
	void RegisterSpawnedChildActor(AActor* actor);


protected:
	/**
	 * Room Bounding Box
	 * 部屋のバウンディングボックスセンサーコンポーネント
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Room Bounding Box"))
	TObjectPtr<UBoxComponent> Bounding;

	/**
	 * Enables shadow-free Point Lights that provide minimum gameplay visibility across the room.
	 * 部屋全体でゲームプレイに必要な最低限の視認性を確保する、影なしの Point Light を有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ToolTip = "Creates shadow-free Point Lights for minimum gameplay visibility across this room. These fill lights are separate from decorative torches and chandeliers."))
	bool bEnableBaseFillLights = false;

	/*
	 * Units used for the intensity of every generated base fill light.
	 * 生成される各ベース補助光の明るさに使用する単位です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ValidEnumValues = "Unitless,Candelas,Lumens", EditCondition = "bEnableBaseFillLights", ToolTip = "Units for base fill light intensity. Lumens and Candelas use physically based inverse-square falloff. Unitless uses the artistic falloff model with exponent 2."))
	ELightUnits BaseFillLightIntensityUnits = ELightUnits::Unitless;

	/*
	 * Intensity applied to every generated base fill light in the selected units.
	 * 選択した単位で指定する、生成される各ベース補助光の明るさです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "0", UIMin = "0", UIMax = "20", EditCondition = "bEnableBaseFillLights", ToolTip = "Intensity of each generated base fill Point Light in the selected units. Values above the slider range can be entered directly."))
	float BaseFillLightIntensity = 10.0f;

	/**
	 * Color applied to every generated base fill light.
	 * 生成される各ベース補助光の色です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (EditCondition = "bEnableBaseFillLights", ToolTip = "Color of each generated base fill Point Light."))
	FLinearColor BaseFillLightColor = FLinearColor::White;

	/**
	 * Target horizontal size in centimeters covered by one base fill light.
	 * 1つのベース補助光が担当する水平方向の目安サイズ（cm）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "1", EditCondition = "bEnableBaseFillLights", ToolTip = "Target room-cell size in centimeters. Larger rooms are divided into equal cells, with one Point Light at each cell center."))
	float BaseFillLightCellSize = 1200.f;

	/**
	 * Maximum number of base fill lights along the room X axis.
	 * 部屋のX軸方向に配置するベース補助光の最大数です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "1", EditCondition = "bEnableBaseFillLights", ToolTip = "Maximum number of generated base fill lights along the room X axis."))
	int32 MaxBaseFillLightCountX = 3;

	/**
	 * Maximum number of base fill lights along the room Y axis.
	 * 部屋のY軸方向に配置するベース補助光の最大数です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "1", EditCondition = "bEnableBaseFillLights", ToolTip = "Maximum number of generated base fill lights along the room Y axis."))
	int32 MaxBaseFillLightCountY = 3;

	/**
	 * Vertical position of each base fill light as a ratio of room height measured from the floor.
	 * 各ベース補助光の高さを、床から測った部屋高さに対する比率で指定します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bEnableBaseFillLights", ToolTip = "Height of each Point Light measured from the room floor as a ratio of room height. A value of 0.6 improves character and furniture visibility without placing the light at the ceiling."))
	float BaseFillLightHeightFromFloorRatio = 0.6f;

	/**
	 * Scale applied to the 3D distance from the light to the farthest cell-floor corner.
	 * ライトからセル床面の最遠角までの3D距離に掛ける倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "0", EditCondition = "bEnableBaseFillLights", ToolTip = "Multiplier applied to the 3D distance from the light to the farthest corner of its cell floor. Increase carefully because overlapping Point Lights add together."))
	float BaseFillLightAttenuationScale = 1.05f;

	/**
	 * Minimum attenuation radius in centimeters.
	 * 減衰半径の最小値（cm）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "0", EditCondition = "bEnableBaseFillLights", ToolTip = "Minimum attenuation radius in centimeters for generated base fill lights. Keep this small to avoid unnecessary light leakage in compact rooms."))
	float BaseFillLightMinAttenuationRadius = 100.f;

	/**
	 * Maximum attenuation radius in centimeters.
	 * 減衰半径の最大値（cm）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ClampMin = "0", EditCondition = "bEnableBaseFillLights", ToolTip = "Maximum attenuation radius in centimeters for generated base fill lights. Large values can leak through walls and ceilings because these Point Lights do not cast shadows."))
	float BaseFillLightMaxAttenuationRadius = 2000.f;

	/**
	 * Activates and hides Room Sensor scene components according to dungeon partition distance.
	 * ダンジョン区画との距離に応じて Room Sensor のシーンコンポーネントを表示・非表示にします。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|BaseFillLight", meta = (ToolTip = "Controls visibility by dungeon partition distance. Because this uses the generic visibility controller, other visible Scene Components added to this Room Sensor Blueprint are controlled too."))
	TObjectPtr<UDungeonComponentActivatorComponent> BaseFillLightActivator;

	/**
	 * Point Light components generated for this room.
	 * この部屋用に生成された Point Light コンポーネントです。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Information|BaseFillLight", meta = (ToolTip = "Generated base fill Point Light components. The array is rebuilt from the room bounds during sensor preparation."))
	TArray<TObjectPtr<UPointLightComponent>> BaseFillLightComponents;

	/**
	 * Enables shadow-free Spot Lights that emphasize generated doors and stairs.
	 * 生成されたドアと階段を強調する、影なしの Spot Light を有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ToolTip = "Creates shadow-free Spot Lights that emphasize generated room doors and stair entrances."))
	bool bEnableGuidanceLights = true;

	/**
	 * Enables guidance lights for generated room doors.
	 * 生成された部屋のドアに対する誘導光を有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (EditCondition = "bEnableGuidanceLights", ToolTip = "Creates guidance lights for doors that were actually generated in this room."))
	bool bEnableDoorGuidanceLights = true;

	/**
	 * Enables guidance lights for generated stairs inside this room.
	 * この部屋内に生成された階段に対する誘導光を有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (EditCondition = "bEnableGuidanceLights", ToolTip = "Creates guidance lights for stair or slope entrances that were actually generated inside this room."))
	bool bEnableStairGuidanceLights = false;

	/**
	 * Units used for the intensity of every generated guidance light.
	 * 生成される各誘導光の明るさに使用する単位です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ValidEnumValues = "Unitless,Candelas,Lumens", EditCondition = "bEnableGuidanceLights", ToolTip = "Units for guidance light intensity. Lumens and Candelas use physically based inverse-square falloff. Unitless uses the artistic falloff model with exponent 2."))
	ELightUnits GuidanceLightIntensityUnits = ELightUnits::Candelas;

	/**
	 * Base intensity of every generated guidance light in the selected units.
	 * 選択した単位で指定する、生成される各誘導光の基本となる明るさです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", UIMin = "0", UIMax = "20", EditCondition = "bEnableGuidanceLights", ToolTip = "Base intensity of each generated guidance Spot Light in the selected units before the target-type multiplier is applied. Values above the slider range can be entered directly."))
	float GuidanceLightIntensity = 8.f;

	/**
	 * Color of every generated guidance light.
	 * 生成される各誘導光の色です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (EditCondition = "bEnableGuidanceLights", ToolTip = "Color of each generated guidance Spot Light."))
	FLinearColor GuidanceLightColor = FLinearColor(1.f, 0.82f, 0.58f);

	/**
	 * Attenuation radius in centimeters.
	 * 減衰半径（cm）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights", ToolTip = "Attenuation radius in centimeters. Keep this limited to reduce light leaking through nearby walls and floors."))
	float GuidanceLightAttenuationRadius = 1200.f;

	/**
	 * Inner cone angle in degrees.
	 * 内側コーン角度（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", ClampMax = "80", EditCondition = "bEnableGuidanceLights", ToolTip = "Inner cone angle in degrees. Values are normalized so this never exceeds the outer cone angle."))
	float GuidanceLightInnerConeAngle = 18.f;

	/**
	 * Outer cone angle in degrees.
	 * 外側コーン角度（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "1", ClampMax = "80", EditCondition = "bEnableGuidanceLights", ToolTip = "Outer cone angle in degrees. Narrow angles make doors and stair entrances easier to distinguish and reduce light leakage."))
	float GuidanceLightOuterConeAngle = 32.f;

	/**
	 * Horizontal distance from the target toward the room center.
	 * ターゲットから部屋中央側へ離す水平距離です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights", ToolTip = "Horizontal distance in centimeters from the target toward the room center where the Spot Light is placed."))
	float GuidanceLightHorizontalOffset = 300.f;

	/**
	 * Height above the top of a door, expressed as a vertical-grid-size multiplier.
	 * ドア天面から上へ離す高さを垂直グリッドサイズの倍率で指定します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", UIMin = "0", UIMax = "1", EditCondition = "bEnableGuidanceLights && bEnableDoorGuidanceLights", ToolTip = "Vertical-grid-size multiplier added above the top of a generated door when placing its guidance light. The default 0.5 places the light half a vertical grid above the door."))
	float DoorGuidanceLightHeightOffsetScale = 0.5f;

	/*
	 * Distance below the room ceiling used to place stair guidance lights.
	 * 階段誘導光を配置する部屋天井からの距離です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights && bEnableStairGuidanceLights", ToolTip = "Distance in centimeters below the room ceiling used to place each stair guidance light."))
	float StairGuidanceLightHeightBelowCeiling = 100.f;

	/**
	 * Height above the target floor where the Spot Light aims.
	 * Spot Light が狙うターゲット床面からの高さです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights", ToolTip = "Height in centimeters above the target floor where each guidance Spot Light aims."))
	float GuidanceLightTargetHeightAboveFloor = 120.f;

	/**
	 * Intensity multiplier used for door targets.
	 * ドアターゲットに使用する明るさ倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights && bEnableDoorGuidanceLights", ToolTip = "Intensity multiplier applied to guidance lights for generated doors."))
	float DoorGuidanceLightIntensityMultiplier = 1.f;

	/**
	 * Intensity multiplier used for stair targets.
	 * 階段ターゲットに使用する明るさ倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights && bEnableStairGuidanceLights", ToolTip = "Intensity multiplier applied to guidance lights for generated stair entrances."))
	float StairGuidanceLightIntensityMultiplier = 1.15f;

	/**
	 * Maximum number of guidance lights generated for this room.
	 * この部屋に生成する誘導光の最大数です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Lights|GuidanceLight", meta = (ClampMin = "0", EditCondition = "bEnableGuidanceLights", ToolTip = "Maximum number of guidance lights generated for this room. Door targets are considered before stair targets when this limit is reached."))
	int32 MaxGuidanceLightCount = 8;

	/**
	 * Collected door and stair landmarks used by guidance-light generation.
	 * 誘導光生成に使用する、収集済みのドアと階段の目印です。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Information|GuidanceLight", meta = (ToolTip = "Collected world-space door and stair targets used to generate guidance lights."))
	TArray<FDungeonGuidanceTarget> GuidanceTargets;

	/**
	 * Spot Light components generated for this room.
	 * この部屋用に生成された Spot Light コンポーネントです。
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Information|GuidanceLight", meta = (ToolTip = "Generated guidance Spot Light components. The array is rebuilt after doors and stairs have been collected."))
	TArray<TObjectPtr<USpotLightComponent>> GuidanceLightComponents;

	/**
	 * Server-side reference to the replicated visual proxy for this room's generated lights.
	 * この部屋の生成ライトを同期する表示専用Proxyへのサーバー側参照です。
	 */
	UPROPERTY(Transient)
	TObjectPtr<ADungeonRoomLightingActor> RoomLightingActor;

	/**
	 * Generator that created this server-only Room Sensor, used to scope bulk light updates.
	 * このサーバー専用Room Sensorを生成したGeneratorで、一括ライト更新の対象範囲に使用します。
	 */
	UPROPERTY(Transient)
	TObjectPtr<ADungeonGenerateBase> DungeonGeneratorOwner;

	/**
	 * the probability (0%-100%) of adding a door
	 * ドアを追加する確率(0%～100%)
	 */
	UPROPERTY(meta = (DeprecatedProperty))
	uint8 DoorAddingProbability = 100;

	/**
	 * Percentage chance of adding each eligible door.
	 * 配置可能な各ドアを追加する確率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100", ForceUnits = "Percent", ToolTip = "Chance of adding each eligible door. 0% never adds it; 100% always adds it."))
	float DoorSpawnChance = 100.f;

	/**
	 * Automatically calls OnReset when the player leaves
	 * プレイヤーが離れたら自動的にOnResetを呼ぶ
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Automatically calls OnReset when the player leaves"))
	bool AutoReset = true;

#if WITH_EDITORONLY_DATA
	/**
	 * Display debugging information
	 * デバッグ情報を表示
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator", meta = (ToolTip = "Display debugging information"))
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

	/**
	 * Enemy spawn count multipliers by gameplay role.
	 * ゲームプレイ役割ごとの敵スポーン数倍率です。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|SpawnActorInRoom", meta = (ToolTip = "Enemy spawn count multipliers by GameplayRole. Edit the field for each role directly; 0 disables enemy spawning for that role."))
	FDungeonGameplayRoleEnemySpawnMultipliers GameplayRoleEnemySpawnMultipliers;

	/**
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
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|MissionGraph", meta = (ToolTip = "Key actor in the mission graph. Spawns at a random location in the room if specified. If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.", AllowedClasses = "/Script/Engine.Blueprint"))
	FSoftObjectPath SpawnKeyActor;

	/**
	 * Unique key actor in the mission graph.
	 * Spawns at a random location in the room if specified.
	 * If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.
	 * ミッショングラフのユニーク鍵アクター
	 * 指定した場合は部屋のランダムな位置にスポーンします。
	 * 宝箱や敵を倒した時に鍵を入手させたい場合は独自に実装して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Helper|MissionGraph", meta = (ToolTip = "Unique key actor in the mission graph. Spawns at a random location in the room if specified. If you want the key to be obtained from a treasure chest or when an enemy is defeated, implement it independently.", AllowedClasses = "/Script/Engine.Blueprint"))
	FSoftObjectPath SpawnUniqueKeyActor;

	/**
	 * Room Size
	 * 部屋の大きさ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding", meta = (ToolTip = "Room Size"))
	FBox RoomSize;

	/**
	 * Horizontal margin of room bounding box sensor
	 * 部屋のバウンディングボックスセンサーの水平マージン
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding", meta = (ToolTip = "Horizontal margin of room bounding box sensor", ClampMin = "0"))
	float HorizontalMargin = 0.f;

	/**
	 * Vertical margin of room bounding box sensor
	 * 部屋のバウンディングボックスセンサーの垂直マージン
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information|Bounding", meta = (ToolTip = "Vertical margin of room bounding box sensor", ClampMin = "0"))
	float VerticalMargin = 0.f;

	/**
	 * Room Identifier
	 * 部屋の識別子
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Room Identifier", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Identifier or RoomInfo.Identifier instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	int32 Identifier = 0;

	/**
	 * Type of room parts (start, goal, hall, etc.)
	 * 部屋パーツの種類（スタート、ゴール、ホール等）
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Type of room parts (start, goal, hall, etc.)", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Parts or RoomInfo.Parts instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	/**
	 * Types of items that should be placed in the room
	 * 部屋に配置すべきアイテムの種類
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Types of items that should be placed in the room", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().Item or RoomInfo.Item instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	/**
	 * Branch identifier generated by the mission graph
	 * ranchId を表します。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Branch identifier generated by the mission graph", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().BranchId or RoomInfo.BranchId instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 BranchId = 0;

	/**
	 * Room depth from the starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 * スタート部屋から部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Room depth from the starting room Can be used to change the background music as you go deeper into the room, etc.", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().DepthFromStart or RoomInfo.DepthFromStart instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 DepthFromStart = 0;

	/**
	 * Depth of deepest room from starting room
	 * Can be used to change the background music as you go deeper into the room, etc.
	 * スタート部屋から最も深い部屋の深さ
	 * 奥に行くほどBGMが変化するなどに利用できます
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Depth of deepest room from starting room Can be used to change the background music as you go deeper into the room, etc.", DeprecatedProperty, DeprecationMessage = "Use GetGeneratedRoomInfo().DeepestDepthFromStart or RoomInfo.DeepestDepthFromStart instead. This legacy v1 room-information field may be removed in v2.1 or later."))
	uint8 DeepestDepthFromStart = 0;

	/**
	 * Generated room information exposed to gameplay Blueprints.
	 * ゲームプレイBlueprintへ公開する生成部屋情報です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Information", meta = (ToolTip = "Generated room information exposed to gameplay Blueprints."))
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
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors", meta = (ToolTip = "Room door"))
	TArray<TObjectPtr<ADungeonDoorBase>> DungeonDoors;

	/**
	 * Torchlight in a room
	 * 部屋の燭台
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors", meta = (ToolTip = "Torchlight in a room"))
	TArray<TObjectPtr<AActor>> DungeonTorches;

	/**
	 * Chandelier in a room
	 * 部屋のシャンデリア
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Information|Actors", meta = (ToolTip = "Chandelier in a room"))
	TArray<TObjectPtr<AActor>> DungeonChandeliers;

	/**
	 * Room objects (excluding doors, candlesticks, and chandeliers)
	 * 部屋のアクター（ドア、燭台、シャンデリアを除く）
	 */
	UPROPERTY(BlueprintReadWrite, Category = "DungeonGenerator|Information|Actors", meta = (ToolTip = "Room objects (excluding doors, candlesticks, and chandeliers)"))
	TArray<TObjectPtr<AActor>> DungeonRoomSpawnedActors;

private:
	CDungeonRandom mSynchronizedRandom;
	CDungeonRandom mLocalRandom;
	float mHorizontalGridSize = 0.f;
	/*
	 * Vertical size in centimeters of one generated dungeon grid.
	 * 生成ダンジョンの1グリッド分の垂直サイズ（cm）です。
	 */
	float mVerticalGridSize = 0.f;
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
