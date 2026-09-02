/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonDirectionPartsSelector.h"

int32 UDungeonDirectionPartsSelector::SelectPartsIndexNative(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	(void)Random;
	if (NumCandidates <= 0)
		return INDEX_NONE;
	return Query.Rotation % NumCandidates;
}
