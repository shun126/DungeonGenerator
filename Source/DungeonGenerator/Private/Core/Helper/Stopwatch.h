/**
@brief	Stopwatch class header files

@file		Stopwatch.h
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <chrono>

namespace dungeon
{
	//! Measure time
	class Stopwatch
	{
	public:
		//! Start time measurement
		Stopwatch();

		//! Start time measurement
		void Start();

		//! Get elapsed time
		double Lap();

	private:
		std::chrono::system_clock::time_point mStartTime;
	};

	inline Stopwatch::Stopwatch()
	{
		Start();
	}

	inline void Stopwatch::Start()
	{
		mStartTime = std::chrono::system_clock::now();
	}

	inline double Stopwatch::Lap()
	{
		const auto now = std::chrono::system_clock::now();
		const auto delta = now - mStartTime;
		const auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
		mStartTime = now;
		return static_cast<double>(nano) / 1000000000.0;
	}
}
