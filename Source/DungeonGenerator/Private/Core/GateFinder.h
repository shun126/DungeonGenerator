/*
ゲート検索

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <memory>
#include <vector>
#include <Math/IntVector.h>

namespace dungeon
{
	/**
	@addtogroup PathGeneration
	@{
	*/
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
		std::vector<std::shared_ptr<const Gate>> mOpenGates;
		std::vector<std::shared_ptr<const Gate>> mCloseGates;
	};
	/**
	@}
	*/
}
