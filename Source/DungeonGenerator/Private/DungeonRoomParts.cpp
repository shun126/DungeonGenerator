/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomParts.h"

const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts)
{
	static const FString names[] = {
		TEXT("Any"),
		TEXT("Start"),
		TEXT("Goal"),
		TEXT("Hall"),
		TEXT("Hanare"),
	};
	static_assert(DungeonRoomPartsSize == (sizeof(names) / sizeof(names[0])));
	return names[static_cast<uint8>(parts)];
}
