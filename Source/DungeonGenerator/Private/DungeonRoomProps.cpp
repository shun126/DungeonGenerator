/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonRoomProps.h"

const FString& GetDungeonRoomPropsName(const EDungeonRoomProps props)
{
	static const FString names[] = {
		TEXT("None"),
		TEXT("Lock"),
		TEXT("UniqueLock"),
	};
	static_assert(DungeonRoomPropsSize == (sizeof(names) / sizeof(names[0])));
	return names[static_cast<uint8>(props)];
}
