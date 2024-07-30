/**
最小スパニングツリーに関するヘッダーファイル

@cite		https://algo-logic.info/kruskal-mst/
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline void MinimumSpanningTree::ForEach(std::function<void(Edge&)> func) noexcept
	{
		for (auto& node : mEdges)
		{
			func(node);
		}
	}

	inline void MinimumSpanningTree::ForEach(std::function<void(const Edge&)> func) const noexcept
	{
		for (auto& node : mEdges)
		{
			func(node);
		}
	}

	inline size_t MinimumSpanningTree::Size() const noexcept
	{
		return mEdges.size();
	}

	inline const std::shared_ptr<const Point>& MinimumSpanningTree::GetStartPoint() const noexcept
	{
		return mStartPoint;
	}

	inline const std::shared_ptr<const Point>& MinimumSpanningTree::GetGoalPoint() const noexcept
	{
		return mGoalPoint;
	}

	inline void MinimumSpanningTree::EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const noexcept
	{
		for (const auto& point : mLeafPoints)
		{
			func(point);
		}
	}

	inline uint8_t MinimumSpanningTree::GetDistance() const noexcept
	{
		return mDistance;
	}
}
