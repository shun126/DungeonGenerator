/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonPartition.h"
#include <CoreMinimal.h>
#include <Containers/Array.h>
#include <Containers/Map.h>
#include <Engine/LevelScriptActor.h>
#include <Math/Box.h>
#include "DungeonMainLevelScriptActor.generated.h"

class ADungeonGenerateActor;
class APlayerController;
class UCanvas;
class ULevel;

/**
 * This class manages the DungeonComponentActivatorComponent validity
 * within a runtime-generated dungeon.
 * The dungeon is partitioned into a specified range of partitions,
 * and the components registered in the partitions component
 * registered in the partition.
 *
 * ランタイム生成ダンジョン内のDungeonComponentActivatorComponent有効性を管理するクラスです。
 * ダンジョンを指定の範囲のパーティションで区切り、パーティションに登録されたコンポーネントの
 * アクティブ性を制御します。
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonMainLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

	/**
	 * Minimum distance from horizontal player to activate partition
	 * パーティションをアクティブにする水平方向のプレイヤーからの最小距離
	 */
	static constexpr int32 AutoPartitionHorizontalMinGridCount = 2;

	/**
	 * Maximum distance from horizontal player to activate partition
	 * パーティションをアクティブにする水平方向のプレイヤーからの最大距離
	 */
	static constexpr int32 AutoPartitionHorizontalMaxGridCount = 8;

	/**
	 * Distance from vertical player to activate partition
	 * パーティションをアクティブにする垂直方向のプレイヤーからの最小距離
	 */
	static constexpr int32 AutoPartitionVerticalMinGridCount = 1;

	/**
	 * Maximum distance from vertical player to activate partition
	 * パーティションをアクティブにする垂直方向のプレイヤーからの最大距離
	 */
	static constexpr int32 AutoPartitionVerticalMaxGridCount = 2;

