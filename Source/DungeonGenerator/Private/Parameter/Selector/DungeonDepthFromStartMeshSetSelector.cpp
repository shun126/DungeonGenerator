/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonDepthFromStartMeshSetSelector.h"
#include <cmath>

int32 UDungeonDepthFromStartMeshSetSelector::SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	(void)Random;
	if (NumCandidates <= 0)
		return INDEX_NONE;

	const float index = static_cast<float>(NumCandidates - 1) * Query.DepthFromStart;
	return static_cast<int32>(std::round(index));
}
