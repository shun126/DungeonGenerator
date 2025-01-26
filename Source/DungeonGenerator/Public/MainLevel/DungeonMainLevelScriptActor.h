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
	Distance from horizontal player to activate partition
	パーティションをアクティブにする水平方向のプレイヤーからの距離
	*/
	static constexpr double ActiveExtentHorizontalSize = 10.0 * 100.0;

	/**
	Distance from vertical player to activate partition
	パーティションをアクティブにする垂直方向のプレイヤーからの距離
	*/
	static constexpr double ActiveExtentVerticalSize = 5.0 * 100.0;

public:
	explicit ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer);
	virtual ~ADungeonMainLevelScriptActor() override = default;

public:
	/**
	 * Called before dungeon generation
	 * ダンジョン生成前に呼ばれます
	 * @param dungeonGenerateActor DungeonGenerateActor
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnPreDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor);

	/**
	 * Called after dungeon generation
	 * ダンジョン生成後に呼ばれます
	 * @param dungeonGenerateActor	DungeonGenerateActor
	 * @param result				trueならば生成成功
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator")
	void OnPostDungeonGeneration(ADungeonGenerateActor* dungeonGenerateActor, const bool result);

public:
	/**
	Find DungeonPartition by world location
	ワールド座標からDungeonPartitionを検索します
	*/
	UDungeonPartition* Find(const FVector& worldLocation) const noexcept;

	/**
	Get AABB Extents to determine if it is around the player
	プレイヤー周辺と判断するためのAABBのExtentsを取得します
	*/
	const FVector& GetActiveExtents() const noexcept;

	/**
	 * Get the partition size
	 * パーティエーションサイズを取得します
	 * @return パーティエーションサイズ
	 */
	float GetPartitionSize() const noexcept;

	// override
	virtual void PreInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void Tick(float deltaSeconds) override;

public:
#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
	/**
	Is load control effective?
	負荷コントロールが有効か取得します
	*/
	bool IsEnableLoadControl() const noexcept;
#endif

private:
	void Begin();
	void Mark(const FBox& activeBounds);
	void End(const float deltaSeconds);

	void ForceActivate();
	void ForceInactivate();

#if WITH_EDITOR
	void DrawDebugInformation() const;
#endif

protected:
	/**
	レベル内のダンジョンパーティエーション
	*/
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDungeonPartition>> DungeonPartitions;

	/**
	Distance to divide the world
	ワールドを分割する距離
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = "1000"))
	float PartitionSize = 1000.0f;

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	/**
	Load control effectiveness
	負荷コントロールの有効性
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "DungeonGenerator|Debug")
	bool bEnableLoadControl = true;

	/**
	 * Displays debugging information
	 * デバッグ情報を表示します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient, Category = "DungeonGenerator|Debug")
	bool ShowDebugInformation = false;
#endif

private:
	FBox mBounding;
	size_t mPartitionWidth = 0;
	size_t mPartitionDepth = 0;
	FVector mActiveExtents;

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	bool mLastEnableLoadControl;
#endif
};

inline const FVector& ADungeonMainLevelScriptActor::GetActiveExtents() const noexcept
{
	return mActiveExtents;
}

inline float ADungeonMainLevelScriptActor::GetPartitionSize() const noexcept
{
	return PartitionSize;
}
