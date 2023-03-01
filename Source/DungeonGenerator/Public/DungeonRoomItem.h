/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

/*
Items to be placed in the room
Same content as dungeon::Room::Item
*/
UENUM(BlueprintType)
enum class EDungeonRoomItem : uint8
{
	Empty,
	Key,
	UniqueKey,
};

/**
Number of different items to be placed in the room
*/
static constexpr uint8 DungeonRoomItemSize = static_cast<uint8>(EDungeonRoomItem::UniqueKey) + 1;

/*
Gets the symbolic name of the item to be placed in the room
Same content as dungeon::Room::Parts
\param[in]	item	EDungeonRoomItem
\return EDungeonRoomItem symbol name
*/
extern const FString& GetDungeonRoomItemName(const EDungeonRoomItem item);
