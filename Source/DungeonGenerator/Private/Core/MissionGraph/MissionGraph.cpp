/*
MissionGraph
Generate dungeon strategy information
ダンジョンの攻略情報を生成します

Algorithm for generating keys and doors
1. set a goal door in the passage connecting to the goal room
2. collect reachable aisle and rooms separated by doors
3. place a key (or goal key) somewhere in the reachable room
4. place a door somewhere in the reachable aisle
5. if more keys and doors can be placed, go back to 2.

鍵と扉の生成アルゴリズム
1. ゴール部屋に接続する通路にゴール扉を設定する
2. 扉で仕切られた到達可能な部屋と通路を集める
3. 到達可能な部屋のどこかに鍵（またはゴール鍵）を置く
4. 到達可能な通路のどこかに扉を置く
5. さらに鍵と扉が置けるなら2へ戻る

敵配置の生成アルゴリズム

部屋の装飾アルゴリズム

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MissionGraph.h"
#include "../Generator.h"
#include "../Helper/DrawLots.h"
#include <vector>

namespace dungeon
{
	MissionGraph::MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<Room>& startRoom, const std::shared_ptr<Room>& goalRoom, const uint8_t maxKeyCount) noexcept
		: mGenerator(generator)
	{
		// 入り口に近い通路を選ぶ
		if (Aisle* aisle = SelectNearestStartAisle(goalRoom))
		{
			// ユニーク鍵をかける
			aisle->SetUniqueLock(true);

			const auto& room0 = aisle->GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle->GetPoint(1)->GetOwnerRoom();
			const auto& connectingRoom = goalRoom == room0 ? room1 : room0;

			// 連絡可能な部屋を集める
			const auto keyRooms = mGenerator->FindByRoute(connectingRoom);
			if (keyRooms.size() > 0)
			{
				const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();

				// 鍵を置く部屋を抽選
				const uint8_t roomBranch = goalRoom->GetBranchId();
				const auto keyRoom = DrawLots(random, keyRooms.begin(), keyRooms.end(), [roomBranch](const std::shared_ptr<const Room>& room) -> uint32_t
					{
						return DetermineUniqueKeyPlacementProbability(roomBranch, room);
					}
				);
				if (keyRoom != keyRooms.end())
				{
					// ユニーク鍵を置く
					check((*keyRoom)->GetItem() == Room::Item::Empty);
					check((*keyRoom)->IsValidReservationNumber() == false);
					(*keyRoom)->SetItem(Room::Item::UniqueKey);

					// 鍵を置く
					GenerateParameter parameter(startRoom, maxKeyCount);
					Generate(parameter);
				}
				else
				{
					// ユニーク鍵を置く部屋が決まらなかったので鍵を開ける
					aisle->SetUniqueLock(false);
				}
			}
			else
			{
				// ユニーク鍵を置く部屋が決まらなかったので鍵を開ける
				aisle->SetUniqueLock(false);
			}
		}
	}

	void MissionGraph::Generate(GenerateParameter& parameter) noexcept
	{
		// スタートから到達可能な部屋を集める
		auto keyRooms = mGenerator->FindByRoute(parameter.mStartRoom);
		const auto eraseKeyRoom = std::remove_if(keyRooms.begin(), keyRooms.end(), [](const std::shared_ptr<Room>& room)
		{
			// はなれを除外
			return room->GetParts() == Room::Parts::Hanare;
		});
		keyRooms.erase(eraseKeyRoom, keyRooms.end());
		if (keyRooms.empty() == false)
		{
			// 鍵を置く部屋を抽選
			const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();
			const auto keyRoom = keyRooms[random->Get(keyRooms.size())];

			// 鍵の部屋よりも奥の部屋に鍵をかける
			std::vector<Aisle*> lockAisles;
			CollectDeepAisle(lockAisles, keyRoom);
			if (lockAisles.empty() == false)
			{
				// 鍵を置く
				check((keyRoom)->GetItem() == Room::Item::Empty);
				check((keyRoom)->IsValidReservationNumber() == false);
				(keyRoom)->SetItem(Room::Item::Key);

				// 鍵をかける
				const auto lockAisle = lockAisles[random->Get(lockAisles.size())];
				lockAisle->SetLock(true);

				// 配置した鍵の数を加算
				++parameter.mKeyCount;
				if (parameter.mKeyCount < parameter.mMaxKeyCount)
					Generate(parameter);
			}
		}
	}

	/*
	Choose an aisle close to the entrance.
	入り口に近い通路を選ぶ。
	*/
	Aisle* MissionGraph::SelectNearestStartAisle(const std::shared_ptr<const Room>& room) const noexcept
	{
		const uint8_t roomDepth = room->GetDepthFromStart();

		std::vector<Aisle*> aisles;
		mGenerator->FindAisle(room, [&aisles, roomDepth](const Aisle& edge)
			{
				if (!edge.IsLocked())
				{
					const auto& room0 = edge.GetPoint(0)->GetOwnerRoom();
					const auto& room1 = edge.GetPoint(1)->GetOwnerRoom();
					if (room0->GetDepthFromStart() <= roomDepth && room1->GetDepthFromStart() <= roomDepth)
						aisles.emplace_back(const_cast<Aisle*>(&edge));
				}
				return false;
			}
		);

		if (aisles.size() <= 0)
		{
			return nullptr;
		}
		if (aisles.size() == 1)
		{
			return aisles[0];
		}

		const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();
		const size_t index = random->Get(aisles.size());
		return aisles[index];
	}

	void MissionGraph::CollectDeepAisle(std::vector<Aisle*>& aisles, const std::shared_ptr<const Room>& room) const noexcept
	{
		const uint8_t roomDepth = room->GetDepthFromStart();
		mGenerator->FindAisle(room, [&aisles, roomDepth](const Aisle& edge)
			{
				if (!edge.IsLocked())
				{
					const auto& room0 = edge.GetPoint(0)->GetOwnerRoom();
					const auto& room1 = edge.GetPoint(1)->GetOwnerRoom();
					if (room0->GetDepthFromStart() >= roomDepth && room1->GetDepthFromStart() >= roomDepth)
						aisles.emplace_back(const_cast<Aisle*>(&edge));
				}
				return false;
			}
		);
	}

	uint32_t MissionGraph::DetermineUniqueKeyPlacementProbability(const uint8_t branchId, const std::shared_ptr<const Room>& room) noexcept
	{
		// 予約済みの部屋はアイテムを置く事ができない
		check(room->IsValidReservationNumber() == false);

		// 違う経路ほど優先
		uint32_t weight = 1;
		if (room->GetBranchId() != branchId)
			++weight;

		// 奥の部屋ほど優先
		weight *= room->GetDepthFromStart();

		// はなれの部屋ほど優先
		if (room->GetParts() == Room::Parts::Hanare)
			weight *= 10;

		return weight;
	}
}
