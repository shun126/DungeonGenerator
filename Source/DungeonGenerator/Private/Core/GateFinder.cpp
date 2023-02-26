/*
ゲート検索

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
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
		const auto closeGate = std::find_if(mCloseGates.begin(), mCloseGates.end(), [&start](const std::shared_ptr<const Gate>& gate)
			{
				return start == gate->mLocation;
			}
		);
		if (closeGate != mCloseGates.end())
			return;
		
		const auto openGate = std::find_if(mOpenGates.begin(), mOpenGates.end(), [&start](const std::shared_ptr<const Gate>& gate)
			{
				return start == gate->mLocation;
			}
		);
		if (openGate != mOpenGates.end())
			return;

		const FIntVector delta = goal - start;
		const int64_t squaredLength =
			static_cast<int64_t>(delta.X * delta.X) +
			static_cast<int64_t>(delta.Y * delta.Y) +
			static_cast<int64_t>(delta.Z * delta.Z);
		mOpenGates.emplace_back(std::make_shared<const Gate>(start, squaredLength));
	}

	bool GateFinder::Pop(FIntVector& result)
	{
		auto nearestGate = mOpenGates.begin();
		int64_t nearestDistance = std::numeric_limits<int64_t>::max();

		for (std::vector<std::shared_ptr<const Gate>>::iterator i = mOpenGates.begin(); i != mOpenGates.end(); ++i)
		{
			if (nearestDistance > (*i)->mSquaredDistance)
			{
				nearestDistance = (*i)->mSquaredDistance;
				nearestGate = i;
			}
		}

		if (nearestGate == mOpenGates.end())
			return false;

		result = (*nearestGate)->mLocation;

		mCloseGates.push_back(*nearestGate);
		mOpenGates.erase(nearestGate);

		return true;
	}
}
