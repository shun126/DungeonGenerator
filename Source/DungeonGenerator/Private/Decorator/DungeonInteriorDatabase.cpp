/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Decorator/DungeonInteriorDatabase.h"
#include "Core/Math/Random.h"
#include <GameFramework/Actor.h>

TArray<FDungeonInteriorParts> UDungeonInteriorDatabase::Select(const TArray<FString>& interiorTags, const std::shared_ptr<dungeon::Random>& random) const
{
	TArray<FDungeonInteriorParts> selects;

	if (interiorTags.Num() > 0)
	{
		for (const FDungeonInteriorParts& parts : Parts)
		{
			if (parts.Frequency >= random->Get<uint8>(100 + 1) && parts.Match(interiorTags))
			{
				selects.Emplace(parts);
			}
		}
	}

	return selects;
}

void UDungeonInteriorDatabase::EachInteriorLocation(const AActor* actor, std::function<void(UDungeonInteriorLocationComponent* component)> function) const
{
	actor->ForEachComponent<UDungeonInteriorLocationComponent>(false, [this, &function](UDungeonInteriorLocationComponent* interiorLocationComponent)
		{
			if (IsValid(interiorLocationComponent))
			{
				function(interiorLocationComponent);
			}
		}
	);
}
