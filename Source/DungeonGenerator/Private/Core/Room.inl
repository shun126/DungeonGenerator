/**
部屋に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline bool Room::IsUndeletable() const noexcept
	{
		return mUndeletable;
	}

	inline void Room::SetUndeletables(const bool undeletable) noexcept
	{
		mUndeletable = undeletable;
	}

	inline void Room::SetNoMeshGeneration(const bool noRoofMeshGeneration, const bool noFloorMeshGeneration)
	{
		mNoMeshGeneration.set(static_cast<uint8_t>(NoMeshGeneration::Roof), noRoofMeshGeneration);
		mNoMeshGeneration.set(static_cast<uint8_t>(NoMeshGeneration::Floor), noFloorMeshGeneration);
	}

	inline bool Room::IsNoFloorMeshGeneration() const noexcept
	{
		return mNoMeshGeneration.test(static_cast<uint8_t>(NoMeshGeneration::Floor));
	}

	inline bool Room::IsNoRoofMeshGeneration() const noexcept
	{
		return mNoMeshGeneration.test(static_cast<uint8_t>(NoMeshGeneration::Roof));
	}
}
