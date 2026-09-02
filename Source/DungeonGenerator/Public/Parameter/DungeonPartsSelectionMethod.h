/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonPartsSelectionMethod.generated.h"

/**
 * Part Selection Method
 * パーツを選択する方法
 */
UENUM(BlueprintType)
enum class EDungeonPartsSelectionMethod : uint8
{
	Random UMETA(DisplayName = "Random", ToolTip = "Select parts randomly."),
	GridIndex UMETA(DisplayName = "Grid Index", ToolTip = "Select parts deterministically from the grid index."),
	Direction UMETA(DisplayName = "Direction", ToolTip = "Select parts deterministically from the grid direction."),
	Custom UMETA(DisplayName = "Custom (Scriptable)", ToolTip = "Use a custom selector callback to choose parts index. This runs on the generation hot path and can affect server/client determinism if unsynchronized randomness is used."),
};
