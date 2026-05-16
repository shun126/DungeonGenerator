/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonPartsPlacementDirection.generated.h"

/**
 * Parts placement direction
 * パーツを配置する方向
 */
UENUM(BlueprintType)
enum class EDungeonPartsPlacementDirection : uint8
{
	North UMETA(DisplayName = "North", ToolTip = "Place parts facing north."),
	East UMETA(DisplayName = "East", ToolTip = "Place parts facing east."),
	South UMETA(DisplayName = "South", ToolTip = "Place parts facing south."),
	West UMETA(DisplayName = "West", ToolTip = "Place parts facing west."),
	FollowGridDirection UMETA(DisplayName = "Follow Grid Direction", ToolTip = "Place parts using the current grid direction."),
	RandomDirection UMETA(DisplayName = "Random Direction", ToolTip = "Place parts with a random direction.")
};
