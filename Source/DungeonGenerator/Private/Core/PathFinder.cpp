/*!
A*によるパス検索 ソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "PathFinder.h"
#include <cassert>

namespace dungeon
{
	PathFinder::BaseNode::BaseNode(const NodeType nodeType, const FIntVector& location, const Direction direction)
		: mLocation(location)
		, mNodeType(nodeType)
		, mDirection(direction)
	{
	}

	PathFinder::BaseNode::BaseNode(const BaseNode& other)
		: mLocation(other.mLocation)
		, mNodeType(other.mNodeType)
		, mDirection(other.mDirection)
	{
	}

	PathFinder::BaseNode::BaseNode(BaseNode&& other) noexcept
		: mLocation(std::move(other.mLocation))
		, mNodeType(std::move(other.mNodeType))
		, mDirection(std::move(other.mDirection))
	{
	}

	PathFinder::OpenNode::OpenNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const SearchDirection searchDirection, const uint32_t cost)
		: BaseNode(nodeType, location, direction)
		, mParentKey(parentKey)
		, mCost(cost)
		, mSearchDirection(searchDirection)
	{
	}

	PathFinder::OpenNode::OpenNode(const OpenNode& other)
		: BaseNode(other)
		, mParentKey(other.mParentKey)
		, mCost(other.mCost)
		, mSearchDirection(other.mSearchDirection)
	{
	}

	PathFinder::OpenNode::OpenNode(OpenNode&& other) noexcept
		: BaseNode(std::move(other))
		, mParentKey(std::move(other.mParentKey))
		, mCost(std::move(other.mCost))
		, mSearchDirection(std::move(other.mSearchDirection))
	{
	}

	PathFinder::CloseNode::CloseNode(const uint64_t parentKey, const NodeType nodeType, const FIntVector& location, const Direction direction, const uint32_t cost)
		: BaseNode(nodeType, location, direction)
		, mParentKey(parentKey)
		, mCost(cost)
	{
	}

	PathFinder::CloseNode::CloseNode(const CloseNode& other)
		: BaseNode(other)
		, mParentKey(other.mParentKey)
		, mCost(other.mCost)
	{
	}

	PathFinder::CloseNode::CloseNode(CloseNode&& other) noexcept
		: BaseNode(std::move(other))
		, mParentKey(std::move(other.mParentKey))
		, mCost(std::move(other.mCost))
	{
	}

	uint64_t PathFinder::Start(const NodeType nodeType, const FIntVector& location, const FIntVector& goal, const SearchDirection searchDirection)
	{
		// キーを生成
		const uint64_t parentKey = Hash(location);

		// Openリストを作成
		return Open(parentKey, nodeType, 0, location, goal, Direction(), searchDirection);
	}

	uint64_t PathFinder::Open(const uint64_t parentKey, const NodeType nodeType, const uint32_t cost, const FIntVector& location, const FIntVector& goal, const Direction direction, const SearchDirection searchDirection)
	{
		// キーを生成
		const uint64_t key = Hash(location);

		// コスト計算
		const uint32_t newCost = TotalCost(cost, location, goal);

		// Openリスト内を検索
		const auto openNode = mOpen.find(key);

		// Closeリスト内を検索
		const auto closeNode = mClose.find(key);

		// OpenとClose両方に存在する事はありえない
		check((openNode != mOpen.end() && closeNode != mClose.end()) == false);

		// オープンリストに追加するノードがある。かつ、新しいノードの方がトータルコストが低い
		if (openNode != mOpen.end())
		{
			if (openNode->second.mCost > newCost)
			{
				// Openリストを更新
				openNode->second.mNodeType = nodeType;
				openNode->second.mDirection = direction;
				openNode->second.mParentKey = parentKey;
				openNode->second.mSearchDirection = searchDirection;
				openNode->second.mCost = newCost;
			}
		}
		// クローズリストに追加するノードがある。かつ、新しいノードの方がトータルコストが低い
		else if (closeNode != mClose.end())
		{
			if (closeNode->second.mCost > newCost)
			{
				// Closeリストから消す
				mClose.erase(closeNode);
				// Openリストに再登録
				mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, newCost));
			}
		}
		// オープンとクローズリストに追加するノードがない
		else
		{
			// Openリストに登録
			mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, newCost));
		}

		return key;
	}

	bool PathFinder::Empty() const
	{
		return mOpen.empty();
	}

	bool PathFinder::Pop(uint64_t& key, NodeType& nodeType, uint32_t& cost, FIntVector& location, Direction& direction, SearchDirection& searchDirection)
	{
		if (mOpen.empty())
			return false;

		// 最もコストの低いノードを検索
		std::unordered_map<uint64_t, OpenNode>::iterator result;
		uint32_t minimumCost = std::numeric_limits<uint32_t>::max();
		for (std::unordered_map<uint64_t, OpenNode>::iterator i = mOpen.begin(); i != mOpen.end(); ++i)
		{
			if (minimumCost > (*i).second.mCost)
			{
				minimumCost = (*i).second.mCost;
				result = i;
			}
			// タイブレーク（コストが同じならば上下移動を優先）
			else if (minimumCost == (*i).second.mCost)
			{
				if ((*i).second.mNodeType == NodeType::Downstairs || (*i).second.mNodeType == NodeType::Upstairs)
				{
					result = i;
				}
			}
		}
		check(result != mOpen.end());

		// 結果をコピー
		key = result->first;
		location = result->second.mLocation;
		nodeType = result->second.mNodeType;
		direction = result->second.mDirection;
		cost = result->second.mCost;
		searchDirection = result->second.mSearchDirection;

		// Closeノードに追加
		mClose.emplace(key, CloseNode(result->second.mParentKey, nodeType, location, direction, result->second.mCost));

		// Openノードを削除
		mOpen.erase(result);

		return true;
	}

//#define CHECK_ROUTE

	bool PathFinder::Commit(const FIntVector& goal)
	{
		if (!mRoute.empty())
			return false;

#if !defined(CHECK_ROUTE)
		// Openノードは不要なのでクリア
		mOpen.clear();
#endif

		// ゴールノードを探すキーを生成
		const uint64_t key = Hash(goal);

		// ゴールからスタートを記録する（親ノードを手繰る）
		for (auto current = mClose.find(key); current != mClose.end(); current = mClose.find(current->second.mParentKey))
		{
			// 階段ノードの場合、斜面が22.5度に傾いているので4グリッド分確保する
			switch (current->second.mNodeType)
			{
			case NodeType::Downstairs:
				mRoute.emplace_back(NodeType::Space, current->second.mLocation + FIntVector(0, 0, 1), current->second.mDirection);
				mRoute.emplace_back(NodeType::Space, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;
			
			case NodeType::Upstairs:
				mRoute.emplace_back(NodeType::Space, current->second.mLocation + FIntVector(0, 0, -1), current->second.mDirection);
				mRoute.emplace_back(NodeType::Space, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;
			}

			// ノードを記録する
			mRoute.emplace_back(current->second);

			// スタートノード？
			if (current->first == current->second.mParentKey)
				break;
		}

		// 経路生成に失敗
		if (mRoute.empty())
			return false;

#if !defined(CHECK_ROUTE)
		// Closeノードは不要なのでクリア
		mClose.clear();
#endif

		// 階段ノードを修正
		for (auto current = mRoute.rbegin(); current != mRoute.rend(); ++current)
		{
			switch (current->mNodeType)
			{
			case NodeType::Downstairs:
			{
				const auto previous = current - 1;
				check(previous != mRoute.rend());
				check(previous->mNodeType == NodeType::Aisle || previous->mNodeType == NodeType::Gate);
				previous->mNodeType = NodeType::Space;
				break;
			}
			case NodeType::Upstairs:
			{
				const auto previous = current - 1;
				check(previous != mRoute.rend());
				check(previous->mNodeType == NodeType::Aisle || previous->mNodeType == NodeType::Gate);
				previous->mNodeType = NodeType::Upstairs;
				current->mNodeType = NodeType::Space;
				break;
			}
			default:
				break;
			}
		}

		return true;
	}

	bool PathFinder::IsValidPath() const
	{
		return mRoute.empty() == false;
	}

	uint64_t PathFinder::Hash(const FIntVector& location)
	{
		return
			static_cast<uint64_t>(location.Z) << 32 |
			static_cast<uint64_t>(location.Y) << 16 |
			static_cast<uint64_t>(location.X);
	}

	uint32_t PathFinder::TotalCost(const uint32_t cost, const FIntVector& location, const FIntVector& goal)
	{
		return TotalCost(cost, Heuristics(location, goal));
	}

	uint32_t PathFinder::TotalCost(const uint32_t cost, const uint32_t heuristics)
	{
		return cost + heuristics;
	}

	uint32_t PathFinder::Heuristics(const FIntVector& location, const FIntVector& goal)
	{
#if 1
		// マンハッタン距離
		auto dx = std::abs(goal.X - location.X);
		auto dy = std::abs(goal.Y - location.Y);
		auto dz = std::abs(goal.Z - location.Z);
		const uint32_t heuristics = dx + dy + dz;
#else
		// ユークリッド距離
		const FIntVector delta = goal - location;
		const int32_t heuristics = delta.X * delta.X + delta.Y * delta.Y + delta.Z * delta.Z;
#endif
		return heuristics;
	}

	Direction PathFinder::Cast(const SearchDirection direction)
	{
		check(static_cast<uint8_t>(SearchDirection::North) == static_cast<uint8_t>(Direction::North));
		check(static_cast<uint8_t>(SearchDirection::East) == static_cast<uint8_t>(Direction::East));
		check(static_cast<uint8_t>(SearchDirection::South) == static_cast<uint8_t>(Direction::South));
		check(static_cast<uint8_t>(SearchDirection::West) == static_cast<uint8_t>(Direction::West));
		check(direction != SearchDirection::Any);
		return Direction(static_cast<Direction::Index>(direction));
	}

	PathFinder::SearchDirection PathFinder::Cast(const Direction direction)
	{
		check(static_cast<uint8_t>(SearchDirection::North) == static_cast<uint8_t>(Direction::North));
		check(static_cast<uint8_t>(SearchDirection::East) == static_cast<uint8_t>(Direction::East));
		check(static_cast<uint8_t>(SearchDirection::South) == static_cast<uint8_t>(Direction::South));
		check(static_cast<uint8_t>(SearchDirection::West) == static_cast<uint8_t>(Direction::West));
		return static_cast<SearchDirection>(direction.Get());
	}
}
