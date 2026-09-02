/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonUniformRandomMeshSetSelector.h"
#include "Core/Math/Random.h"

int32 UDungeonUniformRandomMeshSetSelector::SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	if (NumCandidates <= 0)
		return INDEX_NONE;
	if (Random != nullptr)
		return Random->Get<int32_t>(NumCandidates);
	return FMath::Abs(Query.SeedKey) % NumCandidates;
}
