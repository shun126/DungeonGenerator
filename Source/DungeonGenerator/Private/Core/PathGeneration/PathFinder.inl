/**
A*によるパス検索 ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline PathFinder::BaseNode::BaseNode(const NodeType nodeType, const FIntVector& location, const Direction direction) noexcept
		: mLocation(location)
		, mNodeType(nodeType)
		, mDirection(direction)
	{
	}

	inline PathFinder::BaseNode::BaseNode(const BaseNode& other) noexcept
		: mLocation(other.mLocation)
		, mNodeType(other.mNodeType)
		, mDirection(other.mDirection)
	{
	}

	inline PathFinder::BaseNode::BaseNode(BaseNode&& other) noexcept
		: mLocation(std::move(other.mLocation))
		, mNodeType(std::move(other.mNodeType))
		, mDirection(std::move(other.mDirection))
	{
	}

	inline PathFinder::OpenNode::OpenNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const SearchDirection searchDirection, const uint32_t cost) noexcept
		: BaseNode(nodeType, location, direction)
		, mParentKey(parentKey)
		, mCost(cost)
		, mSearchDirection(searchDirection)
	{
	}

	inline PathFinder::OpenNode::OpenNode(const OpenNode& other) noexcept
		: BaseNode(other)
		, mParentKey(other.mParentKey)
		, mCost(other.mCost)
		, mSearchDirection(other.mSearchDirection)
	{
	}

	inline PathFinder::OpenNode::OpenNode(OpenNode&& other) noexcept
		: BaseNode(std::move(other))
		, mParentKey(std::move(other.mParentKey))
		, mCost(std::move(other.mCost))
		, mSearchDirection(std::move(other.mSearchDirection))
	{
	}

	inline PathFinder::CloseNode::CloseNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const uint32_t cost) noexcept
		: BaseNode(nodeType, location, direction)
		, mParentKey(parentKey)
		, mCost(cost)
	{
	}

	inline bool PathFinder::Empty() const noexcept
	{
		return mOpen.empty();
	}

	inline uint64_t PathFinder::Hash(const FIntVector& location) noexcept
	{
		return
			static_cast<uint64_t>(location.Z) << 44 |
			static_cast<uint64_t>(location.Y) << 22 |
			static_cast<uint64_t>(location.X);
	}

	inline uint32_t PathFinder::TotalCost(const uint32_t cost, const FIntVector& location, const FIntVector& goal) noexcept
	{
		return TotalCost(cost, Heuristics(location, goal));
	}

	inline uint32_t PathFinder::TotalCost(const uint32_t cost, const uint32_t heuristics) noexcept
	{
		return cost + heuristics;
	}

	inline bool PathFinder::IsValidPath() const noexcept
	{
		return mRoute.empty() == false;
	}

	inline size_t PathFinder::GetPathLength() const noexcept
	{
		return mRoute.size();
	}

	inline void PathFinder::ReserveOpenNode(const FIntVector& parentLocation, const std::shared_ptr<PathNodeSwitcher::Node>& openNode)
	{
		const uint64_t parentHash = Hash(parentLocation);
		// 親ノードがオープンリストにあるなら、進入禁止予約リストに登録
		mNoEntryNodeSwitcher.Reserve(parentHash, openNode);
		// クローズリストにあるなら、進入禁止リストに登録
	}

	inline bool PathFinder::IsUsingOpenNode(const FIntVector& location) const
	{
		const uint64_t key = Hash(location);
		return mNoEntryNodeSwitcher.IsUsing(key);
	}

	inline void PathFinder::UseOpenNode(const uint64_t parentHash)
	{
		mNoEntryNodeSwitcher.Use(parentHash);
	}

	inline void PathFinder::RevertOpenNode(const uint64_t parentHash)
	{
		mNoEntryNodeSwitcher.Revert(parentHash);
	}

	inline void PathFinder::ClearOpenNode()
	{
		mNoEntryNodeSwitcher.Clear();
	}

	inline size_t PathFinder::GetCloseNodeSize() const noexcept
	{
		return mClose.size();
	}
}
