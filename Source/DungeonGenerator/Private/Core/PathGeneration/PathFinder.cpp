/**
A*によるパス検索 ソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "PathFinder.h"
#include "../Debug/BuildInfomation.h"
#include <cassert>

// 定義するとマンハッタン距離で計算する。未定義ならユークリッド距離で計算する
#define CALCULATE_IN_MANHATTAN_DISTANCE

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
// 定義すると経路を調べるため、中間データを開放しない
//#define CHECK_ROUTE
#endif

namespace dungeon
{
	PathFinder::CloseNode::CloseNode(const CloseNode& other) noexcept
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

	uint64_t PathFinder::Start(const FIntVector& location, const FIntVector& goal, const SearchDirection searchDirection) noexcept
	{
		// キーを生成
		const uint64_t parentKey = Hash(location);

		// Openリストを作成
		return Open(parentKey, NodeType::Gate, 0, location, goal, Direction(), searchDirection);
	}

	uint64_t PathFinder::Open(const uint64_t parentKey, const NodeType nodeType, const uint32_t cost, const FIntVector& location, const FIntVector& goal, const Direction direction, const SearchDirection searchDirection) noexcept
	{
		// キーを生成
		const uint64_t key = Hash(location);

		// コスト計算
		const uint32_t newCost = TotalCost(cost, location, goal);

		// オープンリスト内を検索
		const auto openNode = mOpen.find(key);

		// クローズリスト内を検索
		const auto closeNode = mClose.find(key);

		// オープンとクローズ両方に存在する事はありえない
		check((openNode != mOpen.end() && closeNode != mClose.end()) == false);

		// オープンリストに追加するノードがある。かつ、新しいノードの方がトータルコストが低い
		if (openNode != mOpen.end())
		{
			if (openNode->second.mCost > newCost)
			{
				// Replace node with open list
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
				// Delete from close list
				mClose.erase(closeNode);
				// Re-register on open list
				mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, newCost));

				RevertOpenNode(key);
			}
		}
		// No nodes on open and closed list
		else
		{
			// Register open List
			mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, newCost));
		}

		return key;
	}

	bool PathFinder::Pop(uint64_t& key, NodeType& nodeType, uint32_t& cost, FIntVector& location, Direction& direction, SearchDirection& searchDirection) noexcept
	{
		if (mOpen.empty())
			return false;

		// Find the node with the lowest cost
		std::unordered_map<uint64_t, OpenNode>::iterator result = mOpen.begin();
		uint32_t minimumCost = std::numeric_limits<uint32_t>::max();
		for (std::unordered_map<uint64_t, OpenNode>::iterator i = mOpen.begin(); i != mOpen.end(); ++i)
		{
			if (minimumCost > (*i).second.mCost)
			{
				minimumCost = (*i).second.mCost;
				result = i;
			}
			// Tiebreaker (if costs are the same, up/down movement is preferred)
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

		UseOpenNode(key);

		return true;
	}

//#define CHECK_ROUTE

	bool PathFinder::Commit(const FIntVector& goal) noexcept
	{
		ClearOpenNode();

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
				mRoute.emplace_back(NodeType::UpSpace, current->second.mLocation + FIntVector(0, 0, 1), current->second.mDirection);
				mRoute.emplace_back(NodeType::DownSpace, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;
			
			case NodeType::Upstairs:
				mRoute.emplace_back(NodeType::DownSpace, current->second.mLocation + FIntVector(0, 0, -1), current->second.mDirection);
				mRoute.emplace_back(NodeType::UpSpace, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;
			}

			// ゴールノードなら必ずノードは門に変更する
			if (current->first == key)
			{
				current->second.mNodeType = NodeType::Gate;
			}

			// ノードを記録する
			mRoute.emplace_back(current->second);

			// スタートノード？
			if (current->first == current->second.mParentKey)
			{
				/*
				スタートノードの方向は仮に北を設定していたので、
				ここで修正する
				*/
				if (mRoute.size() > 1)
				{
					mRoute[mRoute.size() - 1].mDirection = mRoute[mRoute.size() - 2].mDirection;
				}
				break;
			}
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
				check(current != mRoute.rbegin());
				check((current + 1) != mRoute.rend());
				check((current + 2) != mRoute.rend());
				check((current - 1)->mNodeType == NodeType::Aisle || (current - 1)->mNodeType == NodeType::Gate);
				(current - 1)->mNodeType = NodeType::UpSpace;
				(current + 2)->mNodeType = NodeType::Stairwell;
				(current - 1)->mDirection.SetInverse();
				(current + 0)->mDirection.SetInverse();
				(current + 1)->mDirection.SetInverse();
				(current + 2)->mDirection.SetInverse();
				break;
			}
			case NodeType::Upstairs:
			{
				check(current != mRoute.rbegin());
				check((current + 1) != mRoute.rend());
				check((current - 1)->mNodeType == NodeType::Aisle || (current - 1)->mNodeType == NodeType::Gate);
				(current - 1)->mNodeType = NodeType::Upstairs;
				(current + 0)->mNodeType = NodeType::UpSpace;
				(current + 1)->mNodeType = NodeType::Stairwell;
				break;
			}
			default:
				break;
			}
		}

		return true;
	}

	uint32_t PathFinder::Heuristics(const FIntVector& location, const FIntVector& goal) noexcept
	{
#if defined(CALCULATE_IN_MANHATTAN_DISTANCE)
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

	Direction PathFinder::Cast(const SearchDirection direction) noexcept
	{
		check(static_cast<uint8_t>(SearchDirection::North) == static_cast<uint8_t>(Direction::North));
		check(static_cast<uint8_t>(SearchDirection::East) == static_cast<uint8_t>(Direction::East));
		check(static_cast<uint8_t>(SearchDirection::South) == static_cast<uint8_t>(Direction::South));
		check(static_cast<uint8_t>(SearchDirection::West) == static_cast<uint8_t>(Direction::West));
		check(direction != SearchDirection::Any);
		return Direction(static_cast<Direction::Index>(direction));
	}

	PathFinder::SearchDirection PathFinder::Cast(const Direction direction) noexcept
	{
		check(static_cast<uint8_t>(SearchDirection::North) == static_cast<uint8_t>(Direction::North));
		check(static_cast<uint8_t>(SearchDirection::East) == static_cast<uint8_t>(Direction::East));
		check(static_cast<uint8_t>(SearchDirection::South) == static_cast<uint8_t>(Direction::South));
		check(static_cast<uint8_t>(SearchDirection::West) == static_cast<uint8_t>(Direction::West));
		return static_cast<SearchDirection>(direction.Get());
	}
}
