/*
ゲート検索

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <vector>
#include <Math/IntVector.h>

namespace dungeon
{
	class GateFinder
	{
	public:
		struct Gate final
		{
			FIntVector mLocation;
			int64_t mSquaredDistance;

			Gate(const FIntVector& location, const int64_t squaredDistance);
		};

	public:
		GateFinder(const FIntVector& start, const FIntVector& goal);
		virtual ~GateFinder() = default;

		void Entry(const FIntVector& start, const FIntVector& goal);

		bool Pop(FIntVector& result);

	private:
		std::vector<Gate> mGates;
	};
}
