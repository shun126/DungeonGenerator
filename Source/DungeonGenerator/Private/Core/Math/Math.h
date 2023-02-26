/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <algorithm>

namespace dungeon
{
	namespace math
	{
		template<typename T = double>
		inline T Square(const T value) noexcept
		{
			return value * value;
		}

		template<typename T = double>
		T Clamp(const T value, const T min, const T max) noexcept
		{
			return std::max(min, std::min(value, max));
		}
	}
}
