/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonRoomItem.h"

const FString& GetDungeonRoomItemName(const EDungeonRoomItem item)
{
	static const FString names[] = {
		TEXT("Empty"),
		TEXT("Key"),
		TEXT("UniqueKey"),
	};
	static_assert(DungeonRoomItemSize == (sizeof(names) / sizeof(names[0])));
	return names[static_cast<uint8>(item)];
}
