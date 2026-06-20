/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonStructuralRoleEnemySpawnMultipliers.generated.h"

/*
 * Enemy spawn count multipliers for each structural room role.
 * 構造役割ごとの敵スポーン数倍率です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonStructuralRoleEnemySpawnMultipliers
{
	GENERATED_BODY()

	/*
	 * Multiplier used for the start room.
	 * 開始部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Start rooms. 0 disables enemy spawning for the role."))
	float Start = 0.5f;

	/*
	 * Multiplier used for the goal room.
	 * ゴール部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Goal rooms. 0 disables enemy spawning for the role."))
	float Goal = 1.0f;

	/*
	 * Multiplier used for hub rooms.
	 * ハブ部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Hub rooms. 0 disables enemy spawning for the role."))
	float Hub = 1.0f;

	/*
	 * Multiplier used for connector rooms.
	 * 接続部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Connector rooms. 0 disables enemy spawning for the role."))
	float Connector = 1.0f;

	/*
	 * Multiplier used for branch rooms.
	 * 分岐部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Branch rooms. 0 disables enemy spawning for the role."))
	float Branch = 1.0f;

	/*
	 * Multiplier used for dead-end rooms.
	 * 行き止まり部屋に適用される倍率です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|SpawnActorInRoom", meta = (ClampMin = "0.00", ToolTip = "Enemy spawn count multiplier for Dead End rooms. 0 disables enemy spawning for the role."))
	float DeadEnd = 1.0f;
};
