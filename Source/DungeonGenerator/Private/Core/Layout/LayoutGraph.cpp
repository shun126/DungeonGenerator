/**
 * Intent-driven dungeon layout graph types.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "LayoutGraph.h"
#include <algorithm>

namespace dungeon
{
	RouteShapeProfile GetRouteShapeProfile(const EDungeonProgressionPolicy policy) noexcept
	{
		switch (policy)
		{
		case EDungeonProgressionPolicy::FreeExploration:
			return { 0.42f, 0.25f, 0.60f, 0.28f, 0.15f, 0.45f };
		case EDungeonProgressionPolicy::KeysAndLocks:
			return { 0.62f, 0.50f, 0.78f, 0.00f, 0.00f, 0.00f };
		case EDungeonProgressionPolicy::BossRoute:
			return { 0.72f, 0.58f, 0.90f, 0.05f, 0.00f, 0.12f };
		case EDungeonProgressionPolicy::HubQuest:
			return { 0.45f, 0.32f, 0.58f, 0.08f, 0.02f, 0.16f };
		case EDungeonProgressionPolicy::StartToGoal:
		default:
			return { 0.55f, 0.40f, 0.72f, 0.10f, 0.02f, 0.20f };
		}
	}

	float CalculateEffectiveMainRouteRatio(const FDungeonPathSettings& settings) noexcept
	{
		const RouteShapeProfile profile = GetRouteShapeProfile(settings.ProgressionPolicy);
		return std::clamp(
			profile.BaseMainRouteRatio + settings.MainRouteBias * MainRouteBiasInfluence,
			profile.MinEffectiveMainRouteRatio,
			profile.MaxEffectiveMainRouteRatio
		);
	}

	float CalculateEffectiveLoopRouteDensity(const FDungeonPathSettings& settings) noexcept
	{
		const RouteShapeProfile profile = GetRouteShapeProfile(settings.ProgressionPolicy);
		return std::clamp(
			profile.LoopRouteDensity + settings.LoopRouteDensity * LoopRouteDensityInfluence,
			profile.MinLoopRouteDensity,
			profile.MaxLoopRouteDensity
		);
	}
}
