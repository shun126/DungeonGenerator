/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "Parameter/DungeonSelectionQuery.h"
#include "DungeonPartsSelector.generated.h"

/**
 * Optional selector object used when EDungeonPartsSelectionMethod is Custom.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonPartsSelector : public UObject
{
	GENERATED_BODY()

public:
};