public:
	/**
	 * Represents ADungeonMainLevelScriptActor.
	 * コンストラクタ
	 */
	explicit ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer);

	/**
	 * Destroys the ~ADungeonMainLevelScriptActor instance.
	 * デストラクタ
	 */
	virtual ~ADungeonMainLevelScriptActor() override = default;

	/**
	 * Called before dungeon generation
	 *
	 * ダンジョン生成前に呼ばれます
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Called before dungeon generation"))
	void OnPreDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor);

	/**
	 * Called after dungeon generation
	 *
	 * ダンジョン生成後に呼ばれます
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (ToolTip = "Called after dungeon generation"))
	void OnPostDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor, const bool result);

	/**
	 * Find DungeonPartition by world location
	 *
	 * ワールド座標からDungeonPartitionを検索します
	 */
	UDungeonPartition* Find(const FVector& worldLocation) const noexcept;

	/**
	 * Finds PartitionIndex.
	 * ワールド座標から実行時パーティションのインデックスを検索します。
	 */
	int32 FindPartitionIndex(const FVector& worldLocation) const noexcept;

	/**
	 * Returns whether PartitionActive.
	 * 指定された実行時パーティションが現在アクティブかどうかを返します。
	 */
	bool IsPartitionActive(int32 partitionIndex) const noexcept;

	/**
	 * Is load control effective?
	 *
	 * 負荷コントロールが有効か取得します
	 */
	bool IsEnableLoadControl() const noexcept;

	/**
	 * Enables or disables load control
	 *
	 * 負荷コントロールを有効または無効にします
	 */
	void EnableLoadControl(bool enable) noexcept;

	/**
	 * Rebuilds the sparse partition graph and refreshes activator registrations.
	 * Call this after dungeon generation changes the traversable layout.
	 *
	 * ダンジョン生成で通行可能レイアウトが変化した後に呼び出してください。
	 */
	void RebuildSparsePartitionGraphAndRefresh();

	/**
	 * Coalesces a refresh of partition links after deferred vegetation bounds change.
	 * 遅延植生の境界変更後にパーティションリンクの更新要求を集約します。
	 */
	void RequestSpatialMeshGroupPartitionLinkRefresh();

	// override
	virtual void PreInitializeComponents() override;
	virtual void Tick(float deltaSeconds) override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	/**
	 * Describes why a partition visibility sample was added.
	 * パーティション可視性サンプルが追加された理由を表します。
	 */
	enum class EPartitionVisibilitySampleType : uint8
	{
		Center,
		Edge,
		Roof,
		SlopeLower,
		SlopeUpper
	};

	struct FPartitionVisibilitySample
	{
		const ADungeonGenerateActor* DungeonGenerateActor = nullptr;
		FIntVector GridLocation = FIntVector::ZeroValue;
		FVector WorldLocation = FVector::ZeroVector;
		EPartitionVisibilitySampleType SampleType = EPartitionVisibilitySampleType::Center;
		/**
		 * Identifies the cached trace context used by this sample.
		 * このサンプルが使用するキャッシュ済みTraceコンテキストを識別します。
		 */
		int32 TraceContextIndex = INDEX_NONE;
	};

	/**
	 * Identifies why the partition build pipeline is running.
	 * パーティション構築パイプラインの実行理由を表します。
	 */
	enum class EPartitionBuildReason : uint8
	{
		InitialLoad,
		RuntimeRebuild,
	};

	/**
	 * Stores the policy differences for each build trigger.
	 * 構築トリガーごとの差分ポリシーを保持します。
	 */
	struct FPartitionBuildOptions
	{
		EPartitionBuildReason Reason = EPartitionBuildReason::InitialLoad;

		bool RequiresGeneratedDungeon() const noexcept
		{
			return Reason == EPartitionBuildReason::RuntimeRebuild;
		}

		bool ShouldSyncRuntimeState() const noexcept
		{
			return Reason == EPartitionBuildReason::RuntimeRebuild;
		}

		bool ShouldUpdateTickState() const noexcept
		{
			return Reason == EPartitionBuildReason::RuntimeRebuild;
		}
	};

	/**
	 * Collects temporary values for partition building before they are committed.
	 * member に確定反映する前のパーティション構築用一時値を集約します。
	 */
	struct FPartitionBuildContext
	{
		ULevel* Level = nullptr;
		TArray<ADungeonGenerateActor*> DungeonGenerateActors;
		FBox Bounding = FBox(EForceInit::ForceInit);
		FVector2D DungeonMaxLongestStraightPath = FVector2D::ZeroVector;
		FVector DungeonRoomMaxSize = FVector::ZeroVector;
		float MaxGridSize = 1.f;
		int32 MinimumRoomWidthInGrid = TNumericLimits<int32>::Max();
		int32 MinimumRoomDepthInGrid = TNumericLimits<int32>::Max();
		int32 MinimumRoomHeightInGrid = TNumericLimits<int32>::Max();
		int32 CommonHorizontalGridSize = 0;
		int32 CommonVerticalGridSize = 0;
		float TheoreticalMaxVisibilityDistance = 0.f;
		FIntVector PartitionGridCount = FIntVector::ZeroValue;
		FVector PartitionWorldSize = FVector::OneVector;
	};

	/**
	 * Identifies one actor-owned instanced mesh spatial group.
	 * 1つのActorが所有するInstanced Mesh空間グループを識別します。
	 */
	struct FInstancedMeshGroupHandle
	{
		/**
		 * Actor that owns the referenced spatial group.
		 * 参照する空間グループを所有するActorです。
		 */
		TWeakObjectPtr<ADungeonGenerateActor> DungeonGenerateActor;

		/**
		 * Stable XYZ coordinate of the referenced spatial group.
		 * 参照する空間グループの安定したXYZ座標です。
		 */
		FIntVector GroupCoordinate = FIntVector::ZeroValue;

		bool operator==(const FInstancedMeshGroupHandle& other) const noexcept
		{
			return DungeonGenerateActor == other.DungeonGenerateActor &&
				GroupCoordinate == other.GroupCoordinate;
		}

		friend uint32 GetTypeHash(const FInstancedMeshGroupHandle& handle)
		{
			return HashCombine(GetTypeHash(handle.DungeonGenerateActor), GetTypeHash(handle.GroupCoordinate));
		}
	};

	/**
	 * Executes the shared partition build pipeline.
	 * 共通のパーティション構築パイプラインを実行します。
	 */
	bool ExecutePartitionBuild(const FPartitionBuildOptions& options);

	/**
	 * Validates the level and prepares the initial build context.
	 * レベルの妥当性を確認して初期構築コンテキストを準備します。
	 */
	bool TryPreparePartitionBuildContext(const FPartitionBuildOptions& options, FPartitionBuildContext& context) const;

	/**
	 * Collects dungeon actors used by the current build policy.
	 * 現在の構築ポリシーで使用するダンジョン actor を収集します。
	 */
	static void CollectPartitionBuildActors(const FPartitionBuildOptions& options, FPartitionBuildContext& context);

	/**
	 * Accumulates the metrics contributed by one dungeon actor.
	 * 1 つのダンジョン actor が持つ集計値を加算します。
	 */
	static void AccumulatePartitionBuildActor(const ADungeonGenerateActor* dungeonGenerateActor, FPartitionBuildContext& context);

	/**
	 * Validates actor collection results before metric computation.
	 * メトリクス計算前に actor 収集結果を検証します。
	 */
	static bool ValidateCollectedPartitionBuildActors(const FPartitionBuildOptions& options, const FPartitionBuildContext& context);

	/**
	 * Computes derived partition metrics from the collected actor data.
	 * 収集済み actor データから派生するパーティション指標を計算します。
	 */
	void ComputePartitionBuildMetrics(FPartitionBuildContext& context) const;

	/**
	 * Validates the computed build context before committing it.
	 * member へ確定反映する前に計算済みコンテキストを検証します。
	 */
	static bool ValidatePartitionBuildContext(const FPartitionBuildContext& context);

	/**
	 * Resets PartitionBuildState.
	 * 再構築前に現在のパーティション構築状態をリセットします。
	 */
	void ResetPartitionBuildState();

	/**
	 * Commits the computed build context into runtime members.
	 * 計算済みコンテキストを runtime member へ確定反映します。
	 */
	void CommitPartitionBuildContext(const FPartitionBuildContext& context);

	/**
	 * Rebuilds graph and visibility data from the committed context.
	 * 確定反映済みコンテキストから graph と visibility を再構築します。
	 */
	void BuildPartitionRuntimeData(const FPartitionBuildContext& context);
	void BuildInstancedMeshGroupPartitionLinks(const FPartitionBuildContext& context);
	void ResetInstancedMeshGroupPartitionLinks();
	void SetInstancedMeshGroupVisible(const FInstancedMeshGroupHandle& handle, bool visible) const;
	void UpdateInstancedMeshGroupPartitionActivation(int32 partitionIndex, bool active);
	void SetPartitionActivationState(int32 partitionIndex, bool active, bool resetPartitionInactivateRemainTimer);

	/**
	 * Applies updated culling distances back to dungeon actors.
	 * 更新後のカリング距離をダンジョン actor へ反映します。
	 */
	void ApplyDungeonActorCullDistances(const FPartitionBuildContext& context) const;

	/**
	 * Applies the post-build runtime synchronization for the selected policy.
	 * 選択されたポリシーに応じた構築後の runtime 同期を適用します。
	 */
	void FinalizePartitionBuild(const FPartitionBuildOptions& options, bool buildSucceeded);
	FIntVector ToPartitionCell(const FVector& worldLocation) const noexcept;
	FBox MakePartitionBounds(const FIntVector& partitionCell) const noexcept;
	int32 FindNearestPartitionIndex(const FIntVector& partitionCell, const FVector& worldLocation) const noexcept;
	int32 FindOrAddPartition(const FIntVector& partitionCell);
	void LinkPartitions(const int32 partitionIndex0, const int32 partitionIndex1);
	void BuildSparsePartitionGraph(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors);
	void ResetPrecomputedPartitionVisibility();
	void BuildPartitionVisibilitySamples(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors);
	static TArray<FVector> MakeSpatialVisibilitySampleAnchors(const FBox& bounds);
	static TArray<FVector> MakeRoofVisibilitySampleAnchors(const FBox& bounds);
	static void AppendClosestUniqueVisibilitySamples(TArray<FPartitionVisibilitySample>& destination, const TArray<FPartitionVisibilitySample>& candidates, const TArray<FVector>& anchors, EPartitionVisibilitySampleType sampleType);
	static void AppendRemainingUniqueVisibilitySamples(TArray<FPartitionVisibilitySample>& destination, const TArray<FPartitionVisibilitySample>& candidates, EPartitionVisibilitySampleType sampleType);
	static bool IsVisibilitySampleUsableAsSource(EPartitionVisibilitySampleType sampleType) noexcept;
	struct FPVSBuildStatistics;
	struct FPVSTraceContext;
	struct FPVSPartitionSamples;
	void BuildPartitionConnectedComponents();
	void BuildPrecomputedPartitionVisibility(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors);
	bool HasPrecomputedPartitionVisibility() const noexcept;
	bool IsPartitionLoadControlAvailable() const noexcept;
	void MarkPrecomputedPartitionVisibility(int32 sourcePartitionIndex) const;
	bool IsPrecomputedPartitionVisible(int32 sourcePartitionIndex, int32 targetPartitionIndex) const noexcept;
	static void BuildUniqueVisibilitySampleIndices(const TArray<FPartitionVisibilitySample>& samples, bool sourceSamplesOnly, TArray<int32>& outputIndices);
	bool IsPartitionPairPotentiallyVisible(int32 sourcePartitionIndex, int32 targetPartitionIndex, const TArray<FPVSTraceContext>& traceContexts, const TArray<FPVSPartitionSamples>& partitionSamples, bool validateOptimization, FPVSBuildStatistics& statistics) const;
	static bool TracePartitionVisibility(const FPartitionVisibilitySample& sourceSample, const FPartitionVisibilitySample& targetSample, const FPVSTraceContext& traceContext, FPVSBuildStatistics* statistics);
