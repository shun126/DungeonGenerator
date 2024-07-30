/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonPartsPlacementDirection.generated.h"

/**
Parts placement direction
パーツを配置する方向
*/
UENUM(BlueprintType)
enum class EDungeonPartsPlacementDirection : uint8
{
	North,
	East,
	South,
	West,
	FollowGridDirection,
	RandomDirection
};
