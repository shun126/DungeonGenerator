/**
部屋の中に構造物を生成できるか調査・生成します

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "RoomStructureGenerator.h"
#include "Voxel.h"
#include "../RoomGeneration/Room.h"

namespace dungeon
{
	struct Offset final
	{
		int32 mX, mZ;
		Grid::Type mGridType;
	};
	static const std::array<Offset, 5> Offsets = {
		{
			{ 0, 1, Grid::Type::Aisle },
			{ 1, 1, Grid::Type::Stairwell },
			{ 2, 1, Grid::Type::UpSpace },
			{ 1, 0, Grid::Type::DownSpace },
			{ 2, 0, Grid::Type::Slope }
		},
	};

	bool RoomStructureGenerator::CheckSlopePlacement(const std::shared_ptr<Voxel>& voxel, const std::shared_ptr<Room>& room, const std::shared_ptr<Random>& random)
	{
		if (room->GetHeight() < 2)
			return false;

		auto checker = [voxel](const int32 x, const int32 y, const int32 z, const Identifier& identifier, const int32 xFlip, const int32 yFlip)
			{
				for (const auto& offset : Offsets)
				{
					const int32 ox = x + offset.mX * xFlip;
					const int32 oy = y + offset.mX * yFlip;
					const int32 oz = z + offset.mZ;
					const Grid& grid = voxel->Get(ox, oy, oz);
					if (grid.GetIdentifier() != identifier)
						return false;
					const auto gridType = grid.GetType();
					if (offset.mZ == 0)
					{
						if (gridType != Grid::Type::Deck)
							return false;
					}
					else
					{
						if (gridType != Grid::Type::Floor)
							return false;
					}
				}

				const int32 ox = x + 3 * xFlip;
				const int32 oy = y + 3 * yFlip;
				const Grid& grid = voxel->Get(ox, oy, z);
				if (grid.GetIdentifier() != identifier)
					return false;
				const auto gridType = grid.GetType();
				return gridType == Grid::Type::Deck || gridType == Grid::Type::Gate;
			};

		// 部屋の四隅にスロープが生成できるか調べます
		std::vector<EffectiveDirection> pass;
		if (room->GetWidth() >= 4)
		{
			if (checker(room->GetLeft(), room->GetTop(), room->GetBackground(), room->GetIdentifier(), 1, 0) == true)
				pass.emplace_back(EffectiveDirection::HorizontalNortheast);
			if (checker(room->GetRight() - 1, room->GetTop(), room->GetBackground(), room->GetIdentifier(), -1, 0) == true)
				pass.emplace_back(EffectiveDirection::HorizontalNorthwest);
			if (checker(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), -1, 0) == true)
				pass.emplace_back(EffectiveDirection::HorizontalSouthwest);
			if (checker(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 1, 0) == true)
				pass.emplace_back(EffectiveDirection::HorizontalSoutheast);
		}
		if (room->GetDepth() >= 4)
		{
			if (checker(room->GetLeft(), room->GetTop(), room->GetBackground(), room->GetIdentifier(), 0, 1) == true)
				pass.emplace_back(EffectiveDirection::VerticalNortheast);
			if (checker(room->GetRight() - 1, room->GetTop(), room->GetBackground(), room->GetIdentifier(), 0, 1) == true)
				pass.emplace_back(EffectiveDirection::VerticalNorthwest);
			if (checker(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 0, -1) == true)
				pass.emplace_back(EffectiveDirection::VerticalSouthwest);
			if (checker(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 0, -1) == true)
				pass.emplace_back(EffectiveDirection::VerticalSoutheast);
		}

		// 抽選
		mValid = pass.empty() == false;
		if (mValid)
		{
			const size_t index = random->Get<size_t>(pass.size());
			mEffectiveDirection = pass[index];

			switch (mEffectiveDirection)
			{
			case EffectiveDirection::HorizontalNortheast:
			case EffectiveDirection::VerticalNortheast:
				mLocation.X = room->GetRight() - 1;
				mLocation.Y = room->GetTop();
				break;

			case EffectiveDirection::HorizontalNorthwest:
			case EffectiveDirection::VerticalNorthwest:
				mLocation.X = room->GetLeft();
				mLocation.Y = room->GetTop();
				break;

			case EffectiveDirection::HorizontalSouthwest:
			case EffectiveDirection::VerticalSouthwest:
				mLocation.X = room->GetLeft();
				mLocation.Y = room->GetBottom() - 1;
				break;

			case EffectiveDirection::HorizontalSoutheast:
			case EffectiveDirection::VerticalSoutheast:
				mLocation.X = room->GetRight() - 1;
				mLocation.Y = room->GetBottom() - 1;
				break;

			default:
				return false;
			}
			mLocation.Z = room->GetBackground();
		}

		return mValid;
	}

	FIntVector RoomStructureGenerator::GetGateLocation() const noexcept
	{
		return FIntVector(mLocation.X, mLocation.Y, mLocation.Z + Offsets[0].mZ);
	}

	void RoomStructureGenerator::GenerateSlope(const std::shared_ptr<Voxel>& voxel) const
	{
		if (mValid == false)
			return;

		// 中二階グリッド属性のみ設定
		static_assert(Offsets.empty() == false);
		if (Offsets[0].mZ > 0)
		{
			const FIntVector location = mLocation + FIntVector(Offsets[0].mX, 0, Offsets[0].mZ);
			auto grid = voxel->Get(location);
			grid.Catwalk(true);
			voxel->Set(location, grid);
		}

		// 中二階グリッドを生成
		for (size_t i = 1; i < Offsets.size(); ++i)
		{
			const auto& offset = Offsets[i];
			Direction direction;
			int32 ox, oy;
			switch (mEffectiveDirection)
			{
			case EffectiveDirection::HorizontalNortheast:
				ox = mLocation.X - offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::East);
				break;

			case EffectiveDirection::HorizontalNorthwest:
				ox = mLocation.X + offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::West);
				break;

			case EffectiveDirection::HorizontalSouthwest:
				ox = mLocation.X + offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::West);
				break;

			case EffectiveDirection::HorizontalSoutheast:
				ox = mLocation.X - offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::East);
				break;

			case EffectiveDirection::VerticalNortheast:
				ox = mLocation.X;
				oy = mLocation.Y + offset.mX;
				direction.Set(Direction::North);
				break;

			case EffectiveDirection::VerticalNorthwest:
				ox = mLocation.X;
				oy = mLocation.Y + offset.mX;
				direction.Set(Direction::North);
				break;

			case EffectiveDirection::VerticalSouthwest:
				ox = mLocation.X;
				oy = mLocation.Y - offset.mX;
				direction.Set(Direction::South);
				break;

			case EffectiveDirection::VerticalSoutheast:
				ox = mLocation.X;
				oy = mLocation.Y - offset.mX;
				direction.Set(Direction::South);
				break;

			default:
				return;
			}

			const int32 oz = mLocation.Z + offset.mZ;
			auto grid = voxel->Get(ox, oy, oz);
			grid.SetType(offset.mGridType);
			grid.SetDirection(direction);
			if (offset.mZ > 0)
				grid.Catwalk(true);
			voxel->Set(ox, oy, oz, grid);
		}
	}
}
