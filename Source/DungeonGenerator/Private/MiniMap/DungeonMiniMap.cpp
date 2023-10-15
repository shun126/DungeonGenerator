/*
A mini-map of the dungeon generated in the editor
It contains the mini-map textures of each floor and the information
about the conversion from world coordinates to texture coordinates.
*/

#include "MiniMap/DungeonMiniMap.h"

int32 UDungeonMiniMap::TransformWorldToLayer(const float worldElevation) const noexcept
{
	const int32 voxelHeight = static_cast<int32>(worldElevation * WorldToVoxelScale);
	for (int32 i = 0; i < Elevations.Num(); ++i)
	{
		if (voxelHeight <= Elevations[i])
			return i;
	}
	return 0;
}
