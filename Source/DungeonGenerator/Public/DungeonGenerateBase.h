/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonDeferredActorSpawnManager.h"
#include "DungeonGenerationPerformanceSettings.h"
#include "DungeonInstancedMeshCluster.h"
#include "DungeonInteriorPlacement.h"
#include "Helper/DungeonRandom.h"
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include "Parameter/DungeonGridSize.h"
#include "Parameter/DungeonLayoutTypes.h"
#include "Validation/DungeonValidationIssue.h"


#include <CoreMinimal.h>
#include <CollisionQueryParams.h>
#include <GameFramework/Actor.h>
#include <EngineUtils.h>
#include <Containers/Array.h>

#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>
#include "DungeonGenerateBase.generated.h"

// Forward declaration
class UDungeonAisleGridMap;
class ADungeonAisleSlopeLightingActor;
class ADungeonDoorBase;
class ADungeonMainLevelScriptActor;
class ADungeonRoomSensorBase;
class ADungeonRoomLightingActor;
class ADungeonSubLevelScriptActor;
class ADungeonGenerateBase;
class UDungeonGenerateParameter;
class UDungeonComponentActivatorComponent;
class UDungeonMiniMapWidget;
class FDungeonVegetationJobQueue;
class ANavMeshBoundsVolume;
class APlayerStart;
class AStaticMeshActor;
class ULevel;
class ULevelStreamingDynamic;
class UStaticMesh;
struct FDungeonGeneratedRoomInfo;


namespace dungeon
{
	class Identifier;
	class Generator;
	class Grid;
	class Random;
	class Room;
	class Voxel;
	struct GenerateParameter;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationSuccessSignature);
/**
 * Notifies that dungeon generation failed.
 * The reason is not passed as an argument. Call ADungeonGenerateBase::GetLastGenerationIssues()
 * on the actor that raised the event to read it.
 * ダンジョンの生成に失敗した事を通知します。
 * 理由は引数では渡されません。イベントを発行したアクターの
 * ADungeonGenerateBase::GetLastGenerationIssues() を呼び出して取得して下さい。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationFailureSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationCompleteSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGenerateBaseOnBeginGenerateSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonGenerateBaseOnEndGenerateSignature, UDungeonRandom*, synchronizedRandom, const UDungeonAisleGridMap*, aisleGridMap);

/**
 * Abstract pipeline shared by runtime and editor dungeon generators.
 * It transforms parameters into a core voxel layout, builds terrain and content, manages streamed rooms,
 * and gates gameplay and visual completion while deferred work is processed across ticks.
 *
 * RuntimeとEditorのDungeon Generatorが共有する抽象Pipelineです。
 * ParameterからCore Voxel Layoutを生成し、地形・Content・Streaming Roomを構築した後、
 * Tickへ分散した処理の完了に応じてGameplay準備完了と視覚生成完了を通知します。
 */
