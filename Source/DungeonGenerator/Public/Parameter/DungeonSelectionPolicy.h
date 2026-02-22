#pragma once
#include <CoreMinimal.h>
#include "DungeonSelectionPolicy.generated.h"

/**
 * Unified selection policy used internally by mesh-set and parts selection.
 * Legacy enums are kept for asset compatibility and migrated to this policy.
 */
UENUM(BlueprintType)
enum class EDungeonSelectionPolicy : uint8
{
	Random UMETA(DisplayName = "Random", ToolTip = "Select randomly."),
	GridIndex UMETA(DisplayName = "Grid Index", ToolTip = "Select deterministically from grid index."),
	Direction UMETA(DisplayName = "Direction", ToolTip = "Select deterministically from grid direction."),
	Identifier UMETA(DisplayName = "Identifier", ToolTip = "Select deterministically from grid identifier."),
	DepthFromStart UMETA(DisplayName = "Depth From Start", ToolTip = "Select based on distance from the start."),
};
