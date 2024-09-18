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
}
