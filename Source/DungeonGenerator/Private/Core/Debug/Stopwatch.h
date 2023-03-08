#pragma once
#include <chrono>

class Stopwatch
{
public:
	Stopwatch()
	{
		Start();
	}

	void Start()
	{
		mStartTime = std::chrono::system_clock::now();
	}

	double Lap()
	{
		const auto now = std::chrono::system_clock::now();
		const auto delta = now - mStartTime;
		const auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count();
		mStartTime = now;
		return static_cast<double>(nano) / 1000000000.0;
	}

private:
	std::chrono::system_clock::time_point mStartTime;
};
