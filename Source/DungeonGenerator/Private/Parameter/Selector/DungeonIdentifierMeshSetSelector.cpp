/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonIdentifierMeshSetSelector.h"

int32 UDungeonIdentifierMeshSetSelector::SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	(void)Random;
	if (NumCandidates <= 0)
		return INDEX_NONE;
	return FMath::Abs(Query.RoomId) % NumCandidates;
}
