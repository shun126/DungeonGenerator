/**
部屋の中に構造物を生成できるか調査・生成します

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "RoomStructureGenerator.h"
#include "Voxel.h"
#include "../Debug/Debug.h"
#include "../RoomGeneration/Room.h"

namespace dungeon
{
	struct Offset final
	{
		int32 mX, mZ;
		bool bShouldWriteGrid;
		Grid::Type mWriteGridType;
	};
	static const std::array<Offset, 7> Offsets = {
		{
			{ 0, 1, true, Grid::Type::Aisle },		// 属性のみ設定
			{ 1, 1, true, Grid::Type::Stairwell },
			{ 2, 1, true, Grid::Type::UpSpace },
			{ 1, 0, true, Grid::Type::DownSpace },
			{ 2, 0, true, Grid::Type::Slope },
			{ 0, 0, false, Grid::Type::OutOfBounds },
			{ 3, 0, false, Grid::Type::OutOfBounds },
		},
	};

	bool RoomStructureGenerator::CheckSlopePlacement(const std::shared_ptr<Voxel>& voxel, const std::shared_ptr<Room>& room, const FIntVector& baseLocation, const std::shared_ptr<Random>& random)
	{
		if (room->GetHeight() < 2)
			return false;

		auto checker = [voxel](const int32 x, const int32 y, const int32 z, const Identifier& identifier, const int32 xFlip, const int32 yFlip, const int32 xLanding, const int32 yLanding) -> bool
			{
				auto commonChecker = [voxel](const Grid& grid, const Identifier& identifier) -> bool
					{
						// 識別子が違うなら不許可
						if (grid.GetIdentifier() != identifier)
							return false;
						// 予約済みなら不許可
						if (grid.IsReserved())
							return false;
						// 中二階なら不許可
						if (grid.IsCatwalk())
							return false;
						// サブレベル生成済みなら不許可
						if (grid.IsSubLevel())
							return false;
						return true;
					};

				// スロープと中二階部分の判定
				for (const auto& offset : Offsets)
				{
					const int32 ox = x + offset.mX * xFlip;
					const int32 oy = y + offset.mX * yFlip;
					const int32 oz = z + offset.mZ;
					const Grid& grid = voxel->Get(ox, oy, oz);
					if (!commonChecker(grid, identifier))
						return false;
					if (offset.mZ == 0)
					{
						if (offset.bShouldWriteGrid)
						{
							if (grid.GetType() != Grid::Type::Deck)
								return false;
						}
						else
						{
							// 踊り場の判定
							if (grid.GetType() != Grid::Type::Deck && grid.GetType() != Grid::Type::Gate)
								return false;
						}
					}
					else
					{
						if (grid.GetType() != Grid::Type::Floor)
							return false;
					}
				}
				// スロープ前にいずれかが必要な床(踊り場への接続)の判定
				{
					const Grid& grid1 = voxel->Get(x + 3 * xFlip + xLanding, y + 3 * yFlip + yLanding, z);
					if (commonChecker(grid1, identifier) && (grid1.GetType() == Grid::Type::Deck || grid1.GetType() == Grid::Type::Gate))
						return true;

					const Grid& grid2 = voxel->Get(x + 4 * xFlip, y + 4 * yFlip, z);
					return commonChecker(grid2, identifier) && (grid2.GetType() == Grid::Type::Deck || grid2.GetType() == Grid::Type::Gate);
				}
			};

		auto VectorDistance = [](const int32 v0x, const int32 v0y, const int32 v0z, const FIntVector& v1) -> uint32_t
			{
				return
					static_cast<uint32_t>(v0x * v1.X) +
					static_cast<uint32_t>(v0y * v1.Y) +
					static_cast<uint32_t>(v0z * v1.Z);
			};

		// 部屋の四隅にスロープが生成できるか調べます
		struct Entry final
		{
			uint32_t mDistance;
			EffectiveDirection mEffectiveDirection;
			Entry(const uint32_t distance, const EffectiveDirection effectiveDirection)
				: mDistance(distance)
				, mEffectiveDirection(effectiveDirection)
			{}
		};
		std::vector<Entry> pass;
		pass.reserve(8);
		uint32_t furthestDistance = 0;
		// 東西方向
		if (room->GetWidth() >= 4)
		{
			// 北東
			if (checker(room->GetRight() - 1, room->GetTop(), room->GetBackground(), room->GetIdentifier(), -1, 0, 0, 1) == true)
			{
				const uint32_t distance = VectorDistance(room->GetRight() - 1, room->GetTop(), room->GetBackground(), baseLocation);
				pass.emplace_back(distance, EffectiveDirection::HorizontalNortheast);
			}
			// 北西
			if (checker(room->GetLeft(), room->GetTop(), room->GetBackground(), room->GetIdentifier(), 1, 0, 0, 1) == true)
			{
				const uint32_t distance = VectorDistance(room->GetLeft(), room->GetTop(), room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::HorizontalNorthwest);
			}
			// 南西
			if (checker(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 1, 0, 0, -1) == true)
			{
				const uint32_t distance = VectorDistance(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::HorizontalSouthwest);
			}
			// 南東
			if (checker(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), -1, 0, 0, -1) == true)
			{
				const uint32_t distance = VectorDistance(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::HorizontalSoutheast);
			}
		}
		// 南北方向
		if (room->GetDepth() >= 4)
		{
			// 北東
			if (checker(room->GetRight() - 1, room->GetTop(), room->GetBackground(), room->GetIdentifier(), 0, 1, -1, 0) == true)
			{
				const uint32_t distance = VectorDistance(room->GetRight() - 1, room->GetTop(), room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::VerticalNortheast);
			}
			// 北西
			if (checker(room->GetLeft(), room->GetTop(), room->GetBackground(), room->GetIdentifier(), 0, 1, 1, 0) == true)
			{
				const uint32_t distance = VectorDistance(room->GetLeft(), room->GetTop(), room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::VerticalNorthwest);
			}
			// 南西
			if (checker(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 0, -1, 1, 0) == true)
			{
				const uint32_t distance = VectorDistance(room->GetLeft(), room->GetBottom() - 1, room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::VerticalSouthwest);
			}
			// 南東
			if (checker(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), room->GetIdentifier(), 0, -1, -1, 0) == true)
			{
				const uint32_t distance = VectorDistance(room->GetRight() - 1, room->GetBottom() - 1, room->GetBackground(), baseLocation);
				furthestDistance = std::max(furthestDistance, distance);
				pass.emplace_back(distance, EffectiveDirection::VerticalSoutheast);
			}
		}

		// 抽選
		mValid = pass.empty() == false;
		if (mValid)
		{
			// 以降mDistanceは当選確率として扱う
			++furthestDistance;
			uint32_t probability = 0;
			for (Entry& entry : pass)
			{
				probability += furthestDistance - entry.mDistance;
				entry.mDistance = probability;
			}
			const uint32_t dice = random->Get<uint32_t>(probability + 1);
			for (const Entry& entry : pass)
			{
				if (dice <= entry.mDistance)
				{
					mEffectiveDirection = entry.mEffectiveDirection;
					break;
				}
			}
			switch (mEffectiveDirection)
			{
			case EffectiveDirection::HorizontalNortheast:
			case EffectiveDirection::VerticalNortheast:
				// 北東
				mLocation.X = room->GetRight() - 1;
				mLocation.Y = room->GetTop();
				break;

			case EffectiveDirection::HorizontalNorthwest:
			case EffectiveDirection::VerticalNorthwest:
				// 北西
				mLocation.X = room->GetLeft();
				mLocation.Y = room->GetTop();
				break;

			case EffectiveDirection::HorizontalSouthwest:
			case EffectiveDirection::VerticalSouthwest:
				// 南西
				mLocation.X = room->GetLeft();
				mLocation.Y = room->GetBottom() - 1;
				break;

			case EffectiveDirection::HorizontalSoutheast:
			case EffectiveDirection::VerticalSoutheast:
				// 南東
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

			Direction catwalkDirection;
			switch (mEffectiveDirection)
			{
			case EffectiveDirection::HorizontalNortheast:
				catwalkDirection.Set(Direction::North);
				break;

			case EffectiveDirection::HorizontalNorthwest:
				catwalkDirection.Set(Direction::North);
				break;

			case EffectiveDirection::HorizontalSouthwest:
				catwalkDirection.Set(Direction::South);
				break;

			case EffectiveDirection::HorizontalSoutheast:
				catwalkDirection.Set(Direction::South);
				break;

			case EffectiveDirection::VerticalNortheast:
				catwalkDirection.Set(Direction::East);
				break;

			case EffectiveDirection::VerticalNorthwest:
				catwalkDirection.Set(Direction::West);
				break;

			case EffectiveDirection::VerticalSouthwest:
				catwalkDirection.Set(Direction::West);
				break;

			case EffectiveDirection::VerticalSoutheast:
				catwalkDirection.Set(Direction::East);
				break;
			}
			grid.SetCatwalkDirection(catwalkDirection);

			voxel->Set(location, grid);
		}

		// 中二階グリッドを生成
		for (size_t i = 1; i < Offsets.size(); ++i)
		{
			const auto& offset = Offsets[i];
			Direction direction;
			Direction catwalkDirection;
			int32 ox, oy;
			switch (mEffectiveDirection)
			{
			case EffectiveDirection::HorizontalNortheast:
				ox = mLocation.X - offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::East);
				catwalkDirection.Set(Direction::North);
				break;

			case EffectiveDirection::HorizontalNorthwest:
				ox = mLocation.X + offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::West);
				catwalkDirection.Set(Direction::North);
				break;

			case EffectiveDirection::HorizontalSouthwest:
				ox = mLocation.X + offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::West);
				catwalkDirection.Set(Direction::South);
				break;

			case EffectiveDirection::HorizontalSoutheast:
				ox = mLocation.X - offset.mX;
				oy = mLocation.Y;
				direction.Set(Direction::East);
				catwalkDirection.Set(Direction::South);
				break;

			case EffectiveDirection::VerticalNortheast:
				ox = mLocation.X;
				oy = mLocation.Y + offset.mX;
				direction.Set(Direction::North);
				catwalkDirection.Set(Direction::East);
				break;

			case EffectiveDirection::VerticalNorthwest:
				ox = mLocation.X;
				oy = mLocation.Y + offset.mX;
				direction.Set(Direction::North);
				catwalkDirection.Set(Direction::West);
				break;

			case EffectiveDirection::VerticalSouthwest:
				ox = mLocation.X;
				oy = mLocation.Y - offset.mX;
				direction.Set(Direction::South);
				catwalkDirection.Set(Direction::West);
				break;

			case EffectiveDirection::VerticalSoutheast:
				ox = mLocation.X;
				oy = mLocation.Y - offset.mX;
				direction.Set(Direction::South);
				catwalkDirection.Set(Direction::East);
				break;

			default:
				DUNGEON_GENERATOR_ERROR(TEXT("An incorrect orientation was detected on the slope in the room, so cancel the generation of this slope"));
				continue;
			}

			const int32 oz = mLocation.Z + offset.mZ;
			auto grid = voxel->Get(ox, oy, oz);
			grid.Reserve(true);
			if (offset.bShouldWriteGrid)
			{
				grid.SetType(offset.mWriteGridType);
				grid.SetDirection(direction);
				grid.SetCatwalkDirection(catwalkDirection);
				if (offset.mZ > 0)
					grid.Catwalk(true);
			}
			voxel->Set(ox, oy, oz, grid);
		}
	}
}
