/**
 * Aisle planner for intent-driven dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "LayoutGraph.h"

namespace dungeon
{
	/*
	 * Converts graph edges into concrete aisle objects.
	 * グラフエッジを具体的な通路オブジェクトへ変換します。
	 */
	class AislePlanner final
	{
	public:
		static bool Plan(const LayoutGraph& graph, std::list<std::shared_ptr<Room>>& rooms, std::vector<Aisle>& aisles, std::shared_ptr<const Point>& startPoint, std::shared_ptr<const Point>& goalPoint) noexcept;

	private:
		static void ApplyRoomParts(const LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& indexedRooms) noexcept;
	};
}
