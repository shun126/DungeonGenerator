/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorAsset.h"
#include "Core/Math/Random.h"

TArray<FDungeonInteriorParts> UDungeonInteriorAsset::Select(const TArray<FString>& interiorTags, dungeon::Random& random) const
{
	TArray<FDungeonInteriorParts> selects;

	if (interiorTags.IsEmpty())
	{
		for (const FDungeonInteriorParts& parts : Parts)
		{
			if (parts.Frequency >= random.Get<uint8>(100))
			{
				selects.Emplace(parts);
			}
		}
	}
	else
	{
		for (const FDungeonInteriorParts& parts : Parts)
		{
			if (parts.Frequency >= random.Get<uint8>(100) &&
				parts.Match(interiorTags))
			{
					selects.Emplace(parts);
			}
		}
	}

	return selects;
}

void UDungeonInteriorAsset::EachInteriorLocation(const AActor* actor, std::function<void(UDungeonInteriorLocationComponent* component)> function) const
{
	actor->ForEachComponent<UDungeonInteriorLocationComponent>(false, [this, &function](UDungeonInteriorLocationComponent* interiorLocationComponent)
		{
			if (IsValid(interiorLocationComponent))
			{
				TArray<FDungeonInteriorParts> partsResult;
				for (const FDungeonInteriorParts& parts : Parts)
				{
					if (parts.Match(interiorLocationComponent->GetInteriorTags()))
						partsResult.Emplace(parts);
				}
				function(interiorLocationComponent);
			}
		}
	);
}
