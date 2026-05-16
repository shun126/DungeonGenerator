/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
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
	 * コンストラクタ
	 */
	explicit ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer);

	/**
	 * デストラクタ
	 */
	virtual ~ADungeonMainLevelScriptActor() override = default;

public:
	/**
	 * Called before dungeon generation
	 *
	 * ダンジョン生成前に呼ばれます
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnPreDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor);

	/**
	 * Called after dungeon generation
	 *
	 * ダンジョン生成後に呼ばれます
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnPostDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor, const bool result);

	/**
	 * Find DungeonPartition by world location
	 *
	 * ワールド座標からDungeonPartitionを検索します
	 */
	UDungeonPartition* Find(const FVector& worldLocation) const noexcept;

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
	void EnableLoadControl(const bool enable) noexcept;

	/**
	 * Rebuilds the sparse partition graph and refreshes activator registrations.
	 * Call this after dungeon generation changes the traversable layout.
	 *
	 * sparse partition graph を再構築し、activator の登録を更新します。
	 * ダンジョン生成で通行可能レイアウトが変化した後に呼び出してください。
	 */
	void RebuildSparsePartitionGraphAndRefresh();

	// override
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float deltaSeconds) override;

private:
	struct FPartitionVisibilitySample
	{
		const ADungeonGenerateActor* DungeonGenerateActor = nullptr;
		FIntVector GridLocation = FIntVector::ZeroValue;
		FVector WorldLocation = FVector::ZeroVector;
	};

	/*
	 * Identifies why the partition build pipeline is running.
	 * パーティション構築パイプラインの実行理由を表します。
	 */
	enum class EPartitionBuildReason : uint8
	{
		InitialLoad,
		RuntimeRebuild,
	};

	/*
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

	/*
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

	/*
	 * Executes the shared partition build pipeline.
	 * 共通のパーティション構築パイプラインを実行します。
	 */
	bool ExecutePartitionBuild(const FPartitionBuildOptions& options);

	/*
	 * Validates the level and prepares the initial build context.
	 * レベルの妥当性を確認して初期構築コンテキストを準備します。
	 */
	bool TryPreparePartitionBuildContext(const FPartitionBuildOptions& options, FPartitionBuildContext& context) const;

	/*
	 * Collects dungeon actors used by the current build policy.
	 * 現在の構築ポリシーで使用するダンジョン actor を収集します。
	 */
	static void CollectPartitionBuildActors(const FPartitionBuildOptions& options, FPartitionBuildContext& context);

	/*
	 * Accumulates the metrics contributed by one dungeon actor.
	 * 1 つのダンジョン actor が持つ集計値を加算します。
	 */
	static void AccumulatePartitionBuildActor(const ADungeonGenerateActor* dungeonGenerateActor, FPartitionBuildContext& context);

	/*
	 * Validates actor collection results before metric computation.
	 * メトリクス計算前に actor 収集結果を検証します。
	 */
	static bool ValidateCollectedPartitionBuildActors(const FPartitionBuildOptions& options, const FPartitionBuildContext& context);

	/*
	 * Computes derived partition metrics from the collected actor data.
	 * 収集済み actor データから派生するパーティション指標を計算します。
	 */
	void ComputePartitionBuildMetrics(FPartitionBuildContext& context) const;

	/*
	 * Validates the computed build context before committing it.
	 * member へ確定反映する前に計算済みコンテキストを検証します。
	 */
	static bool ValidatePartitionBuildContext(const FPartitionBuildContext& context);

	/*
	 * Resets the current partition build state before rebuilding it.
	 * 再構築前に現在のパーティション構築状態をリセットします。
	 */
	void ResetPartitionBuildState();

	/*
	 * Commits the computed build context into runtime members.
	 * 計算済みコンテキストを runtime member へ確定反映します。
	 */
	void CommitPartitionBuildContext(const FPartitionBuildContext& context);

	/*
	 * Rebuilds graph and visibility data from the committed context.
	 * 確定反映済みコンテキストから graph と visibility を再構築します。
	 */
	void BuildPartitionRuntimeData(const FPartitionBuildContext& context);

	/*
	 * Applies updated culling distances back to dungeon actors.
	 * 更新後のカリング距離をダンジョン actor へ反映します。
	 */
	void ApplyDungeonActorCullDistances(const FPartitionBuildContext& context) const;

	/*
	 * Applies the post-build runtime synchronization for the selected policy.
	 * 選択されたポリシーに応じた構築後の runtime 同期を適用します。
	 */
	void FinalizePartitionBuild(const FPartitionBuildOptions& options, bool buildSucceeded);
	int32 FindPartitionIndex(const FVector& worldLocation) const noexcept;
	FIntVector ToPartitionCell(const FVector& worldLocation) const noexcept;
	FBox MakePartitionBounds(const FIntVector& partitionCell) const noexcept;
	int32 FindNearestPartitionIndex(const FIntVector& partitionCell, const FVector& worldLocation) const noexcept;
	int32 FindOrAddPartition(const FIntVector& partitionCell);
	void LinkPartitions(const int32 partitionIndex0, const int32 partitionIndex1);
	void BuildSparsePartitionGraph(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors);
	void ResetPrecomputedPartitionVisibility();
	void BuildPartitionVisibilitySamples(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors);
	void BuildPartitionConnectedComponents();
	void BuildPrecomputedPartitionVisibility();
	bool HasPrecomputedPartitionVisibility() const noexcept;
	bool IsPartitionLoadControlAvailable() const noexcept;
	void MarkPrecomputedPartitionVisibility(int32 sourcePartitionIndex) const;
	bool IsPrecomputedPartitionVisible(int32 sourcePartitionIndex, int32 targetPartitionIndex) const noexcept;
	bool IsPartitionPairPotentiallyVisible(int32 sourcePartitionIndex, int32 targetPartitionIndex) const;
	static bool TracePartitionVisibility(const FPartitionVisibilitySample& sourceSample, const FPartitionVisibilitySample& targetSample);
	void RefreshActivatorComponentRegistrations();
	void ApplyCurrentPartitionActivationState();
	void ResetPartitionTransitionQueue();
	void EnqueuePartitionTransition(int32 partitionIndex);
	void ProcessPartitionTransitionQueue();
	FInt32Interval ComputeTerrainCullingDistanceRange() const noexcept;

	void Begin() const;
	void Mark(const FVector& playerLocation) const;
	void End(const float deltaSeconds);

	/**
	 * ポイントライトおよびスポットライトの影を落とすか制御します
	 */
	void UpdateShadowCastingPointAndSpotLights();
	void ForceActivateShadowCastingPointAndSpotLights();

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
#endif

