/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonActorPartsWithDirection.h"
#include "DungeonRandomActorParts.generated.h"

/**
 * Actor parts selected with a configurable spawn chance.
 * 設定可能な生成確率で選択されるアクターパーツです。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRandomActorParts : public FDungeonActorPartsWithDirection
{
	GENERATED_BODY()

public:
	/**
	 * Migrates the version 1 normalized frequency to a percentage.
	 * バージョン1の正規化された頻度を百分率へ移行します。
	 */
	void MigrateFromVersion1() noexcept
	{
		SpawnChance = FMath::Clamp(Frequency * 100.f, 0.f, 100.f);
	}

	/** Version 1 normalized frequency retained only for migration. 移行専用に保持するバージョン1の正規化済み頻度です。 */
	UPROPERTY(meta = (DeprecatedProperty))
	float Frequency = 1.f;

	/**
	 * Percentage chance for this actor to participate in one selection.
	 * このアクターが1回の選択へ参加する確率（百分率）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "100", UIMin = "0", UIMax = "100", ForceUnits = "Percent", ToolTip = "Chance for this actor to participate in one selection. 0% never participates; 100% always participates."))
	float SpawnChance = 100.f;
};
