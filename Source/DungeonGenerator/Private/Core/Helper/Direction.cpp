/**
方向

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Direction.h"

namespace dungeon
{
	namespace detail
	{
		/**
		方向
		Direction::IndexとPathFinder::SearchDirectionに並びを合わせて下さい
		*/
		static const std::array<FIntVector, 4> Offsets = {
			{
				{  0, -1,  0 },	// 北
				{  1,  0,  0 },	// 東
				{  0,  1,  0 },	// 南
				{ -1,  0,  0 },	// 西
			}
		};
	}

	const FIntVector& Direction::GetVector(const Direction::Index index) noexcept
	{
		return detail::Offsets[static_cast<size_t>(index)];
	}

	std::array<FIntVector, 4>::const_iterator Direction::Begin() noexcept
	{
		return detail::Offsets.cbegin();
	}

	std::array<FIntVector, 4>::const_iterator Direction::End() noexcept
	{
		return detail::Offsets.cend();
	}

#if WITH_EDITOR
	const FString& Direction::GetName() const noexcept
	{
		static const FString names[] = {
			TEXT("North"),
			TEXT("East"),
			TEXT("South"),
			TEXT("West")
		};
		return names[mIndex];
	}
#endif
}
