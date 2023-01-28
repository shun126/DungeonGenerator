/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
Same content as dungeon::Room::Item
*/
UENUM(BlueprintType)
enum class EDungeonRoomItem : uint8
{
	Empty,
	Key,
	UniqueKey,
};
