/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonMeshSetSelectionMethod.generated.h"

/**
Part Selection Method
パーツを選択する方法
*/
UENUM(BlueprintType)
enum class EDungeonMeshSetSelectionMethod : uint8
{
	Identifier,
	DepthFromStart,
	Random,
};
