/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <cstdint>
#include <ctime>
#include <limits>

namespace dungeon
{
	/*
	constexpr Template for delaying evaluation of blocks in if
	*/
	template <typename T>
	constexpr bool RandamFalse_v = false;

	inline Random::Random()
	{
		SetSeed(static_cast<uint32_t>(std::time(nullptr)));
	}

	inline Random::Random(const uint32_t seed)
	{
		SetSeed(seed);
	}

	inline Random::Random(const Random& other)
		: mX(other.mX)
		, mY(other.mY)
		, mZ(other.mZ)
		, mW(other.mW)
	{
	}

	inline Random::Random(Random&& other) noexcept
		: mX(std::move(other.mX))
		, mY(std::move(other.mY))
		, mZ(std::move(other.mZ))
		, mW(std::move(other.mW))
	{
	}

	inline Random& Random::operator = (const Random& other)
	{
		mX = other.mX;
		mY = other.mY;
		mZ = other.mZ;
		mW = other.mW;
		return *this;
	}

	inline Random& Random::operator = (Random&& other) noexcept
	{
		mX = std::move(other.mX);
		mY = std::move(other.mY);
		mZ = std::move(other.mZ);
		mW = std::move(other.mW);
		return *this;
	}

	inline void Random::GetSeeds(uint32_t& x, uint32_t& y, uint32_t& z, uint32_t& w) const noexcept
	{
		x = mX;
		y = mY;
		z = mZ;
		w = mW;
	}

	inline void Random::SetSeed(const uint32_t seed)
	{
		mX = 123456789;
		mY = 362436069;
		mZ = 521288629;
		mW = 88675123;

		// Make sure it doesn't all go to zero.
		do
		{
			mX ^= seed; mX ^= mX >> 21;	mX ^= mX >> 4; mX *= 1332534557;
			mY ^= seed; mY ^= mY >> 21;	mY ^= mY >> 4; mY *= 1332534557;
			mZ ^= seed; mZ ^= mZ >> 21;	mZ ^= mZ >> 4; mZ *= 1332534557;
			mW ^= seed; mW ^= mW >> 21;	mW ^= mW >> 4; mW *= 1332534557;
		} while (mX == 0 && mY == 0 && mZ == 0 && mW == 0);
	}

	template<typename T>
	inline T Random::GetSign()
	{
		static_assert(std::is_signed<T>::value == true, "T must be signed type");
		return static_cast<T>(GetU32() & 0x2) - 1;
	}

	template<typename T>
	inline T Random::Get()
	{
		if constexpr (std::is_same<T, bool>())
		{
			/*
			Unlike linear congruence, XorShift has no short - period regularity
			in the least significant bit. Implementation that takes the least
			significant bit because it is possible to obtain 0 or 1 directly.
			*/
			return static_cast<bool>(GetU32() & 0x1);
		}
		else if constexpr (std::is_same<T, double>())
		{
			static const double INVERT = 1. / static_cast<double>(0xFFFFFFFF);
			return static_cast<double>(GetU32()) * INVERT;
		}
		else if constexpr (std::is_same<T, float>())
		{
			static const float INVERT = 1.f / static_cast<float>(0xFFFFFFFF);
			return static_cast<float>(GetU32()) * INVERT;
		}
		else if constexpr (std::is_same<T, uint64_t>())
		{
			return GetU64();
		}
		else if constexpr (std::is_same<T, int64_t>())
		{
			return static_cast<int64_t>(GetU64());
		}
		else if constexpr (std::is_integral<T>::value)
		{
			return static_cast<T>(GetU32());
		}
		else
		{
			static_assert(RandamFalse_v<T>, "Type T is unsupported type");
			return 0;
		}
	}

	template <typename T>
	inline T Random::Get(const T to)
	{
		if constexpr (std::is_floating_point<T>::value)
		{
			const T value = Get<T>();
			return value * to;
		}
		else
		{
			if (to != 0)
			{
				const std::make_unsigned_t<T> value = Get<T>();
				return value % to;
			}
			else
			{
				return 0;
			}
		}
	}

	template <typename T>
	inline T Random::Get(const T from, const T to)
	{
		if constexpr (std::is_floating_point<T>::value)
		{
			const T value = Get<T>();
			return from + value * (to - from);
		}
		else
		{
			if (from != to)
			{
				const std::make_unsigned_t<T> value = Get<T>();
				return from + value % (to - from);
			}
			else
			{
				return from;
			}
		}
	}

	inline uint32_t Random::GetU32()
	{
		const uint32_t t = (mX ^ (mX << 11));
		mX = mY;
		mY = mZ;
		mZ = mW;
		mW = (mW ^ (mW >> 19)) ^ (t ^ (t >> 8));
		return mW;
	}

	inline uint64_t Random::GetU64()
	{
		const uint64_t upper = static_cast<uint64_t>(GetU32()) << 32ULL;
		const uint64_t lower = static_cast<uint64_t>(GetU32());
		return upper | lower;
	}
}
