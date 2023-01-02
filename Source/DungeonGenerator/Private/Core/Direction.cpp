/*!
方向

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Direction.h"

namespace dungeon
{
	namespace detail
	{
		/*!
		方向
		Direction::IndexとPathFinder::SearchDirectionに並びを合わせて下さい
		*/
		static const std::array<FIntVector, 4> offsets = {
			{
				{  0, -1,  0 },	// 北
				{  1,  0,  0 },	// 東
				{  0,  1,  0 },	// 南
				{ -1,  0,  0 },	// 西
			}
		};
	}

	const FIntVector& Direction::GetVector(const Direction::Index index)
	{
		return detail::offsets[static_cast<size_t>(index)];
	}

	std::array<FIntVector, 4>::const_iterator Direction::Begin()
	{
		return detail::offsets.cbegin();
	}

	std::array<FIntVector, 4>::const_iterator Direction::End()
	{
		return detail::offsets.cend();
	}
}
