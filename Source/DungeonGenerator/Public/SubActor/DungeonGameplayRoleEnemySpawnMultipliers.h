/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonGameplayRoleEnemySpawnMultipliers.generated.h"

/*
 * Enemy spawn count multipliers for each gameplay room role.
 * ゲームプレイ役割ごとの敵スポーン数倍率です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGameplayRoleEnemySpawnMultipliers
{
	GENERATED_BODY()

	/*
	 * Multiplier used when no special gameplay role is assigned.
	 * 特別なゲームプレイ役割がない部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for rooms with no special gameplay role. 0 disables enemy spawning for the role."))
	float None_ = 0.5f;

	/*
	 * Multiplier used for combat rooms.
	 * 戦闘部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Combat rooms. 0 disables enemy spawning for the role."))
	float Combat_ = 1.0f;

	/*
	 * Multiplier used for treasure rooms.
	 * 宝物部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Treasure rooms. 0 disables enemy spawning for the role."))
	float Treasure_ = 0.8f;

	/*
	 * Multiplier used for puzzle rooms.
	 * パズル部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Puzzle rooms. 0 disables enemy spawning for the role."))
	float Puzzle_ = 0.5f;

	/*
	 * Multiplier used for rest rooms.
	 * 休憩部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Rest rooms. 0 disables enemy spawning for the role."))
	float Rest_ = 0.0f;

	/*
	 * Multiplier used for boss rooms.
	 * ボス部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Boss rooms. 0 disables enemy spawning for the role."))
	float Boss_ = 2.0f;

	/*
	 * Multiplier used for secret rooms.
	 * 隠し部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Secret rooms. 0 disables enemy spawning for the role."))
	float Secret_ = 0.7f;
};
