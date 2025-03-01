/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Helper/NonCopyable.h"
#include <CoreMinimal.h>
#include <memory>

namespace dungeon
{
	class Random;
	class Room;
	class Voxel;

	/**
	 * 部屋の中に構造物を生成できるか調査・生成します
	 */
	class RoomStructureGenerator final : NonCopyable
	{
	public:
		explicit RoomStructureGenerator() = default;
		~RoomStructureGenerator() = default;

		/**
		 * 室内にスロープが生成できるか調べて生成可能な条件を記録します。
		 * GenerateSlopeを呼ぶとボクセルにスロープを生成します。
		 * @param voxel		ボクセル
		 * @param room		調べる部屋
		 * @param baseLocation	通路を生成する基準の位置（接続先の部屋の位置）
		 * @param random	乱数
		 * @return trueならば生成可能
		 */
		bool CheckSlopePlacement(const std::shared_ptr<Voxel>& voxel, const std::shared_ptr<Room>& room, const FIntVector& baseLocation, const std::shared_ptr<Random>& random);

		/**
		 * CheckSlopePlacementで調べた部屋の門になる位置を取得します
		 * @return 門になる位置
		 */
		FIntVector GetGateLocation() const noexcept;

		/**
		 * CheckSlopePlacementで調べた部屋にスロープを生成します
		 * @param voxel		ボクセル
		 */
		void GenerateSlope(const std::shared_ptr<Voxel>& voxel) const;

	private:
		enum class EffectiveDirection : uint8_t
		{
			HorizontalNortheast,
			HorizontalNorthwest,
			HorizontalSouthwest,
			HorizontalSoutheast,

			VerticalNortheast,
			VerticalNorthwest,
			VerticalSouthwest,
			VerticalSoutheast,
		};
		FIntVector mLocation;
		EffectiveDirection mEffectiveDirection;
		bool mValid = false;
	};
}
