/**
方向

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline Direction::Direction(const Index index) noexcept
		: mIndex(index)
	{
	}

	inline const Direction::Index Direction::Get() const noexcept
	{
		return mIndex;
	}

	inline void Direction::Set(const Direction::Index index) noexcept
	{
		mIndex = index;
	}

	inline Direction& Direction::operator=(const Direction::Index index) noexcept
	{
		mIndex = index;
		return *this;
	}

	inline bool Direction::operator==(const Direction& other) const noexcept
	{
		return mIndex == other.mIndex;
	}

	inline bool Direction::operator!=(const Direction& other) const noexcept
	{
		return mIndex != other.mIndex;
	}

	inline bool Direction::IsNorthSouth() const noexcept
	{
		return IsNorthSouth(mIndex);
	}

	inline bool Direction::IsNorthSouth(const Index index) noexcept
	{
		const uint8_t isNorthSouth = static_cast<uint8_t>(index) & 1;
		return isNorthSouth == 0;
	}

	inline float Direction::ToDegree() const noexcept
	{
		return static_cast<float>(mIndex) * 90.f;
	}

	inline float Direction::ToRadian() const noexcept
	{
		return static_cast<float>(mIndex) * 1.5707963267948966192313216916398f;
	}

	inline Direction Direction::Inverse() const noexcept
	{
		Direction result(mIndex);
		result.SetInverse();
		return result;
	}

	inline void Direction::SetInverse() noexcept
	{
		std::uint_fast8_t value = static_cast<std::uint_fast8_t>(mIndex);
		value += 2;
		value &= 3;
		mIndex = static_cast<Index>(value);
	}

	inline const FIntVector& Direction::GetVector() const noexcept
	{
		return GetVector(mIndex);
	}

	inline Direction Direction::CreateFromRandom(Random& random) noexcept
	{
		return Direction(static_cast<Direction::Index>(random.Get<int32_t>() % 4));
	}
}
