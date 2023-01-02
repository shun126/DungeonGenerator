/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
Same content as dungeon::Grid::Props
*/
UENUM(BlueprintType)
enum class EDungeonRoomProps : uint8
{
	None,
	Lock,
	UniqueLock,
};