protected:
	/**
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "1"))
	float ActivationRangeScale = 1.0f;

	/**
	 * Override grid counts used to build sparse partitions.
	 * A component value of 0 uses auto sizing for that axis.
	 *
	 * sparse partition の構築に使うグリッド個数の上書き値です。
	 * 各軸の値に 0 を指定すると、その軸は自動で決定されます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	FIntVector PartitionGridCountOverride = FIntVector::ZeroValue;

	/**
	 * Uses precomputed potential visibility sets built after dungeon generation
	 * to control runtime partition activation.
	 *
	 * ダンジョン生成後に構築した PVS を使用して、
	 * 実行時の partition アクティブ制御を行います。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool bUsePrecomputedPartitionVisibility = true;

	/**
	 * Expands precomputed visible partitions by neighbor graph hops.
	 * Set to 0 to disable dilation.
	 *
	 * 事前計算した可視パーティエーションを隣接グラフの hop 数だけ拡張します。
	 * 0 を指定すると拡張を無効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 PrecomputedVisibilityDilationHopCount = 1;

	/**
	 * Maximum number of partition activations processed in a single frame.
	 * Set to 0 to remove the per-frame activation limit.
	 *
	 * 1 フレーム内で処理する partition アクティブ化の最大数です。
	 * 0 を指定するとフレームごとのアクティブ化上限を無効化します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 MaxPartitionActivationsPerFrame = 8;

	/**
	 * Maximum number of partition inactivations processed in a single frame.
	 * Set to 0 to remove the per-frame inactivation limit.
	 *
	 * 1 フレーム内で処理する partition 非アクティブ化の最大数です。
	 * 0 を指定するとフレームごとの非アクティブ化上限を無効化します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 MaxPartitionInactivationsPerFrame = 16;

	/**
	 * Maximum number of point lights or spotlights casting shadows
	 * Unlimited if 0
	 *
	 * ポイントライトまたはスポットライトの影を落とす最大数
	 * 0ならば無制限
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	uint8 MaxShadowCastingPointAndSpotLights = 12;

	/**
	 * Load control effectiveness
	 * 負荷コントロールの有効性
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	bool bEnableLoadControl = true;

#if WITH_EDITORONLY_DATA
	/**
	 * Displays debugging information
	 * デバッグ情報を表示します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "DungeonGenerator|Debug")
	bool ShowDebugInformation = false;
#endif

private:
	FBox mBounding;
	FVector mPartitionWorldSize = FVector::OneVector;
	FIntVector mPartitionGridCount = FIntVector(AutoPartitionHorizontalMinGridCount, AutoPartitionHorizontalMinGridCount, AutoPartitionVerticalMinGridCount);
	float mTheoreticalMaxVisibilityDistance = 0.f;
	bool mLastEnableLoadControl;
	TMap<FIntVector, int32> mPartitionIndexByCell;
	TArray<TArray<FPartitionVisibilitySample>> mPartitionVisibilitySamples;
	TArray<TArray<uint64>> mPartitionPotentialVisibilityMasks;
	TArray<int32> mPartitionConnectedComponents;
	TArray<int32> mPendingPartitionTransitions;
	TArray<uint8> mDesiredPartitionActivation;
	TArray<uint8> mQueuedPartitionTransitions;
	int32 mPendingPartitionTransitionReadIndex = 0;
};