UCLASS(Abstract, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonGenerateBase : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Per-generation performance settings copied when generation begins.
	 * 生成開始時にコピーされるGeneration単位の性能設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Controls how Actor spawning and vegetation work are distributed across frames. Changes made during generation take effect on the next generation."))
	FDungeonGenerationPerformanceSettings GenerationPerformance;

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
	 * @param[in]	randomSeedOverride Optional seed supplied by synchronized generation state.
	 * @return		If false, generation fails
	 */
	bool BeginDungeonGeneration(const UDungeonGenerateParameter* parameter, const bool hasAuthority, TOptional<int32> randomSeedOverride = TOptional<int32>());
	bool BeginDungeonGenerationPhase_Prepare(const UDungeonGenerateParameter* parameter, bool hasAuthority, dungeon::GenerateParameter& generateParameter, const TOptional<int32>& randomSeedOverride);
	bool BeginDungeonGenerationPhase_InitializeCore(const dungeon::GenerateParameter& generateParameter);
	bool BeginDungeonGenerationPhase_RunGenerator(dungeon::GenerateParameter& generateParameter, bool hasAuthority);
	void BeginDungeonGenerationPhase_BuildWorld(const dungeon::GenerateParameter& generateParameter, bool hasAuthority);

	/**
	 * End Generate dungeon
	 * ダンジョン生成を終了
	 */
	void EndDungeonGeneration();

	/**
	 * Processes every queued deferred Actor immediately for synchronous editor generation.
	 * 同期エディタ生成のため、キュー済み遅延Actorをすべて即時処理します。
	 */
	void FlushDeferredActorSpawning();

	/**
	 * Processes every queued Actor and vegetation task immediately for synchronous editor generation.
	 * 同期エディタ生成のため、キュー済みActorと植生処理をすべて即時処理します。
	 */
	void FlushDistributedGeneration();

private:
	/**
	 * Resets the completion state for a new, cancelled, disposed, or failed generation.
	 * 新規生成、キャンセル、破棄、失敗に備えて生成完了状態をリセットします。
	 */
	void ResetGenerationCompletionState() noexcept;

	/**
	 * Broadcasts gameplay readiness after every deferred actor request has been processed.
	 * すべての遅延Actor生成要求を処理した後、ゲームプレイ準備完了を通知します。
	 */
	void TryFinalizeGenerationSuccess();

	/**
	 * Marks generation complete and broadcasts its event when all visual completion conditions are met.
	 * すべての視覚的な完了条件を満たしたとき、生成完了イベントを通知します。
	 */
	void TryCompleteGeneration();

public:
	/**
	 * Returns whether core layout and world construction succeeded, even if deferred visual work remains.
	 * Core LayoutとWorld構築が成功したかを返します。遅延Visual処理が残っている場合もtrueです。
	 */
	bool IsGenerated() const noexcept;

	/**
	 * Returns whether every gameplay and visual element for the current successful generation is ready.
	 * 現在の生成に含まれるゲームプレイ要素と視覚要素の準備がすべて完了したかを返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Returns true after generation succeeds and every deferred actor, vegetation instance, and foliage tree is ready. Use OnGenerationSuccess when only gameplay-required actors need to be ready."))
	bool IsGenerationComplete() const noexcept;

	/**
	 * Notifies once after successful dungeon generation when every gameplay and visual element is ready.
	 * ダンジョン生成成功後、ゲームプレイ要素と視覚要素の準備がすべて完了したときに一度だけ通知します。
	 */
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event", meta = (ToolTip = "Called once after OnGenerationSuccess when every deferred visual element, including distant vegetation and foliage trees, is ready. Use this to close a loading screen after generation is fully complete."))
	FDungeonGeneratorActorNotifyGenerationCompleteSignature OnGenerationComplete;

protected:
	/**
	 * Returns whether generated content may still exist in the world.
	 * ワールドに生成物が残っている可能性があるかを返します。
	 * 生成が失敗した場合もtrueになるため、後始末の要否はIsGeneratedではなくこちらで判定して下さい。
	 * @return		後始末が必要ならtrue
	 */
	bool IsDisposeRequired() const noexcept;

public:
	/**
	 * Cancels pending generation work and releases generated actors, instances, core data, and streamed levels.
	 * 保留中の生成処理をCancelし、生成Actor・Instance・Core Data・Streaming Levelを解放します。
	 */
	virtual void Dispose(const bool flushStreamLevels);

	/**
	 * Get start position
	 * StartLocation を返します。
	 */
	FVector GetStartLocation() const;

	/**
	 * Returns StartBoundingBox.
	 * スタート部屋のバウンディングボックスを取得します
	 */
	FBox GetStartBoundingBox() const;

	/**
	 * Get goal position
	 * GoalLocation を返します。
	 */
	FVector GetGoalLocation() const;

	/**
	 * Get a bounding box covering the entire generated dungeon
	 * BoundingBox を計算します。
	 */
	FBox CalculateBoundingBox() const;

	/**
	 * Gets the minimum and maximum Z values that contain visible generated grid cells.
	 * 表示可能な生成済みグリッドセルを含む最小および最大のZ値を取得します。
	 */
	bool GetVisibleGridHeightRange(int32& minZ, int32& maxZ) const noexcept;

	/**
	 * Get the length of the longest straight line
	 * 最も長い直線の長さを取得します
	 */
	FVector2D GetLongestStraightPath() const noexcept;

	/**
	 * Get metrics from the selected dungeon layout candidate.
	 * 選択されたダンジョンレイアウト候補の品質指標を取得します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Layout", meta = (ToolTip = "Get metrics from the selected dungeon layout candidate."))
	FDungeonLayoutMetrics GetLastLayoutMetrics() const noexcept;

	/**
	 * Get score information from the selected dungeon layout candidate.
	 * 選択されたダンジョンレイアウト候補のスコア情報を取得します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Layout", meta = (ToolTip = "Get score information from the selected dungeon layout candidate."))
	FDungeonLayoutScore GetLastLayoutScore() const noexcept;

	/**
	 * Get the issues reported by the last dungeon generation.
	 * 直前のダンジョン生成が報告した問題を取得します。
	 * 生成に失敗した場合はErrorが、要求の一部を満たせなかった場合はWarningが入ります。
	 * 要求どおりのダンジョンが生成できた場合は空になります。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Layout", meta = (ToolTip = "Get the issues reported by the last dungeon generation. It holds an Error when generation failed and a Warning when the dungeon does not match every request. The array is empty when the dungeon matches every request."))
	const TArray<FDungeonValidationIssue>& GetLastGenerationIssues() const noexcept;

protected:
	/**
	 * Appends an issue to the list returned by GetLastGenerationIssues.
	 * Use it for problems that are found outside the generator itself.
	 * GetLastGenerationIssuesが返す一覧へ問題を追加します。
	 * 生成器の外側で見つかった問題を報告するために使います。
	 * @param[in]	issue	追加する問題
	 */
	void AddGenerationIssue(const FDungeonValidationIssue& issue);

	/**
	 * Marks the generated dungeon as unusable so that EndDungeonGeneration takes the failure path.
	 * Call it before EndDungeonGeneration; afterwards the success has already been announced.
	 * 生成したダンジョンを使用不可として印を付け、EndDungeonGenerationが失敗の経路を通るようにします。
	 * EndDungeonGenerationより前に呼んで下さい。後から呼んでも成功は通知済みです。
	 */
	void InvalidateGeneratedDungeon();

private:
	/**
	 * 生成の失敗を利用者向けの問題として記録し、ログへ出力します
	 * dungeon::Generator::Errorは前方宣言しかできないためuint8で受け取ります
	 * @param[in]	generatorError	生成器が報告したエラー
	 */
	void ReportGenerationFailure(const uint8 generatorError);

	/**
	 * 生成は成功したものの要求を満たせなかった内容を利用者向けの問題として記録し、ログへ出力します
	 * dungeon::Generator::Warningは前方宣言しかできないためuint8で受け取ります
	 * @param[in]	warningFlags	生成器が報告した警告のビット和
	 */
	void ReportGenerationWarnings(const uint8 warningFlags);

	/**
	 * Issues reported by the last dungeon generation.
	 * 直前のダンジョン生成が報告した問題です。
	 */
	TArray<FDungeonValidationIssue> mLastGenerationIssues;

public:

	////////////////////////////////////////////////////////////////////////////
	// 乱数
private:
	std::shared_ptr<dungeon::Random> GetSynchronizedRandom() const noexcept;
	const std::shared_ptr<dungeon::Random>& GetRandom() const noexcept;

	void InvalidateVisibleGridHeightRange() noexcept;
	void CacheVisibleGridHeightRange() noexcept;
	void ResetGeneratedRoomSnapshots() noexcept;
	void CacheGeneratedRoomSnapshots();
	void ShiftGeneratedRoomSnapshots(const FVector& delta) noexcept;

	////////////////////////////////////////////////////////////////////////////
	// アクターのスポーンと破棄
public:
	/**
	 * Spawns an Actor, assigns its editor folder, and tags it for generator-owned cleanup.
	 * Actorを生成してEditor Folderと生成物Tagを設定し、Generatorによる一括破棄の対象にします。
	 */
	static AActor* SpawnActorWithFolderPath(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters);

	/**
	 * Queues an actor spawn in this generator's world and invokes the callback after spawning completes.
	 * この生成アクターのワールドでアクター生成を予約し、生成完了後にコールバックを呼び出します。
	 */
	void DeferredSpawnActorWithFolderPath(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters, const TFunction<void(AActor*)>& onSpawned, UObject* requestOwner = nullptr);
	/**
	 * Queues an actor spawn in the specified world and invokes the callback after spawning completes.
	 * 指定ワールドでアクター生成を予約し、生成完了後にコールバックを呼び出します。
	 */
	void DeferredSpawnActorWithFolderPath(UWorld* world, UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters, const TFunction<void(AActor*)>& onSpawned, UObject* requestOwner = nullptr);

	/**
	 * Returns an Actor's component activator, creating and registering one when absent.
	 * ActorのComponent Activatorを返し、存在しない場合は生成して登録します。
	 */
	static UDungeonComponentActivatorComponent* FindOrAddComponentActivatorComponent(AActor* actor);

private:
	AActor* SpawnActorWithFolderPath(UClass* actorClass, const FString& folderPath, const FTransform& transform, const FActorSpawnParameters& actorSpawnParameters) const;
	template<typename T = AActor> T* SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	template<typename T = AActor> T* SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	void DestroySpawnedActors() const;

protected:
	static void DestroySpawnedActors(UWorld* world);

	static void DisableSpawnedActorNavigation(const TArray<AActor*>& actors);

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
	 * Get dungeon generation core object.
	 * @return		dungeon::Generator
	 * Generator を返します。
	 */
	std::shared_ptr<const dungeon::Generator> GetGenerator() const;

	/**
	 * Notification when a dungeon is successfully created
	 * ダンジョンの生成に成功した時の通知
	 */
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationSuccessSignature OnGenerationSuccess;

	/**
	 * Notification when dungeon creation fails.
	 * Call GetLastGenerationIssues() from the handler to find out why the generation failed.
	 * It returns the reason with a code, a message, a fix hint, the parameter to review and the
	 * related sub-level asset, so the handler can branch on it and retry with another setup.
	 * ダンジョンの生成に失敗した時の通知
	 * 失敗した理由は、ハンドラからGetLastGenerationIssues()を呼び出して取得して下さい。
	 * コード、メッセージ、対処のヒント、確認すべきパラメータ、関連するサブレベルアセットが
	 * 得られるため、理由に応じて別の設定で再生成する、といった判断ができます。
	 */
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationFailureSignature OnGenerationFailure;

	////////////////////////////////////////////////////////////////////////////
	/**
	 * This event is called at the start of the Create function
	 * Create関数開始時に呼び出されるイベントです
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "This event is called at the start of the Create function"))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Event to query the creation of an aisle. If true is returned, the roof and aisle meshes are not generated."))
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Event called at the end of the Create function synchronizedRandom is a random number that is synchronized between clients. It must always be called the same number of times on server and client. aisleGridMap is a container for the generated aisle grid"))
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

	// dungeon::Generator::Generate前イベント
	virtual void OnPreDungeonGeneration();

	// dungeon::Generator::Generate後イベント
	virtual void OnPostDungeonGeneration(const bool result);

	////////////////////////////////////////////////////////////////////////////
	// Terrain
public:
	// event
	/**
	 * Callback signature used when a generated non-pillar and non-roof static mesh is added.
	 * 柱と屋根以外の生成スタティックメッシュ追加時に使うコールバック型です。
	 */
	using AddStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;

	/**
	 * Callback signature used when a generated roof static mesh is added.
	 * 生成された屋根スタティックメッシュ追加時に使うコールバック型です。
	 */
	using AddRoofStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&, const FVector&)>;

	/**
	 * Callback signature used when a generated pillar static mesh is added.
	 * 生成された柱スタティックメッシュ追加時に使うコールバック型です。
	 */
	using AddPillarStaticMeshEvent = std::function<void(UStaticMesh*, const FTransform&)>;

	/**
	 * Registers the callback invoked for generated floor meshes.
	 * 生成された床メッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddFloor(const AddStaticMeshEvent& function);

	/**
	 * Registers the callback invoked for generated slope meshes.
	 * 生成されたスロープメッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddSlope(const AddStaticMeshEvent& function);

	/**
	 * Registers the callback invoked for generated wall meshes.
	 * 生成された壁メッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddWall(const AddStaticMeshEvent& function);

	/**
	 * Registers the callback invoked for generated roof meshes.
	 * 生成された天井メッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddRoof(const AddRoofStaticMeshEvent& function);

	/**
	 * Registers the callback invoked for generated pillar meshes.
	 * 生成された柱メッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddPillar(const AddPillarStaticMeshEvent& function);

	/**
	 * Registers the callback invoked for generated catwalk meshes.
	 * 生成された中二階通路メッシュごとに呼び出すコールバックを登録します。
	 */
	void OnAddCatwalk(const AddStaticMeshEvent& function);


