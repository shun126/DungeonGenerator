/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	//! Generates random numbers by xorshift32
	class Random
	{
	public:
		//! Default constructor
		Random();

		/**
		Constructor
		\param[in]	seed	乱数の種
		*/
		explicit Random(const uint32_t seed);

		/**
		copy constructor
		*/
		explicit Random(const Random& other);

		/**
		move constructor
		*/
		explicit Random(Random&& other) noexcept;

		/**
		copy assignment
		*/
		Random& operator = (const Random& other);

		/**
		move assignment
		*/
		Random& operator = (Random&& other) noexcept;

		/**
		Sets the random number seed
		\param[in]	seed	Random number seeds
		*/
		void SetSeed(const uint32_t seed);

		/**
		\return	-1 or 1
		T must be a signed type.
		*/
		template <typename T>
		T GetSign();

		/**
		Get a random number
		\return		Returns the range [type_min,type_max) if T is an integer,
					or [0,1] with equal probability if T is a real number.
		*/
		template <typename T>
		T Get();

		/**
		Get a random number
		\param[in]	to	Upper value
		\return		Returns the range [0,to) if T is an integer,
					or [0,to] with equal probability if T is a real number.
		*/
		template <typename T>
		T Get(const T to);

		/**
		Get a random number
		\param[in]	from	Lower value
		\param[in]	to		Upper value
		\return		Returns the range [from,to) if T is an integer,
					or [from,to] with equal probability if T is a real number.
		*/
		template <typename T>
		T Get(const T from, const T to);

	private:
		/**
		Get a random number of type uint32_t
		\return		Returns a range of [0,std::numeric_limits<int32_t>::max
		*/
		uint32_t GetU32();

	private:
		uint32_t mX = 123456789;
		uint32_t mY = 362436069;
		uint32_t mZ = 521288629;
		uint32_t mW = 88675123;
	};
}

#include "Random.inl"
