/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Mission/DungeonRoomParts.h"

const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts)
{
	static const FString names[] = {
		TEXT("Any"),
		TEXT("Hall"),
		TEXT("Hanare"),
		TEXT("Start"),
		TEXT("Goal"),
	};
	static_assert(DungeonRoomPartsSize == (sizeof(names) / sizeof(names[0])));
	return names[static_cast<uint8>(parts)];
}
