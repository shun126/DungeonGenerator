/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonPartition.h"
#include <CoreMinimal.h>
#include <Containers/Array.h>
#include <Engine/LevelScriptActor.h>
#include <Math/Box.h>
#include "DungeonMainLevelScriptActor.generated.h"

class ADungeonGenerateActor;
class APlayerController;
class FSceneView;

/**
This class manages the DungeonComponentActivatorComponent validity
within a runtime-generated dungeon.
The dungeon is partitioned into a specified range of partitions,
and the components registered in the partitions component
registered in the partition.

ランタイム生成ダンジョン内のDungeonComponentActivatorComponent有効性を管理するクラスです。
ダンジョンを指定の範囲のパーティションで区切り、パーティションに登録されたコンポーネントの
アクティブ性を制御します。
*/
UCLASS()
class DUNGEONGENERATOR_API ADungeonMainLevelScriptActor : public ALevelScriptActor
{
	GENERATED_BODY()

	/**
	 * Minimum distance from horizontal player to activate partition
	 * パーティションをアクティブにする水平方向のプレイヤーからの最小距離
	 */
	static constexpr double PartitionHorizontalMinSize = 5.0 * 100.0;

	/**
	 * Maximum distance from horizontal player to activate partition
	 * パーティションをアクティブにする水平方向のプレイヤーからの最大距離
	 */
	static constexpr double PartitionHorizontalMaxSize = 100.0 * 100.0;

	/**
	 * Distance from vertical player to activate partition
	 * パーティションをアクティブにする垂直方向のプレイヤーからの最小距離
	 */
	static constexpr double PartitionVerticalMinSize = 5.0 * 100.0;

	/**
	 * Maximum distance from vertical player to activate partition
	 * パーティションをアクティブにする垂直方向のプレイヤーからの最大距離
	 */
	static constexpr double PartitionVerticalMaxSize = 100.0 * 100.0;

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

public:
	/**
	 * Find DungeonPartition by world location
	 *
	 * ワールド座標からDungeonPartitionを検索します
	 */
	UDungeonPartition* Find(const FVector& worldLocation) const noexcept;

	/**
	 * Get AABB Extents to determine if it is around the player
	 * 
	 * プレイヤー周辺と判断するためのAABBのExtentsを取得します
	 */
	const FVector& GetActiveExtents() const noexcept;

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

	// override
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float deltaSeconds) override;

private:
	const FSceneView* GetSceneView(const APlayerController* playerController) const;

	void Begin();
	void Mark(const FVector& playerLocation);
	void Mark(const FSceneView* sceneView);
	void End(const float deltaSeconds);

	/**
	 * ポイントライトおよびスポットライトの影を落とすか制御します
	 */
	void UpdateShadowCastingPointAndSpotLights();
	void ForceActivateShadowCastingPointAndSpotLights();

	void ForceActivate();
	void ForceInactivate();

	/**
	 * 線分とAABBの交差判定
	 * @param segmentStart	線分の開始位置
	 * @param segmentEnd	線分の終了位置
	 * @param aabbCenter	AABBの中心
	 * @param aabbExtent	AABBの大きさ
	 * @return trueなら交差している
	 */
	static bool TestSegmentAABB(const FVector& segmentStart, const FVector& segmentEnd, const FVector& aabbCenter, const FVector& aabbExtent);

#if WITH_EDITOR
	void DrawDebugInformation() const;
#endif

protected:
	/**
	 * レベル内のダンジョンパーティエーション
	 */
	UPROPERTY(Transient)
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "DungeonGenerator|Debug")
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
	double mBoundingSize = PartitionHorizontalMinSize;
	size_t mPartitionWidth = 0;
	size_t mPartitionDepth = 0;
	double mPartitionSize = PartitionHorizontalMinSize / 2;
	FVector mActiveExtents;
	bool mLastEnableLoadControl;
};

inline const FVector& ADungeonMainLevelScriptActor::GetActiveExtents() const noexcept
{
	return mActiveExtents;
}
