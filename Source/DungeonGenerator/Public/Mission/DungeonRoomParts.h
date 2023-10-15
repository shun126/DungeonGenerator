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
	Hall,			//!< A hall (several corridors are connected)
	Hanare,			//!< separate room (with only one passageway connected to it)
	Start,			//!< starting point
	Goal,			//!< goal point
};

/**
Type of room parts
Same content as dungeon::Room::Parts
*/
UENUM(BlueprintType)
enum class EDungeonRoomLocatorParts : uint8
{
	Any,			//!< Unspecified (equivalent to dungeon::Room::Parts::Unidentified)
	Hall,			//!< A hall (several corridors are connected)
	Hanare,			//!< separate room (with only one passageway connected to it)
};

/**
Number of different room parts
*/
static constexpr uint8 DungeonRoomPartsSize = static_cast<uint8>(EDungeonRoomParts::Goal) + 1;

/**
Get the symbol name of a room part.
Same content as dungeon::Room::Parts
\param[in]	parts	EDungeonRoomParts
\return		Symbol name for EDungeonRoomParts
*/
extern const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts);

/**
Compare EDungeonRoomParts and EDungeonRoomLocatorParts
*/
inline constexpr bool Equal(const EDungeonRoomParts parts, const EDungeonRoomLocatorParts locatorParts)
{
	return static_cast<uint8>(parts) == static_cast<uint8>(locatorParts);
}

/**
Compare EDungeonRoomParts and EDungeonRoomLocatorParts
*/
inline constexpr bool Equal(const EDungeonRoomLocatorParts locatorParts, const EDungeonRoomParts parts)
{
	return Equal(parts, locatorParts);
}
