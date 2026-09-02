/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <limits>

namespace dungeon::player_start
{
	/**
	 * Selects NearestHorizontalTarget.
	 * 水平距離が最も近い候補位置を選択します。
	 */
	inline bool SelectNearestHorizontalTarget(FVector& result, const FVector& playerLocation, const TArray<FVector>& candidateLocations) noexcept
	{
		double nearestDistanceSquared = std::numeric_limits<double>::max();
		bool found = false;
		for (const FVector& candidateLocation : candidateLocations)
		{
			const double distanceSquared = FVector::DistSquaredXY(playerLocation, candidateLocation);
			if (distanceSquared < nearestDistanceSquared)
			{
				nearestDistanceSquared = distanceSquared;
				result = candidateLocation;
				found = true;
			}
		}
		return found;
	}
}
