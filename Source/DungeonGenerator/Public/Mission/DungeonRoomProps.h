/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

/**
Objects to be placed in the room
Same content as dungeon::Grid::Props
*/
UENUM(BlueprintType)
enum class EDungeonRoomProps : uint8
{
	None,
	Lock,
	UniqueLock,
};

/**
Number of different types of placements to be placed in the room
*/
static constexpr uint8 DungeonRoomPropsSize = static_cast<uint8>(EDungeonRoomProps::UniqueLock) + 1;

/**
Gets the symbolic name of the placement that should be placed in the room
Same content as dungeon::Room::Parts
\param[in]	props	EDungeonRoomProps
\return		EDungeonRoomProps symbol name
*/
extern const FString& GetDungeonRoomPropsName(const EDungeonRoomProps props);
