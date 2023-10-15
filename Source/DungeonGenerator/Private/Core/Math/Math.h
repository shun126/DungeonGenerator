/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <algorithm>
#include <limits>

namespace dungeon
{
	//! commonly used calculations Implementation 
	namespace math
	{
		template<typename T = double>
		static constexpr T Pi() noexcept
		{
			return static_cast<T>(3.14159265358979323846264338327950288);
		}

		template<typename T = double>
		static constexpr T PiHalf() noexcept
		{
			return Pi<T>() * static_cast<T>(0.5);
		}

		template<typename T = double>
		static constexpr T Pi2() noexcept
		{
			return Pi<T>() * static_cast<T>(2.);
		}

		template<typename T = double>
		static constexpr T Negative(const T t) noexcept
		{
			return -t;
		}

		template<typename T = double>
		static constexpr T Inverse(const T t) noexcept
		{
			return static_cast<T>(1.) / t;
		}

		template<typename T = double>
		static constexpr T Sign(const T t) noexcept
		{
			return t >= static_cast<T>(0.) ? static_cast<T>(1.) : static_cast<T>(-1.);
		}

		template<typename T = double>
		static constexpr T Square(const T value) noexcept
		{
			return value * value;
		}

		template<typename T = double>
		static constexpr T Clamp(const T value, const T min, const T max) noexcept
		{
			return std::max(min, std::min(value, max));
		}

		template<typename T = double>
		static constexpr bool Equal(const T a, const T b, const T eps = std::numeric_limits<T>::epsilon()) noexcept
		{
			return IsZero(a - b, eps);
		}

		template<typename T = double>
		static constexpr bool IsZero(const T x, const T eps = std::numeric_limits<T>::epsilon()) noexcept
		{
			return (-eps <= x && x <= eps);
		}

		template<typename T = double>
		static constexpr float Lerp(const T a, const T b, const T ratio)
		{
			return a + (b - a) * ratio;
		}

		template<typename T = double>
		static constexpr T ToRadian(const T degree)
		{
			return degree * (Pi<T>() / static_cast<T>(180.));
		}

		template<typename T = double>
		static constexpr T ToDegree(const T radian)
		{
			return radian * (static_cast<T>(180.0) / Pi<T>());
		}
	}
}
