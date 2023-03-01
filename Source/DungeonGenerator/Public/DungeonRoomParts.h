/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

/**
Type of room parts
Same content as dungeon::Room::Parts
*/
UENUM(BlueprintType)
enum class EDungeonRoomParts : uint8
{
	Any,			//!< Unspecified (equivalent to dungeon::Room::Parts::Unidentified)
	Start,			//!< starting point
	Goal,			//!< goal point
	Hall,			//!< A hall (several corridors are connected)
	Hanare,			//!< separate room (with only one passageway connected to it)
};

/**
Number of different room parts
*/
static constexpr uint8 DungeonRoomPartsSize = static_cast<uint8>(EDungeonRoomParts::Hanare) + 1;

/**
Get the symbol name of a room part.
Same content as dungeon::Room::Parts
\param[in]	parts	EDungeonRoomParts
\return		Symbol name for EDungeonRoomParts
*/
extern const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts);
