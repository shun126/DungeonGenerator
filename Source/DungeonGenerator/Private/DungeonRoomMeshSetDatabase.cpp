/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomMeshSetDatabase.h"
#include "Core/Math/Random.h"

const FDungeonRoomMeshSet* UDungeonRoomMeshSetDatabase::Select(const std::shared_ptr<dungeon::Random>& random) const
{
	const int32 size = Parts.Num();
	if (size <= 0)
		return nullptr;

	const uint32_t index = random->Get<uint32_t>(size);
	return &Parts[index];
}

const FDungeonRoomMeshSet* UDungeonRoomMeshSetDatabase::SelectImplement(const std::shared_ptr<dungeon::Random>& random) const
{
	return Select(random);
}
