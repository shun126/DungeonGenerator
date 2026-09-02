/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonGenerationPerformanceSettings.generated.h"

/**
 * Per-frame limits for deferred Actor spawning.
 * 遅延Actor生成に使用するフレーム単位の上限設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorSpawnPerformanceSettings
{
	GENERATED_BODY()

	/**
	 * Whether queued Actor spawn requests are distributed across game frames.
	 * キューに登録されたActor生成要求を複数のゲームフレームへ分散するかを指定します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Actor Spawn", meta = (ToolTip = "Distributes queued Actor spawn requests across game frames. Disable this to process every request synchronously after core dungeon generation."))
	bool bUseDeferredSpawn = false;

	/**
	 * Maximum Actor spawn requests processed per frame. Zero means unlimited.
	 * 1フレームで処理するActor生成要求数の上限です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Actor Spawn", meta = (ClampMin = "0", ToolTip = "Maximum Actor spawn requests processed per frame, including failed and cancelled requests. Use 0 for unlimited."))
	int32 MaxSpawnRequestsPerFrame = 0;

	/**
	 * Maximum milliseconds spent processing Actor spawn requests per frame. Zero means unlimited.
	 * 1フレームでActor生成要求の処理に使用する最大ミリ秒です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Actor Spawn", meta = (ClampMin = "0.0", ToolTip = "Maximum milliseconds spent processing Actor spawn requests per frame. Use 0 for unlimited. When both limits are set, processing stops at the first limit reached."))
	float MaxSpawnTimeMs = 16.0f;
};

/**
 * Per-frame limits for deferred vegetation placement and foliage tree building.
 * 遅延植生配置とFoliage Tree構築に使用するフレーム単位の上限設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonVegetationPerformanceSettings
{
	GENERATED_BODY()

	/**
	 * Whether vegetation candidate materialization, placement, and foliage tree building are distributed across game frames.
	 * 植生候補の実体化、配置、Foliage Tree構築を複数のゲームフレームへ分散するかを指定します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Vegetation", meta = (ToolTip = "Distributes vegetation candidate materialization, collision queries, instance placement, and foliage tree building across game frames. Inactive spatial groups continue processing; partition activity controls visibility only. Disable this to complete vegetation synchronously during dungeon generation."))
	bool bUseDeferredSpawn = true;

	/**
	 * Maximum vegetation candidates processed per frame. Zero means unlimited.
	 * 1フレームで処理する植生候補数の上限です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Vegetation", meta = (ClampMin = "0", ToolTip = "Maximum vegetation candidates materialized, traced, and placed per frame while deferred spawning is enabled. Use 0 for unlimited. Inactive spatial groups also consume this budget."))
	int32 MaxSpawnsPerFrame = 128;

	/**
	 * Maximum milliseconds spent materializing, tracing, and placing vegetation per frame. Zero means unlimited.
	 * 1フレームで植生候補の実体化、Trace、配置に使用する最大ミリ秒です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Vegetation", meta = (ClampMin = "0.0", ToolTip = "Maximum milliseconds spent materializing candidates, running collision queries, and adding vegetation instances per frame. Use 0 for unlimited. When both limits are set, processing stops at the first limit reached."))
	float MaxSpawnTimeMs = 16.0f;

	/**
	 * Maximum foliage component trees built per frame. Zero means unlimited.
	 * 1フレームで構築するFoliage Component Tree数の上限です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Vegetation", meta = (ClampMin = "0", ToolTip = "Maximum foliage component trees built per frame after deferred vegetation placement finishes. Use 0 for unlimited."))
	int32 MaxTreeBuildsPerFrame = 2;

	/**
	 * Maximum milliseconds spent building foliage trees per frame. Zero means unlimited.
	 * 1フレームでFoliage Tree構築に使用する最大ミリ秒です。0は無制限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance|Vegetation", meta = (ClampMin = "0.0", ToolTip = "Maximum milliseconds spent building foliage trees per frame. Use 0 for unlimited. When both limits are set, processing stops at the first limit reached."))
	float MaxTreeBuildTimeMs = 1.0f;
};

/**
 * Runtime scheduling settings for work distributed after core dungeon generation.
 * ダンジョンのコア生成後に分散処理する作業の実行設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGenerationPerformanceSettings
{
	GENERATED_BODY()

	/**
	 * Deferred Actor spawn settings required before gameplay can begin.
	 * ゲームプレイ開始前に必要な遅延Actor生成設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance", meta = (ToolTip = "Controls queued Actor spawning that must finish before OnGenerationSuccess."))
	FDungeonActorSpawnPerformanceSettings ActorSpawn;

	/**
	 * Deferred vegetation settings required before final visual completion.
	 * 最終的な視覚生成完了までに必要な遅延植生設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Generation Performance", meta = (ToolTip = "Controls vegetation placement and foliage tree building that must finish before OnGenerationComplete."))
	FDungeonVegetationPerformanceSettings Vegetation;
};
