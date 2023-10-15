/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Decorator/DungeonInteriorParts.h"

bool FDungeonInteriorParts::Match(const TArray<FString>& tags) const noexcept
{
	const TSet<FString>& interiorSystemTags = InteriorSystemTags.InteriorSystemTags();
	for (const FString& tag : tags)
	{
		if (interiorSystemTags.Contains(tag) == false && InteriorAdditionalTags.Contains(tag) == false)
			return false;
	}
	return true;
}
