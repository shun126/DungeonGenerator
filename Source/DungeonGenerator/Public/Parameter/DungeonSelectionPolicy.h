/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonSelectionPolicy.generated.h"

/**
 * Unified selection policy used internally by mesh-set and parts selection.
 * Legacy enums are kept for asset compatibility and migrated to this policy.
 * メッシュセット選択とパーツ選択で共通して使用する選択ポリシーです。
 * 旧列挙型はアセット互換性のために保持され、このポリシーへ移行されます。
 */
UENUM(BlueprintType)
enum class EDungeonSelectionPolicy : uint8
{
	Random UMETA(DisplayName = "Random", ToolTip = "Select randomly."),
	GridIndex UMETA(DisplayName = "Grid Index", ToolTip = "Select deterministically from grid index."),
	Direction UMETA(DisplayName = "Direction", ToolTip = "Select deterministically from grid direction."),
	Identifier UMETA(DisplayName = "Identifier", ToolTip = "Select deterministically from grid identifier."),
	DepthFromStart UMETA(DisplayName = "Depth From Start", ToolTip = "Select based on distance from the start."),
	CustomSelector UMETA(DisplayName = "Custom Selector", ToolTip = "Use UDungeonBlueprintPartsSelector or UDungeonBlueprintMeshSetSelector."),
};
