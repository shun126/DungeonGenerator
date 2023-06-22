/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorParts.h"

bool FDungeonInteriorParts::Match(const TArray<FString>& tags) const noexcept
{
	for (const FString& tag : tags)
	{
		if (InteriorTags.Contains(tag) == false)
			return false;
	}
	return true;
}
