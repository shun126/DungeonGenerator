/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonRoomParts.generated.h"

/**
 * Type of room parts
 * Same content as dungeon::Room::Parts
 *
 * ルームパーツの種類
 * dungeon::Room::Parts と同じ内容です。
 */
UENUM(BlueprintType)
enum class EDungeonRoomParts : uint8
{
	Any UMETA(DisplayName = "Any", ToolTip = "Unspecified room part."),
	Hall UMETA(DisplayName = "Hall", ToolTip = "A hall where several corridors are connected."),
	Hanare UMETA(DisplayName = "Hanare", ToolTip = "A detached room connected by only one passageway."),
	Start UMETA(DisplayName = "Start", ToolTip = "The dungeon start point room."),
	Goal UMETA(DisplayName = "Goal", ToolTip = "The dungeon goal point room."),
};

/**
 * Type of room parts
 * Same content as dungeon::Room::Parts
 *
 * ルームパーツの種類
 * dungeon::Room::Parts と同じ内容です。
 */
UENUM(BlueprintType)
enum class EDungeonRoomLocatorParts : uint8
{
	Any UMETA(DisplayName = "Any", ToolTip = "Unspecified room part."),
	Hall UMETA(DisplayName = "Hall", ToolTip = "A hall where several corridors are connected."),
	Hanare UMETA(DisplayName = "Hanare", ToolTip = "A detached room connected by only one passageway."),
};

/**
 * Number of different room parts
 */
static constexpr uint8 DungeonRoomPartsSize = static_cast<uint8>(EDungeonRoomParts::Goal) + 1;

/**
 * Get the symbol name of a room part.
 * Same content as dungeon::Room::Parts
 * @param[in]	parts	EDungeonRoomParts
 * @return		Symbol name for EDungeonRoomParts
 */
extern const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts);

/**
 * Compare EDungeonRoomParts and EDungeonRoomLocatorParts
 */
inline constexpr bool Equal(const EDungeonRoomParts parts, const EDungeonRoomLocatorParts locatorParts)
{
	return static_cast<uint8>(parts) == static_cast<uint8>(locatorParts);
}

/**
 * Compare EDungeonRoomParts and EDungeonRoomLocatorParts
 */
inline constexpr bool Equal(const EDungeonRoomLocatorParts locatorParts, const EDungeonRoomParts parts)
{
	return Equal(parts, locatorParts);
}

/**
 * Cast from EDungeonRoomLocatorParts to EDungeonRoomParts
 *
 * EDungeonRoomLocatorPartsからEDungeonRoomPartsへのキャスト
 */
inline constexpr EDungeonRoomParts Cast(const EDungeonRoomLocatorParts parts)
{
	return static_cast<EDungeonRoomParts>(parts);
}

/**
 * Cast from EDungeonRoomParts to EDungeonRoomLocatorParts
 *
 * EDungeonRoomPartsからEDungeonRoomLocatorPartsへのキャスト
 */
inline constexpr EDungeonRoomLocatorParts Cast(const EDungeonRoomParts parts)
{
	return static_cast<EDungeonRoomLocatorParts>(parts);
}
