/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonPartsSelectionMethod.generated.h"

/**
Part Selection Method
パーツを選択する方法
*/
UENUM(BlueprintType)
enum class EDungeonPartsSelectionMethod : uint8
{
	Random,
	GridIndex,
	Direction,
};