#if WITH_EDITOR
	static bool TracePartitionVisibilityReference(const FPartitionVisibilitySample& sourceSample, const FPartitionVisibilitySample& targetSample, FPVSBuildStatistics& statistics);
#endif
	void RefreshActivatorComponentRegistrations();
	void ApplyCurrentPartitionActivationState();
	void ResetPartitionTransitionQueue();
	void EnqueuePartitionTransition(int32 partitionIndex);
	void ProcessPartitionTransitionQueue();
	FInt32Interval ComputeTerrainCullingDistanceRange() const noexcept;
	bool FindGridIdentifier(const FVector& worldLocation, uint16& identifier) const noexcept;

	void Begin() const;
	void Mark(const FVector& playerLocation) const;
	void End(const float deltaSeconds);

	void UpdatePointAndSpotLightStates();
	void RestorePointAndSpotLightStates();

	void ForceActivate();
	void ForceInactivate();

#if WITH_EDITOR
	/**
	 * 線分とAABBの交差判定
	 * @param segmentStart	線分の開始位置
	 * @param segmentEnd	線分の終了位置
	 * @param aabbCenter	AABBの中心
	 * @param aabbExtent	AABBの大きさ
	 * @return trueなら交差している
	 */
	static bool TestSegmentAABB(const FVector& segmentStart, const FVector& segmentEnd, const FVector& aabbCenter, const FVector& aabbExtent);
	void DrawDebugInformation() const;
	void DrawDebugRuntimePartitionState() const;
	void DrawDebugPartitionConstruction() const;
	void DrawDebugPVSResult() const;
	void DrawDebugPVSBuildSamples() const;
	void RegisterDebugLegend();
	void UnregisterDebugLegend();
	void DrawDebugLegend(UCanvas* canvas, APlayerController* playerController) const;
