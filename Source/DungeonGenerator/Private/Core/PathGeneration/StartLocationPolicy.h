/**
 * ダンジョンの開始位置に関する定義
 *
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <cstdint>

namespace dungeon
{
	/**
	 * Defines how start and goal endpoint rooms are selected.
	 * 開始部屋とゴール部屋をどの基準で選ぶかを定義します。
	 * EDungeonStartLocationPolicyと同じ順番にして下さい
	 */
	enum class StartLocationPolicy : uint8_t
	{
		UseNorthernMost,
		UseEasternMost,
		UseWesternMost,
		UseSouthernMost,
		UseHighestPoint,
		UseLowestPoint,
		UseCentralPoint,
		UseMultiStart,
	};
}
