/*
ゲート検索

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "GateFinder.h"

namespace dungeon
{
	GateFinder::Gate::Gate(const FIntVector& location, const int64_t squaredDistance)
		: mLocation(location)
		, mSquaredDistance(squaredDistance)
	{
	}

	GateFinder::GateFinder(const FIntVector& start, const FIntVector& goal)
	{
		Entry(start, goal);
	}

	void GateFinder::Entry(const FIntVector& start, const FIntVector& goal)
	{
		const FIntVector delta = goal - start;
		const int64_t squaredLength =
			static_cast<int64_t>(delta.X * delta.X) +
			static_cast<int64_t>(delta.Y * delta.Y) +
			static_cast<int64_t>(delta.Z * delta.Z);
		mGates.emplace_back(start, squaredLength);
	}

	bool GateFinder::Pop(FIntVector& result)
	{
		std::vector<Gate>::iterator nearestGate = mGates.begin();
		int64_t nearestDistance = std::numeric_limits<int64_t>::max();

		for (std::vector<Gate>::iterator i = mGates.begin(); i != mGates.end(); ++i)
		{
			if (nearestDistance > (*i).mSquaredDistance)
			{
				nearestDistance = (*i).mSquaredDistance;
				nearestGate = i;
			}
		}

		if (nearestGate == mGates.end())
			return false;

		result = (*nearestGate).mLocation;

		mGates.erase(nearestGate);

		return true;
	}
}
