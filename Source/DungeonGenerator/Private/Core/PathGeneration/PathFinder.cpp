/**
A*によるパス検索 ソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "PathFinder.h"
#include "../Debug/BuildInformation.h"
#include <CoreMinimal.h>

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
// 定義すると経路を調べるため、中間データを開放しない
//#define CHECK_ROUTE
#endif

namespace dungeon
{
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

		// これまでのコストとゴールまでの予想コストを合算
		const uint32_t totalCost = TotalCost(cost, location, goal);

		// オープンリスト内を検索
		const auto openNode = mOpen.find(key);

		// オープンリストに追加するノードがある。かつ、新しいノードの方がトータルコストが低い
		if (openNode != mOpen.end())
		{
			if (openNode->second.mCost > totalCost)
			{
				// Replace node with open list
				openNode->second.mNodeType = nodeType;
				openNode->second.mDirection = direction;
				openNode->second.mParentKey = parentKey;
				openNode->second.mSearchDirection = searchDirection;
				openNode->second.mCost = totalCost;
			}
		}
		// クローズリストに追加するノードがある。かつ、新しいノードの方がトータルコストが低い
		else
		{
			// クローズリスト内を検索
			const auto closeNode = mClose.find(key);

			// オープンとクローズ両方に存在する事はありえない
			check((openNode != mOpen.end() && closeNode != mClose.end()) == false);

			if (closeNode != mClose.end())
			{
				if (closeNode->second.mCost > totalCost)
				{
					// Delete from close list
					mClose.erase(closeNode);
					// Re-register on open list
					mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, totalCost));

					// 使用中のOpenノードを予約中に変更します
					RevertOpenNode(key);
				}
			}
			// No nodes on open and closed list
			else
			{
				// Register open List
				mOpen.emplace(key, OpenNode(parentKey, nodeType, location, direction, searchDirection, totalCost));
			}
		}

		return key;
	}

	bool PathFinder::Pop(uint64_t& nextKey, NodeType& nextNodeType, uint32_t& nextCost, FIntVector& nextLocation, Direction& nextDirection, SearchDirection& nextSearchDirection) noexcept
	{
		if (mOpen.empty())
			return false;

		// 最も安いコストのノードを探す
		auto result = mOpen.begin();
		uint32_t minimumCost = std::numeric_limits<uint32_t>::max();
		for (auto i = mOpen.begin(); i != mOpen.end(); ++i)
		{
			if (minimumCost > i->second.mCost)
			{
				minimumCost = i->second.mCost;
				result = i;
			}
#if 0
			// コストが同じ場合、上下移動を優先する
			else if (minimumCost == i->second.mCost)
			{
				if (i->second.mNodeType == NodeType::Downstairs || i->second.mNodeType == NodeType::Upstairs)
				{
					result = i;
				}
			}
#endif
		}
		check(result != mOpen.end());

		// 結果をコピー
		nextKey = result->first;
		nextLocation = result->second.mLocation;
		nextNodeType = result->second.mNodeType;
		nextDirection = result->second.mDirection;
		nextCost = result->second.mCost;
		nextSearchDirection = result->second.mSearchDirection;

		// Closeノードに追加
		mClose.emplace(nextKey, CloseNode(result->second.mParentKey, nextNodeType, nextLocation, nextDirection, result->second.mCost));

		// Openノードを削除
		mOpen.erase(result);

		// Openノードを使用中に指定
		UseOpenNode(nextKey);

		return true;
	}

	bool PathFinder::Commit(const FIntVector& goal) noexcept
	{
		// 使用中と予約中のOpenノードをクリアします
		ClearOpenNode();

#if defined(CHECK_ROUTE)
		// Openノードは不要なのでクリア
		mOpen.clear();
#endif

		// 結果オブジェクトを生成
		mResult = std::make_shared<Result>();

		// ゴールノードを探すキーを生成
		const uint64_t key = Hash(goal);

		// ゴールからスタートを記録する（親ノードを手繰る）
		for (auto current = mClose.find(key); current != mClose.end(); current = mClose.find(current->second.mParentKey))
		{
			// 階段ノードの場合、斜面が22.5度に傾いているので4グリッド分確保する
			switch (current->second.mNodeType)
			{
			case NodeType::Downstairs:
				mResult->mRoute.emplace_back(NodeType::UpSpace, current->second.mLocation + FIntVector(0, 0, 1), current->second.mDirection);
				mResult->mRoute.emplace_back(NodeType::DownSpace, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;
			
			case NodeType::Upstairs:
				mResult->mRoute.emplace_back(NodeType::DownSpace, current->second.mLocation + FIntVector(0, 0, -1), current->second.mDirection);
				mResult->mRoute.emplace_back(NodeType::UpSpace, current->second.mLocation - current->second.mDirection.GetVector(), current->second.mDirection);
				break;

			default:
				break;
			}

			// ゴールノードなら必ずノードは門に変更する
			if (current->first == key)
			{
				current->second.mNodeType = NodeType::Gate;
			}

			// ノードを記録する
			mResult->mRoute.emplace_back(current->second);

			// スタートノード？
			if (current->first == current->second.mParentKey)
			{
				/*
				スタートノードの方向は仮に北を設定していたので、
				ここで修正する
				*/
				if (mResult->mRoute.size() > 1)
				{
					const Direction previousDirection = mResult->mRoute[mResult->mRoute.size() - 2].mDirection;
					mResult->mRoute[mResult->mRoute.size() - 1].mDirection = previousDirection;
				}

				break;
			}
		}

		// 経路生成に失敗
		if (mResult->mRoute.empty())
			return false;

