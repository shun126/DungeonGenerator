/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

namespace dungeon
{
	inline FString Indent(const uint32 depth) noexcept
	{
		FString indent;
		for (uint32 i = 0; i < depth; ++i)
			indent += TEXT(" ");
		return indent;
	}
}
