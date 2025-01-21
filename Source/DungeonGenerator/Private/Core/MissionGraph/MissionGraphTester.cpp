/**
ミッショングラフが攻略可能かテストします

@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
 */

#include "MissionGraphTester.h"
#include "../RoomGeneration/Aisle.h"
#include "../Debug/Debug.h"

namespace dungeon
{
	MissionGraphTester::MissionGraphTester(const std::list<std::shared_ptr<Room>>& rooms, const std::vector<Aisle>& aisles)
	{
		// 例外を投げても良いがMissionGraphTesterはローカルデバッグ用なのでcheckにした
		check(std::find_if(rooms.begin(), rooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Goal;
			}) != rooms.end());

		const auto i = std::find_if(rooms.begin(), rooms.end(), [](const std::shared_ptr<Room>& room)
		{
			return room->GetParts() == Room::Parts::Start;
		});
		// 例外を投げても良いがMissionGraphTesterはローカルデバッグ用なのでcheckにした
		check(i != rooms.end());

		mRootNode = std::make_shared<Node>();
		mRootNode->mIdentifier = (*i)->GetIdentifier();
		mRootNode->mRoomParts = (*i)->GetParts();
		mRootNode->mRoomItem = (*i)->GetItem();

		InitializeParameter parameter(rooms, aisles);
		Initialize(mRootNode, *i, parameter);

		// 冒険を開始
		DUNGEON_GENERATOR_LOG(TEXT("Start a MissionGraph Test"));
		Inventory inventory;
		bool shouldRetry;
		do {
			shouldRetry = false;
			mResult = mRootNode->SeekGoal(inventory, shouldRetry);
		} while (mResult == false && shouldRetry == true);

		// 冒険の結果
		if (mResult)
		{
			if (inventory.mNumberOfKeys == 0)
			{
				DUNGEON_GENERATOR_LOG(TEXT("Test succeeded. The goal was reached."));
			}
			else
			{
				DUNGEON_GENERATOR_WARNING(TEXT("Test succeeded. The goal was reached. However, there are %d keys remaining."),
					inventory.mNumberOfKeys);
			}
		}
		else
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Test failure. The goal was not reached."));
		}
	}

	void MissionGraphTester::Initialize(const std::shared_ptr<Node>& parentNode, const std::shared_ptr<Room>& parentRoom, InitializeParameter& parameter)
	{
		for (const auto& aisle : parameter.mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (parentRoom == room0 || parentRoom == room1)
			{
				if (parameter.mPassableAisles.contains(&aisle) == true)
					continue;
				parameter.mPassableAisles.emplace(&aisle);

				// 通路ノードを作成
				auto newAisle = std::make_shared<Node>();
				newAisle->mIdentifier = aisle.GetIdentifier();
				if (aisle.IsUniqueLocked() == true)
					newAisle->mRoomKey = Room::Item::UniqueKey;
				else if (aisle.IsLocked() == true)
					newAisle->mRoomKey = Room::Item::Key;

				// 元の部屋に新しい通路を接続
				parentNode->mNodes.emplace_back(newAisle);

				// 部屋ノードを作成
				const std::shared_ptr<Room>& nextRoom = (parentRoom != room0) ? room0 : room1;
				auto nextNode = std::make_shared<Node>();
				nextNode->mIdentifier = nextRoom->GetIdentifier();
				nextNode->mRoomParts = nextRoom->GetParts();
				nextNode->mRoomItem = nextRoom->GetItem();

				// 通路に新しい部屋を接続
				newAisle->mNodes.emplace_back(nextNode);

				// 次の部屋を初期化
				Initialize(nextNode, nextRoom , parameter);
			}
		}
	}

	bool MissionGraphTester::Success() const
	{
		return mResult;
	}

	bool MissionGraphTester::Node::SeekGoal(Inventory& inventory, bool& shouldRetry) const
	{
		// ゴールに到着！
		if (mRoomParts == Room::Parts::Goal)
			return true;

		for (const auto& node : mNodes)
		{
			// ユニーク鍵がかかっている？
			if (node->mRoomKey == Room::Item::UniqueKey)
			{
				check(node->mRoomItem == Room::Item::Empty);

				// ユニーク鍵を持っている？
				if (inventory.mNumberOfUniqueKeys > 0)
				{
					// ユニーク鍵を消費して次の部屋へ
					--inventory.mNumberOfUniqueKeys;
					node->mRoomKey = Room::Item::Empty;
					shouldRetry = true;
					DUNGEON_GENERATOR_LOG(TEXT(" -> Used a unique key in Room %d. (%d|%d)"), static_cast<uint16_t>(node->mIdentifier), inventory.mNumberOfUniqueKeys, inventory.mNumberOfKeys);
					if (node->SeekGoal(inventory, shouldRetry) == true)
						return true;
				}
			}
			// 鍵がかかっている？
			else if (node->mRoomKey == Room::Item::Key)
			{
				check(node->mRoomItem == Room::Item::Empty);

				// 鍵を持っている？
				if (inventory.mNumberOfKeys > 0)
				{
					// 鍵を消費して次の部屋へ
					--inventory.mNumberOfKeys;
					node->mRoomKey = Room::Item::Empty;
					shouldRetry = true;
					DUNGEON_GENERATOR_LOG(TEXT(" -> Used a key in Room %d. (%d|%d)"), static_cast<uint16_t>(node->mIdentifier), inventory.mNumberOfUniqueKeys, inventory.mNumberOfKeys);
					if (node->SeekGoal(inventory, shouldRetry) == true)
						return true;
				}
			}
			else
			{
				check(node->mRoomKey == Room::Item::Empty);

				// ユニーク鍵を拾った？
				if (node->mRoomItem == Room::Item::UniqueKey)
				{
					++inventory.mNumberOfUniqueKeys;
					node->mRoomItem = Room::Item::Empty;
					shouldRetry = true;
					DUNGEON_GENERATOR_LOG(TEXT(" -> Picked up a unique key in Room %d. (%d|%d)"), static_cast<uint16_t>(node->mIdentifier), inventory.mNumberOfUniqueKeys, inventory.mNumberOfKeys);
				}
				// 鍵を拾った？
				else if (node->mRoomItem == Room::Item::Key)
				{
					++inventory.mNumberOfKeys;
					node->mRoomItem = Room::Item::Empty;
					shouldRetry = true;
					DUNGEON_GENERATOR_LOG(TEXT(" -> Picked up a key in Room %d. (%d|%d)"), static_cast<uint16_t>(node->mIdentifier), inventory.mNumberOfUniqueKeys, inventory.mNumberOfKeys);
				}

				// 次の部屋へ
				if (node->SeekGoal(inventory, shouldRetry) == true)
					return true;
			}
		}

		return false;
	}

}
