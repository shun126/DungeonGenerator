/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

// cppcheck-suppress [unknownMacro]
UENUM(BlueprintType)
enum class EDungeonRoomHeightCondition : uint8
{
	Equal,
	EqualGreater,
};