private:
	using RoomAndRoomSensorMap = std::unordered_map<const dungeon::Room*, ADungeonRoomSensorBase*>;

	/**
	 * Precomputed coordinate context passed through each voxel-to-world construction step.
	 * 各VoxelからWorldへの構築処理へ渡す、事前計算済み座標Contextです。
	 */
	struct CreateImplementParameter final
	{
		/**
		 * Integer location of the cell currently being converted into world content.
		 * 現在World Contentへ変換しているCellの整数座標です。
		 */
		const FIntVector& mGridLocation;

		/**
		 * Stable linear voxel index used for deterministic part selection.
		 * 決定的なParts選択に使用する安定したVoxel線形Indexです。
		 */
		size_t mGridIndex;

		/**
		 * Generated grid attributes that decide which terrain and fixtures may be built.
		 * 生成可能な地形とFixtureを決定するGrid属性です。
		 */
		const dungeon::Grid& mGrid;

		/**
		 * World-space location of the cell's minimum corner.
		 * Cell最小CornerのWorld座標です。
		 */
		const FVector& mPosition;

		/**
		 * World-space dimensions of one grid cell.
		 * 1 Grid CellのWorld空間寸法です。
		 */
		const FVector& mGridSize;

		/**
		 * Half extents of one grid cell used to position centered content.
		 * 中心基準Contentの配置に使用する1 Grid Cellの半Extentです。
		 */
		const FVector& mGridHalfSize;

		/**
		 * World-space center of the current cell.
		 * 現在のCell中心のWorld座標です。
		 */
		const FVector& mCenterPosition;
	};

	/**
	 * Candidate grid used to resolve fixture settings near shared edges and corners.
	 * 共有された辺や角の近くで Fixture 設定を解決するために使う候補グリッドです。
	 */
	struct FixtureGridCandidate final
	{
		/**
		 * Voxel location of the candidate grid.
		 * 候補グリッドのボクセル位置です。
		 */
		FIntVector mGridLocation;

		/**
		 * Stable voxel index used for deterministic tie breaking and parts selection.
		 * 安定した tie-break とパーツ選択に使うボクセルインデックスです。
		 */
		size_t mGridIndex;

		/**
		 * Grid data used as the fixture selection context.
		 * Fixture 選択コンテキストとして使うグリッド情報です。
		 */
		const dungeon::Grid* mGrid;

		/**
		 * Represents FixtureGridCandidate.
		 * 空の無効な候補を作成します。
		 */
		FixtureGridCandidate() noexcept
			: mGridLocation(FIntVector::ZeroValue)
			, mGridIndex(0)
			, mGrid(nullptr)
		{}

		/**
		 * Represents FixtureGridCandidate.
		 * ボクセル位置とグリッド参照から有効な候補を作成します。
		 */
		FixtureGridCandidate(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid) noexcept
			: mGridLocation(gridLocation)
			, mGridIndex(gridIndex)
			, mGrid(&grid)
		{}
	};

	/**
	 * Deferred wall output retained between voxel analysis and deterministic mesh emission.
	 * Voxel解析から決定的なMesh出力まで保持する遅延Wall出力です。
	 */
	struct ReservedWallInfo final
	{
		/**
		 * Wall mesh selected during the voxel decision pass.
		 * Voxel判定Passで選択されたWall Meshです。
		 */
		UStaticMesh* mStaticMesh;

		/**
		 * Final world transform emitted during the wall output pass.
		 * Wall出力Passで使用する最終World Transformです。
		 */
		FTransform mTransform;

		/**
		 * Stores one wall output selected before the deterministic emission pass.
		 * 決定的な出力Passより前に選択された1つのWall出力を保持します。
		 */
		ReservedWallInfo(UStaticMesh* staticMesh, const FTransform& transform)
			: mStaticMesh(staticMesh)
			, mTransform(transform)
		{}
	};

	void CreateImplement_QueryAisleGeneration(const bool hasAuthority);
	void CreateImplement_AddTerrain(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority);
	void CreateImplement_AddFloorAndSlope(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, bool hasAuthority) const;
	void CreateImplement_ReserveWall(const CreateImplementParameter& cp);
	void CreateImplement_AddWall();
	void CreateImplement_AddRoof(const CreateImplementParameter& cp) const;
	void CreateImplement_AddDoor(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;
	static bool CanAddDoorGuidanceTarget(const dungeon::Grid& upperGrid) noexcept;
	bool CanAddDoor(const ADungeonRoomSensorBase* dungeonRoomSensorBase, const FIntVector& location, const dungeon::Grid& grid) const;
	void CreateImplement_AddPillarAndTorch(const CreateImplementParameter& cp, ADungeonRoomSensorBase* dungeonRoomSensorBase, const bool hasAuthority) const;
	void CreateImplement_AddChandelier(const RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const;
	FixtureGridCandidate MakeFixtureGridCandidate(const FIntVector& location) const;
	void AddFixtureGridCandidate(std::vector<FixtureGridCandidate>& candidates, const FIntVector& location) const;
	FixtureGridCandidate SelectFixtureGridCandidate(const std::vector<FixtureGridCandidate>& candidates, const FixtureGridCandidate& fallback) const;
	int32 GetFixtureGridCandidatePriority(const dungeon::Grid& grid) const;

	// Room sensor
	void CreateImplement_PrepareSpawnRoomSensor(RoomAndRoomSensorMap& roomSensorCache, const bool hasAuthority) const;
	void CreateImplement_FinishSpawnRoomSensor(const RoomAndRoomSensorMap& roomSensorCache);

	// Navigation
	void CreateImplement_Navigation(const bool hasAuthority);
	static int32 CalculateRecommendedNavMeshTilePoolSize(const FBox& bounds, double tileSize, int32 floorCount) noexcept;
	void CheckRecastNavMesh() const;

protected:
	/**
	 * Finalizes generated terrain components before collision-dependent decoration begins.
	 * 衝突判定を使う装飾処理を始める前に、生成した地形Componentを確定します。
	 */
	virtual void FinalizeGeneratedTerrain();

	virtual void FitNavMeshBoundsVolume();

	/**
	 * PlayerStartPIEアクターを除くPlayerStartアクターを収集してstartPointsに記録します
	 */
	void CollectPlayerStartExceptPlayerStartPIE(TArray<APlayerStart*>& startPoints);

	/**
	 * PlayerStartアクターを移動します
	 */
	void MovePlayerStart(const TArray<APlayerStart*>& startPoints);

private:
	enum class EStaticMeshPartitionRegistrationFace : uint8
	{
		None,
		PositiveY,
		PositiveZ,
		NegativeZ,
	};

	static FVector CalculateRoofPartitionRegistrationWorldLocation(const FVector& gridCenterWorldLocation, const FVector& gridSize) noexcept;
	AStaticMeshActor* SpawnStaticMeshActor(UStaticMesh* staticMesh, const FString& folderPath, const FTransform& transform, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod, EStaticMeshPartitionRegistrationFace registrationFace = EStaticMeshPartitionRegistrationFace::None, TOptional<FVector> fixedPartitionRegistrationWorldLocation = TOptional<FVector>(), bool affectsNavigation = true) const;
	ADungeonAisleSlopeLightingActor* SpawnAisleSlopeLightingActor(const FVector& worldLocation, const FDungeonAisleSlopeBaseLightSettings& settings) const;
	ADungeonRoomLightingActor* SpawnRoomLightingActor(ADungeonRoomSensorBase* roomSensor) const;
	ADungeonDoorBase* SpawnDoorActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, const EDungeonRoomProps props, bool addGuidanceTarget) const;
	AActor* SpawnTorchActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod, const bool castShadow) const;
	AActor* SpawnChandelierActor(UClass* actorClass, const FTransform& transform, ADungeonRoomSensorBase* ownerActor, ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const;
	ADungeonRoomSensorBase* SpawnRoomSensorActorDeferred(
		UClass* actorClass,
		const dungeon::Identifier& identifier,
		const FVector& center,
		const FVector& extents,
		EDungeonRoomParts parts,
		EDungeonRoomItem item,
		const FDungeonGeneratedRoomInfo& roomInfo,
		uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart) const;
	void FinishRoomSensorActorSpawning(ADungeonRoomSensorBase* dungeonRoomSensor) const;

	////////////////////////////////////////////////////////////////////////////
	// Streaming Level
	bool ShiftGeneratedDungeonWorldOffset(const FVector& delta);
	bool AlignGeneratedDungeonStartRoomBoundsMinToWorldLocation(const FVector& targetLocation, const bool alignXYOnly = false);


	////////////////////////////////////////////////////////////////////////////
	// MiniMap
public:

	////////////////////////////////////////////////////////////////////////////
	// Interior
private:

	////////////////////////////////////////////////////////////////////////////
	// Vegetation
public:

private:
	/**
	 * Internal phase of the dungeon generation pipeline.
	 * ダンジョン生成パイプラインの内部フェーズです。
	 */
	enum class EDungeonGenerationPhase : uint8
	{
		Idle,
		Preparing,
		BuildingCore,
		BuildingTerrain,
		WaitingForTerrainCollision,
		BuildingContent,
		Finalizing,
		Completed,
		Failed,
		Cancelling
	};

	static const TCHAR* GetGenerationPhaseName(EDungeonGenerationPhase phase) noexcept;
	static bool IsGenerationPhaseTransitionAllowed(EDungeonGenerationPhase from, EDungeonGenerationPhase to) noexcept;
	bool SetGenerationPhase(EDungeonGenerationPhase phase);
	bool SetGenerationPhaseForGeneration(uint64 generationLifecycleId, EDungeonGenerationPhase phase);
	bool IsCoreReady() const noexcept;
	bool IsTerrainCollisionReady() const noexcept;

	/**
	 * Temporarily switches the actor to every-frame ticking while distributed generation work is pending.
	 * 分散生成処理が残っている間、一時的にActorを毎フレームTickへ切り替えます。
	 */
	void BeginDistributedGenerationEveryFrameTick();

	/**
	 * Restores the actor tick interval saved before distributed generation processing.
	 * 分散生成処理の開始前に保存したActor Tick間隔を復元します。
	 */
	void RestoreDistributedGenerationTickInterval();

	/**
	 * Restores normal ticking when no distributed Actor or vegetation work remains.
	 * Actorまたは植生の分散処理が残っていない場合に通常のTickへ戻します。
	 */
	void TryRestoreDistributedGenerationTickInterval();

	/**
	 * Copies and validates editable performance settings for the next generation.
	 * 編集可能な性能設定を次のGeneration用にコピーして検証します。
	 */
	void SnapshotGenerationPerformanceSettings();


	////////////////////////////////////////////////////////////////////////////
	// Debug
public:
	/**
	 * Calculate CRC32
	 * @return		CRC32
	 * CRC32 を計算します。
	 */
	uint32_t CalculateCRC32() const noexcept;

#if WITH_EDITOR
	/**
	 * Draws the selected room, aisle, and voxel diagnostics for the generated dungeon.
	 * 生成Dungeonについて、選択されたRoom・Aisle・Voxel診断情報を描画します。
	 */
	void DrawDebugInformation(const bool showRoomAisleInformation, const bool showVoxelGridType) const;
#endif

private:
#if WITH_EDITOR
	void DrawRoomAisleInformation() const;
	void DrawVoxelGridType() const;
#endif

	////////////////////////////////////////////////////////////////////////////
	// overrides
public:
	virtual void Tick(float DeltaSeconds) override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	////////////////////////////////////////////////////////////////////////////
	// member variables
	// Spatial instanced meshes
	/**
	 * XYZ spatial clusters shared by generated ISM/HISM and vegetation.
	 * 生成ISM/HISMと植生が共有するXYZ空間クラスターです。
	 */
	UPROPERTY(Transient)
	TMap<FIntVector, FDungeonInstancedMeshCluster> mInstancedMeshClusters;

	/**
	 * Generation parameter asset currently used by the generator.
	 *
	 * ジェネレーターが現在使用する生成パラメータアセットです。
	 */
	UPROPERTY(Transient)
	const UDungeonGenerateParameter* mParameter;

	/**
	 * Cached aisle occupancy map used during generation and verification.
	 *
	 * 生成・検証で利用する通路占有マップのキャッシュです。
	 */
	UPROPERTY(Transient)
	UDungeonAisleGridMap* mAisleGridMap;

private:
	/**
	 * Core-independent value snapshot used to place PlayerStart actors in a generated start room.
	 * 生成された開始部屋へPlayerStart Actorを配置するための、Coreに依存しない値スナップショットです。
	 */
	struct GeneratedStartRoomSnapshot final
	{
		/**
		 * Stable identifier of the generated start room.
		 * 生成された開始部屋の安定した識別子です。
		 */
		int32 Identifier = INDEX_NONE;

		/**
		 * World-space bounds of the generated start room.
		 * 生成された開始部屋のワールド空間Boundsです。
		 */
		FBox Bounds = FBox(ForceInit);

		/**
		 * Room sensor successfully generated for this start room.
		 * この開始部屋に正常に生成されたRoom Sensorです。
		 */
		TWeakObjectPtr<ADungeonRoomSensorBase> RoomSensor;
	};

	/**
	 * Generated start rooms cached without retaining Core room objects.
	 * CoreのRoom Objectを保持せずにキャッシュした生成済み開始部屋です。
	 */
	TArray<GeneratedStartRoomSnapshot> mGeneratedStartRoomSnapshots;

	/**
	 * Cached primary start location that remains valid after Core objects are released.
	 * Core Object解放後も有効な、キャッシュ済みのプライマリ開始位置です。
	 */
	FVector mGeneratedStartLocation = FVector::ZeroVector;

	/**
	 * Cached primary start-room bounds that remain valid after Core objects are released.
	 * Core Object解放後も有効な、キャッシュ済みのプライマリ開始部屋Boundsです。
	 */
	FBox mGeneratedStartRoomBounds = FBox(ForceInit);

	/**
	 * Whether the cached primary start location and bounds are valid.
	 * キャッシュ済みのプライマリ開始位置とBoundsが有効かを示します。
	 */
	bool mGeneratedStartSnapshotValid = false;

	/**
	 * Core generator that owns the current room graph and voxel layout.
	 * 現在のRoom GraphとVoxel Layoutを所有するCore Generatorです。
	 */
	std::shared_ptr<dungeon::Generator> mGenerator;

	/**
	 * Local-only random stream for work that must not affect network-synchronized decisions.
	 * Network同期対象の判定へ影響させない処理に使うLocal専用Random Streamです。
	 */
	std::shared_ptr<dungeon::Random> mLocalRandom;

	/**
	 * Output callback selected for generated floor meshes.
	 * 生成Floor Mesh用に選択された出力Callbackです。
	 */
	AddStaticMeshEvent mOnAddFloor;

	/**
	 * Output callback selected for generated slope meshes.
	 * 生成Slope Mesh用に選択された出力Callbackです。
	 */
	AddStaticMeshEvent mOnAddSlope;

	/**
	 * Output callback selected for generated wall meshes.
	 * 生成Wall Mesh用に選択された出力Callbackです。
	 */
	AddStaticMeshEvent mOnAddWall;

	/**
	 * Output callback selected for generated roof meshes and partition anchors.
	 * 生成Roof MeshとPartition Anchor用に選択された出力Callbackです。
	 */
	AddRoofStaticMeshEvent mOnAddRoof;

	/**
	 * Output callback selected for generated pillar meshes.
	 * 生成Pillar Mesh用に選択された出力Callbackです。
	 */
	AddPillarStaticMeshEvent mOnAddPillar;

	/**
	 * Output callback selected for generated catwalk meshes.
	 * 生成Catwalk Mesh用に選択された出力Callbackです。
	 */
	AddStaticMeshEvent mOnAddCatwalk;

	/**
	 * Walls resolved during voxel traversal and emitted after every adjacency decision is known.
	 * Voxel走査中に解決し、全隣接判定の確定後に出力するWall情報です。
	 */
	std::vector<ReservedWallInfo> mReservedWallInfo;

	/**
	 * Generated wall and ceiling face mask keyed by voxel location for trace-free vegetation placement.
	 * Traceなしの植生配置に使用する、Voxel位置ごとの生成済み壁・天井面マスクです。
	 */
	mutable TMap<FIntVector, uint8> mGeneratedVegetationSurfaceMasks;

	/**
	 * Deferred actor spawn manager that spreads actor creation over multiple ticks.
	 * Actor生成を複数Tickに分散する遅延Actor生成マネージャです。
	 */
	FDungeonDeferredActorSpawnManager mDungeonDeferredActorSpawnManager;

	/**
	 * Validated performance settings used by the active generation.
	 * 実行中のGenerationが使用する検証済み性能設定です。
	 */
	FDungeonGenerationPerformanceSettings mActiveGenerationPerformance;

	/**
	 * Current internal phase of the active generation pipeline.
	 * 実行中の生成パイプラインの現在の内部フェーズです。
	 */
	EDungeonGenerationPhase mGenerationPhase = EDungeonGenerationPhase::Idle;

	/**
	 * Actor-local monotonically increasing identifier used to reject stale lifecycle completions.
	 * 古いライフサイクル完了を拒否するためのActor単位で単調増加する識別子です。
	 */
	uint64 mGenerationLifecycleId = 0;

	/**
	 * Lifecycle identifier associated with the pending success and completion gates.
	 * 保留中の成功・完了ゲートに関連付けられたライフサイクル識別子です。
	 */
	uint64 mGenerationCompletionLifecycleId = 0;

	/**
	 * Platform time when the current generation phase began.
	 * 現在の生成フェーズを開始したプラットフォーム時刻です。
	 */
	double mGenerationPhaseStartSeconds = 0.0;


	/**
	 * Whether core generation succeeded and is waiting for deferred gameplay Actor spawning.
	 * コア生成が成功し、ゲームプレイ用遅延Actorの生成完了を待っているかを示します。
	 */
	bool mGenerationSuccessPending = false;

	/**
	 * Whether OnGenerationSuccess was broadcast for the current generation.
	 * 現在の生成でOnGenerationSuccessを通知済みかを示します。
	 */
	bool mGenerationSucceeded = false;

	/**
	 * Whether every gameplay and visual element is ready for the current generation.
	 * 現在の生成に含まれるゲームプレイ要素と視覚要素がすべてReadyかを示します。
	 */
	bool mGenerationComplete = false;


	/**
	 * Actor tick interval saved before distributed generation switched to every-frame processing.
	 * 分散生成処理を毎フレームへ切り替える前に保存したActor Tick間隔です。
	 */
	float mDistributedGenerationSavedTickInterval = 0.f;

	/**
	 * Whether distributed generation currently overrides the actor tick interval.
	 * 分散生成処理が現在Actor Tick間隔を上書きしているかを示します。
	 */
	bool mDistributedGenerationTickIntervalOverridden = false;

	/**
	 * Whether this generation replaces its streamed start room with an already loaded level.
	 * 今回の生成でStreaming Start Roomを読込済みLevelへ置き換えるかを示します。
	 */
	bool mUsePreloadedStartRoom = false;

	/**
	 * Whether the generated grid anchor for the preloaded start room has been resolved.
	 * Preloaded Start Room用の生成Grid Anchorが確定済みかを示します。
	 */
	bool mPreloadedStartRoomGridLocationInitialized = false;


	/**
	 * CRC snapshot captured before world construction mutates transient generation state.
	 * World構築が一時的な生成状態を変更する前に取得するCRC Snapshotです。
	 */
	mutable uint32_t mCrc32AtCreation = ~0;

	/**
	 * Minimum Z value that contains a visible generated grid cell.
	 * 表示可能な生成済みグリッドセルを含む最小Z値です。
	 */
	int32 mMinVisibleGridZ = 0;

	/**
	 * Maximum Z value that contains a visible generated grid cell.
	 * 表示可能な生成済みグリッドセルを含む最大Z値です。
	 */
	int32 mMaxVisibleGridZ = 0;

	/**
	 * True when the cached visible grid height range is valid for the current generated dungeon.
	 * 現在の生成済みダンジョンに対して、表示可能なグリッド高さ範囲のキャッシュが有効な場合はtrueです。
	 */
	bool mVisibleGridHeightRangeValid = false;

	/**
	 * Represents Generated.
	 * 生成済みフラグ
	 */
	bool mGenerated = false;

	/**
	 * Whether generated content may still exist in the world.
	 * ワールドに生成物が残っている可能性があるかを示します。
	 * 生成を開始した時点でtrueになり、後始末を終えた時点でfalseになります。
	 * 生成が途中で失敗してもアクターは残るため、mGeneratedでは後始末の要否を判定できません。
	 */
	bool mDisposeRequired = false;

	/**
	 * Whether a sub-level unload was requested without waiting for it to finish.
	 * サブレベルの解放を要求したまま、完了を待っていない状態かを示します。
	 * 待ち合わせないまま次の生成を始めると、解放前のサブレベルと新しいサブレベルが同居します。
	 */
	bool mStreamLevelUnloadPending = false;

	// friend class
	friend class ADungeonMainLevelScriptActor;
};

