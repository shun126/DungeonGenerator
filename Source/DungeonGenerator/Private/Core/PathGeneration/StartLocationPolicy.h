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
	 * Dungeon start location policy
	 *
	 * スタート位置の種類
	 * EDungeonStartLocationPolicyと同じ意味にして下さい
	 */
	enum class StartLocationPolicy : uint8_t
	{
		NoAdjustment,
		UseSouthernMost,
		UseHighestPoint,
		UseLowestPoint,
		UseCentralPoint,
		UseMultiStart,
	};
}
