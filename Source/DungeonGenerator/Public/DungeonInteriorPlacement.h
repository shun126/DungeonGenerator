/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonInteriorPlacement.generated.h"

/**
 * Areas in which an interior or vegetation part may be placed.
 * インテリアまたは植生パーツを配置できるエリアです。
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDungeonInteriorPlacementArea : uint8
{
	None = 0 UMETA(Hidden),
	Room = 1 << 0 UMETA(DisplayName = "Room", ToolTip = "Allow placement inside generated rooms."),
	Aisle = 1 << 1 UMETA(DisplayName = "Aisle", ToolTip = "Allow placement inside generated flat aisles."),
	Slope = 1 << 2 UMETA(DisplayName = "Slope", ToolTip = "Allow placement inside generated slopes, stairwells, and multi-height passage spaces.")
};
ENUM_CLASS_FLAGS(EDungeonInteriorPlacementArea);

/**
 * Typed placement request passed to the interior database.
 * インテリアデータベースへ渡す型付き配置要求です。
 */
struct FDungeonInteriorPlacementQuery
{
	/** Candidate area, or None for nested context-only placement. 候補エリアです。入れ子のContext専用配置ではNoneです。 */
	EDungeonInteriorPlacementArea Area = EDungeonInteriorPlacementArea::None;
	/** Semantic context tags provided at this candidate. この候補位置から提供される意味的なContext Tagです。 */
	TSet<FString> ProvidedContextTags;
};
