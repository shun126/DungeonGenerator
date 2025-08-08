/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <algorithm>
#include <cmath>
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

		/**
		角度を0～360に収めます
		\param[in]	degree	角度
		\return		0～360
		*/
		template<typename T>
		static constexpr T RoundDegree(const T degree)
		{
			// https://stackoverflow.com/questions/11498169/dealing-with-angle-wrap-in-c-code
			if (degree > static_cast<T>(0.))
				return degree - static_cast<T>(360.) * std::floor(degree * static_cast<T>(1. / 360.));
			else
				return degree + static_cast<T>(360.) * std::floor(degree * static_cast<T>(1. / -360.));
		}

		/**
		ラジアン角を0～2πに収めます
		\param[in]	radian	ラジアン角
		\return		0～2π
		*/
		template<typename T>
		static constexpr T RoundRadian(const T radian)
		{
			// https://stackoverflow.com/questions/11498169/dealing-with-angle-wrap-in-c-code
			if (radian > static_cast<T>(0.))
				return radian - Pi2<T>() * std::floor(radian * (static_cast<T>(1.) / Pi2<T>()));
			else
				return radian + Pi2<T>() * std::floor(radian * (static_cast<T>(1.) / -Pi2<T>()));
		}

		/**
		角度を-180～180に収めます
		\param[in]	degree	角度
		\return		-180～180
		*/
		template<typename T>
		static constexpr T RoundDegreeWithSign(const T degree)
		{
			// https://stackoverflow.com/questions/11498169/dealing-with-angle-wrap-in-c-code
			if (degree > static_cast<T>(0.))
				return degree - static_cast<T>(360.) * std::floor((degree + static_cast<T>(180.)) * static_cast<T>(1. / 360.));
			else
				return degree + static_cast<T>(360.) * std::floor((degree - static_cast<T>(180.)) * static_cast<T>(1. / -360.));
		}

		/**
		ラジアン角を-π～πに収めます
		\param[in]	radian	ラジアン角
		\return		-π～π
		*/
		template<typename T>
		static constexpr T RoundRadianWithSign(const T radian)
		{
			// https://stackoverflow.com/questions/11498169/dealing-with-angle-wrap-in-c-code
			if (radian > static_cast<T>(0.))
				return radian - Pi2<T>() * std::floor((radian + Pi<T>()) * (static_cast<T>(1.) / Pi2<T>()));
			else
				return radian + Pi2<T>() * std::floor((radian - Pi<T>()) * (static_cast<T>(1.) / -Pi2<T>()));
		}

		/**
		角度の距離を取得します
		\param[in]	a	角度
		\param[in]	b	角度
		\return			絶対差
		*/
		template<typename T>
		static constexpr T DegreeDistance(const T a, const T b)
		{
			return std::abs(RoundDegreeWithSign(a - b));
		}

		/**
		ラジアン角の距離を取得します
		\param[in]	a	ラジアン角
		\param[in]	b	ラジアン角
		\return			絶対差
		*/
		template<typename T>
		static constexpr T RadianDistance(const T a, const T b)
		{
			return std::abs(RoundRadianWithSign(a - b));
		}
	}
}
