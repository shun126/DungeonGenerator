/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonAisleMeshSet.h"

const FDungeonMeshParts* FDungeonAisleMeshSet::SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return FDungeonRoomMeshSet::SelectParts(gridIndex, grid, random, SlopeParts, SloopPartsSelectionMethod);
}

void FDungeonAisleMeshSet::EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	FDungeonRoomMeshSet::EachParts(SlopeParts, func);
}