#endif

protected:
	/**
	 * Represents DungeonPartitions.
	 * レベル内のダンジョンパーティエーション
	 */
	UPROPERTY(Transient, meta = (ToolTip = "Runtime partitions currently built for this level."))
	TArray<TObjectPtr<UDungeonPartition>> DungeonPartitions;

	/**
	 * Scaling factor of the range used for the activation decision.
	 * If 1.0, the set distance is used as is.
	 * Larger values increase the effective range, smaller values decrease it.
	 *
	 * アクティベーション判定に用いる範囲のスケーリング係数。
	 * 1.0 の場合は設定された距離をそのまま使用します。
	 * 値を大きくすると有効範囲が広がり、小さくすると狭まります。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|PVS", meta = (ClampMin = "1", ToolTip = "Scaling factor of the range used for the activation decision. If 1.0, the set distance is used as is. Larger values increase the effective range, smaller values decrease it."))
	float ActivationRangeScale = 1.0f;

	/**
	 * Override grid counts used to build sparse partitions.
	 * A component value of 0 uses auto sizing for that axis.
	 *
	 * sparse partition の構築に使うグリッド個数の上書き値です。
	 * 各軸の値に 0 を指定すると、その軸は自動で決定されます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|PVS", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Override grid counts used to build sparse partitions. A component value of 0 uses auto sizing for that axis."))
	FIntVector PartitionGridCountOverride = FIntVector::ZeroValue;

	/**
	 * Expands precomputed visible partitions by neighbor graph hops.
	 * Set to 0 to disable dilation.
	 *
	 * 事前計算した可視パーティエーションを隣接グラフの hop 数だけ拡張します。
	 * 0 を指定すると拡張を無効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|PVS", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Expands precomputed visible partitions by neighbor graph hops. Set to 0 to disable dilation.", ClampMin = "0"))
	int32 PrecomputedVisibilityDilationHopCount = 1;

	/**
	 * Maximum number of partition activations processed in a single frame.
	 * Set to 0 to remove the per-frame activation limit.
	 *
	 * 1 フレーム内で処理する partition アクティブ化の最大数です。
	 * 0 を指定するとフレームごとのアクティブ化上限を無効化します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|PVS", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Maximum number of partition activations processed in a single frame. Set to 0 to remove the per-frame activation limit.", ClampMin = "0"))
	int32 MaxPartitionActivationsPerFrame = 8;

	/**
	 * Maximum number of partition inactivations processed in a single frame.
	 * Set to 0 to remove the per-frame inactivation limit.
	 *
	 * 1 フレーム内で処理する partition 非アクティブ化の最大数です。
	 * 0 を指定するとフレームごとの非アクティブ化上限を無効化します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|PVS", meta = (ClampMin = "0", UIMin = "0", ToolTip = "Maximum number of partition inactivations processed in a single frame. Set to 0 to remove the per-frame inactivation limit.", ClampMin = "0"))
	int32 MaxPartitionInactivationsPerFrame = 8;

	/**
	 * Facing angle at which a managed point or spot light turns on. Angles are measured from the owning actor's local positive Y axis toward the camera.
	 * 管理対象のポイントライトまたはスポットライトをONにする正面角度です。所有ActorのローカルY+軸からカメラ方向への角度で測定します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Light", meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "180", Units = "deg", ToolTip = "Turns a managed point or spot light on when the camera enters this angle from the owning actor's local positive Y axis. The value is normalized with the turn-off angle."))
	float PointAndSpotLightTurnOnAngle = 130.f;

	/**
	 * Facing angle at which a managed point or spot light turns off, measured from the owning actor's local positive Y axis. The gap from the turn-on angle prevents rapid toggling.
	 * 所有ActorのローカルY+軸を基準に、管理対象のポイントライトまたはスポットライトをOFFにする正面角度です。ON角度との差によって頻繁な切り替えを防ぎます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Light", meta = (ClampMin = "0", ClampMax = "180", UIMin = "0", UIMax = "180", Units = "deg", ToolTip = "Turns a managed point or spot light off when the camera leaves this angle from the owning actor's local positive Y axis. The value is normalized with the turn-on angle."))
	float PointAndSpotLightTurnOffAngle = 140.f;

	/**
	 * Fade-in time used when managed Dungeon point or spot lights become visible.
	 * 管理対象の Dungeon Point Light または Spot Light が表示される時に使用するフェードイン時間です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Light", meta = (ClampMin = "0", UIMin = "0", Units = "s", ToolTip = "Fade-in time for managed Dungeon point and spot lights. Set to 0 to switch on immediately. Standard Unreal light components still switch immediately."))
	float PointAndSpotLightFadeInTime = 1.0f;

	/**
	 * Fade-out time used when managed Dungeon point or spot lights become hidden.
	 * 管理対象の Dungeon Point Light または Spot Light が非表示になる時に使用するフェードアウト時間です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Light", meta = (ClampMin = "0", UIMin = "0", Units = "s", ToolTip = "Fade-out time for managed Dungeon point and spot lights. Set to 0 to switch off immediately. Standard Unreal light components still switch immediately."))
	float PointAndSpotLightFadeOutTime = 0.5f;

	/**
	 * Maximum number of visible point lights or spotlights casting shadows. Overflow lights fade out to prevent shadow-free light leaks.
	 * Unlimited if 0.
	 *
	 * 表示する影付きPoint LightまたはSpot Lightの最大数です。上限を超えたライトは、影なしの光漏れを防ぐためフェードアウトします。
	 * 0ならば無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Light", meta = (ClampMin = "3", UIMin = "3", ToolTip = "Maximum number of visible shadow-casting point and spot lights. Overflow lights fade out instead of remaining visible without shadows. Use 0 for unlimited."))
	uint8 MaxShadowCastingPointAndSpotLights = 12;

	/**
	 * Load control effectiveness
	 * 負荷コントロールの有効性
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Load control effectiveness"))
	bool bEnableLoadControl = true;

#if WITH_EDITORONLY_DATA
	/**
	 * Displays partition shapes and runtime activation states.
	 * パーティション形状とランタイムのアクティブ状態を表示します。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows partition shapes, player partitions, and actual versus requested activation states. Intended as the lightweight runtime view."))
	bool ShowRuntimePartitionState = false;

	/**
	 * Displays the bounds and cell structure used to construct partitions.
	 * パーティションの構築に使用した境界とセル構造を表示します。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows the dungeon bounds and partition cell structure. Partition outlines are not drawn twice when the runtime state view is also enabled."))
	bool ShowPartitionConstruction = false;

	/**
	 * Displays the partitions included in the current players' PVS results.
	 * 現在のプレイヤーのPVS結果に含まれるパーティションを表示します。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows the union of partitions included in the current players' precomputed visibility results."))
	bool ShowPVSResult = false;

	/**
	 * Displays the visibility samples used to build the PVS.
	 * PVSの構築に使用した可視性サンプルを表示します。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows Center, Edge, Roof, and Slope visibility samples used to build the PVS. This detailed view can be expensive on large dungeons."))
	bool ShowPVSBuildSamples = false;

#endif

private:
	FBox mBounding;
	FVector mPartitionWorldSize = FVector::OneVector;
	FIntVector mPartitionGridCount = FIntVector(AutoPartitionHorizontalMinGridCount, AutoPartitionHorizontalMinGridCount, AutoPartitionVerticalMinGridCount);
	float mTheoreticalMaxVisibilityDistance = 0.f;
	bool mLastEnableLoadControl;
	TMap<FIntVector, int32> mPartitionIndexByCell;

	/**
	 * Dungeon actors cached by the latest successful partition build for grid-identifier lookup.
	 * 最新の成功したパーティション構築でキャッシュされたグリッドIdentifier検索用のダンジョンActorです。
	 */
	TArray<TWeakObjectPtr<ADungeonGenerateActor>> mDungeonGenerateActors;
	TArray<TArray<FPartitionVisibilitySample>> mPartitionVisibilitySamples;
	TArray<TArray<uint64>> mPartitionPotentialVisibilityMasks;
	TArray<int32> mPartitionConnectedComponents;
	TArray<int32> mPendingPartitionTransitions;
	TArray<uint8> mDesiredPartitionActivation;
	TArray<uint8> mQueuedPartitionTransitions;
	int32 mPendingPartitionTransitionReadIndex = 0;
	TArray<TArray<FInstancedMeshGroupHandle>> mInstancedMeshGroupsByPartition;
	TMap<FInstancedMeshGroupHandle, int32> mActivePartitionCountByInstancedMeshGroup;

	/**
	 * Whether deferred vegetation requested one coalesced spatial-group link refresh.
	 * 遅延植生から集約された空間グループリンク更新が要求されているかを示します。
	 */
	bool mSpatialMeshGroupPartitionLinksDirty = false;

#if WITH_EDITOR
	/**
	 * Delegate used to draw the debug legend on the game canvas.
	 * ゲームキャンバスへデバッグ凡例を描画するデリゲートです。
	 */
	FDelegateHandle mDebugLegendDelegateHandle;
#endif

	friend class UDungeonComponentActivatorComponent;

};
