/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
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