template<typename T>
T* ADungeonGenerateBase::SpawnActorImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorImpl<T>(T::StaticClass(), folderPath, transform, ownerActor, spawnActorCollisionHandlingMethod);
}

template<typename T>
T* ADungeonGenerateBase::SpawnActorDeferredImpl(const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	return SpawnActorDeferredImpl<T>(T::StaticClass(), folderPath, transform, ownerActor, spawnActorCollisionHandlingMethod);
}

template<typename T>
T* ADungeonGenerateBase::SpawnActorImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	return Cast<T>(SpawnActorWithFolderPath(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
T* ADungeonGenerateBase::SpawnActorDeferredImpl(UClass* actorClass, const FString& folderPath, const FTransform& transform, AActor* ownerActor, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod) const
{
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = ownerActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnActorCollisionHandlingMethod;
	actorSpawnParameters.bDeferConstruction = true;
	return Cast<T>(SpawnActorWithFolderPath(actorClass, folderPath, transform, actorSpawnParameters));
}

template<typename T>
T* ADungeonGenerateBase::FindActor()
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
const T* ADungeonGenerateBase::FindActor() const
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
void ADungeonGenerateBase::EachActors(const std::function<bool(T*)>& function)
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
void ADungeonGenerateBase::EachActors(const std::function<bool(const T*)>& function) const
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
