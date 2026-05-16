/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonMeshSetSelectionMethod.generated.h"

/**
 * MeshSet Selection Method
 * メッシュセットを選択する方法
 */
UENUM(BlueprintType)
enum class EDungeonMeshSetSelectionMethod : uint8
{
	Random UMETA(DisplayName = "Random", ToolTip = "Select mesh sets randomly."),
	Identifier UMETA(DisplayName = "Identifier", ToolTip = "Select mesh sets based on grid identifier rules."),
	DepthFromStart UMETA(DisplayName = "Depth From Start", ToolTip = "Select mesh sets based on distance from the start."),
};
