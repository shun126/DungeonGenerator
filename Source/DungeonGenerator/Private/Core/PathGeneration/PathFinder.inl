/**
A*によるパス検索 ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline bool PathFinder::Empty() const noexcept
	{
		return mOpen.empty();
	}

	inline size_t PathFinder::OpenSize() const noexcept
	{
		return mOpen.size();
	}

	inline size_t PathFinder::CloseSize() const noexcept
	{
		return mClose.size();
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

	inline const std::shared_ptr<PathFinder::Result>& PathFinder::GetResult() const noexcept
	{
		return mResult;
	}

	inline void PathFinder::ReserveOpenNode(const FIntVector& parentLocation, const PathNodeSwitcher::Node& openNode)
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

	/*
	BaseNode
	*/
	/**
	コンストラクタ
	@param[in]	nodeType		ノードの種類
	@param[in]	location		現在位置
	@param[in]	direction		検索してきた方向
	*/
	inline PathFinder::BaseNode::BaseNode(const NodeType nodeType, const FIntVector& location, const Direction& direction) noexcept
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

	/*
	OpenNode
	*/
	/**
	コンストラクタ
	@param[in]	parentKey		親ノードのキー
	@param[in]	nodeType		ノードの種類
	@param[in]	location		現在位置
	@param[in]	direction		検索してきた方向
	@param[in]	searchDirection	検索可能な方向
	@param[in]	cost			コスト
	*/
	inline PathFinder::OpenNode::OpenNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction& direction, const SearchDirection searchDirection, const uint32_t cost) noexcept
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

	/*
	CloseNode
	*/
	/**
	コンストラクタ
	@param[in]	parentKey		親ノードのキー
	@param[in]	nodeType		ノードの種類
	@param[in]	location		現在位置
	@param[in]	direction		検索してきた方向
	@param[in]	cost			コスト
	*/
	inline PathFinder::CloseNode::CloseNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction& direction, const uint32_t cost) noexcept
		: BaseNode(nodeType, location, direction)
		, mParentKey(parentKey)
		, mCost(cost)
	{
	}

	inline PathFinder::CloseNode::CloseNode(const CloseNode& other) noexcept
		: BaseNode(other)
		, mParentKey(other.mParentKey)
		, mCost(other.mCost)
	{
	}

	inline PathFinder::CloseNode::CloseNode(CloseNode&& other) noexcept
		: BaseNode(std::move(other))
		, mParentKey(std::move(other.mParentKey))
		, mCost(std::move(other.mCost))
	{
	}

	/*
	Result
	*/

	inline bool PathFinder::Result::IsValidPath() const noexcept
	{
		return mRoute.empty() == false;
	}

	inline size_t PathFinder::Result::GetPathLength() const noexcept
	{
		return mRoute.size();
	}

	inline Direction PathFinder::Result::GetStartDirection() const noexcept
	{
		return mRoute.back().mDirection;
	}

	inline Direction PathFinder::Result::GetGoalDirection() const noexcept
	{
		return mRoute.front().mDirection;
	}

	inline const FIntVector& PathFinder::Result::GetStartLocation() const noexcept
	{
		return mRoute.back().mLocation;
	}

	inline const FIntVector& PathFinder::Result::GetGoalLocation() const noexcept
	{
		return mRoute.front().mLocation;
	}

	inline PathFinder::NodeType PathFinder::Result::GetNodeTypeFromStart(const size_t index) const noexcept
	{
		return (mRoute.end() - 1 - index)->mNodeType;
	}

	inline PathFinder::NodeType PathFinder::Result::GetNodeTypeFromGoal(const size_t index) const noexcept
	{
		return (mRoute.begin() + index)->mNodeType;
	}

	inline void PathFinder::Result::InvalidateStartLocationType() noexcept
	{
		mRoute.back().mNodeType = NodeType::Invalid;
	}

	inline void PathFinder::Result::InvalidateGoalLocationType() noexcept
	{
		mRoute.front().mNodeType = NodeType::Invalid;
	}
}