#if defined(CHECK_ROUTE)
		// Closeノードは不要なのでクリア
		mClose.clear();
#endif

		// 階段ノードを修正
		// TODO: このブロックをResultクラスに含められるか検討して下さい
		for (auto current = mResult->mRoute.rbegin(); current != mResult->mRoute.rend(); ++current)
		{
			switch (current->mNodeType)
			{
			case NodeType::Downstairs:
			{
				check(current != mResult->mRoute.rbegin());
				check((current + 1) != mResult->mRoute.rend());
				check((current + 2) != mResult->mRoute.rend());
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
				check(current != mResult->mRoute.rbegin());
				check((current + 1) != mResult->mRoute.rend());
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
		/*
		 * マンハッタン距離をベースにスロープのルールにあわせて
		 * 上下移動に迂回コストを加算している
		 * 斜めの移動が無いのでユークリッド距離は使用しない
		 */
		const uint32_t dx = std::abs(goal.X - location.X);
		const uint32_t dy = std::abs(goal.Y - location.Y);
		const uint32_t dz = std::abs(goal.Z - location.Z) * 2;
		const uint32_t heuristics = dx + dy + dz;
		return heuristics;
	}

	double PathFinder::Heuristics(const FVector& location, const FVector& goal) noexcept
	{
		/*
		 * マンハッタン距離をベースにスロープのルールにあわせて
		 * 上下移動に迂回コストを加算している
		 * 斜めの移動が無いのでユークリッド距離は使用しない
		 */
		const double dx = std::abs(goal.X - location.X);
		const double dy = std::abs(goal.Y - location.Y);
		const double dz = std::abs(goal.Z - location.Z) * 2.0;
		const double heuristics = dx + dy + dz;
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

	PathFinder::SearchDirection PathFinder::Cast(const Direction& direction) noexcept
	{
		check(static_cast<uint8_t>(SearchDirection::North) == static_cast<uint8_t>(Direction::North));
		check(static_cast<uint8_t>(SearchDirection::East) == static_cast<uint8_t>(Direction::East));
		check(static_cast<uint8_t>(SearchDirection::South) == static_cast<uint8_t>(Direction::South));
		check(static_cast<uint8_t>(SearchDirection::West) == static_cast<uint8_t>(Direction::West));
		return static_cast<SearchDirection>(direction.Get());
	}

	FIntVector2 PathFinder::Result::ComputeLongestStraightPath() const noexcept
	{
		FIntVector2 longestStraight = { 0, 0 };
		if (!mRoute.empty())
		{
			auto current = mRoute.rbegin();
			auto lastIsNorthSouth = current->mDirection.IsNorthSouth();
			++current;

			int32 straight = 1;
			while (current != mRoute.rend())
			{
				const auto isNorthSouth = current->mDirection.IsNorthSouth();
				if (lastIsNorthSouth == isNorthSouth)
				{
					++straight;
				}
				else
				{
					if (lastIsNorthSouth)
					{
						if (longestStraight.Y < straight)
							longestStraight.Y = straight;
					}
					else
					{
						if (longestStraight.X < straight)
							longestStraight.X = straight;
					}
					straight = 1;
					lastIsNorthSouth = isNorthSouth;
				}
				++current;
			}
			if (lastIsNorthSouth)
			{
				if (longestStraight.Y < straight)
					longestStraight.Y = straight;
			}
			else
			{
				if (longestStraight.X < straight)
					longestStraight.X = straight;
			}
		}
		return longestStraight;
	}
}
