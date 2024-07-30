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
	MissionGraph::MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<const Point>& goal) noexcept
		: mGenerator(generator)
	{
		const auto room = goal->GetOwnerRoom();

		// Choose an aisle close to the entrance
		if (Aisle* aisle = SelectAisle(room))
		{
			// Locked. (to limit FindByRoute search scope)
			aisle->SetUniqueLock(true);

			const auto& room0 = aisle->GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle->GetPoint(1)->GetOwnerRoom();
			const auto& connectingRoom = room == room0 ? room1 : room0;

			// Gathering reachable rooms
			std::vector<std::shared_ptr<Room>> keyRooms;
			keyRooms = mGenerator->FindByRoute(connectingRoom);
			if (keyRooms.size() > 0)
			{
				const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();

				const uint8_t roomBranch = room->GetBranchId();
				const auto keyRoom = DrawLots(random, keyRooms.begin(), keyRooms.end(), [roomBranch](const std::shared_ptr<const Room>& room)
					{
						uint32_t weight = room->GetDepthFromStart();
						if (room->GetBranchId() - roomBranch)
							weight *= 3;
						if (room->GetParts() == Room::Parts::Hanare)
							weight *= 3;
						return weight;
					}
				);
				if (keyRoom != keyRooms.end())
				{
					check((*keyRoom)->GetItem() == Room::Item::Empty);
					(*keyRoom)->SetItem(Room::Item::UniqueKey);

					Generate(connectingRoom);
				}
				else
				{
					// It did not decide on a room to put the key, so it will be unlocked
					aisle->SetUniqueLock(false);
				}
			}
			else
			{
				// It did not decide on a room to put the key, so it will be unlocked
				aisle->SetUniqueLock(false);
			}
		}
	}

	void MissionGraph::Generate(const std::shared_ptr<const Room>& room) noexcept
	{
#if 0
		mGenerator->DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/dungeon_aisle.md");
#endif
		// Choose an aisle close to the entrance
		if (Aisle* aisle = SelectAisle(room))
		{
			// Locked. (to limit FindByRoute search scope)
			aisle->SetLock(true);

			const auto& room0 = aisle->GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle->GetPoint(1)->GetOwnerRoom();

			// Gathering reachable rooms
			std::vector<std::shared_ptr<Room>> keyRooms;
			keyRooms = mGenerator->FindByRoute(room == room0 ? room1 : room0);
			if (keyRooms.size() > 0)
			{
				const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();

				// That's the room where I'm supposed to put the key.
				{
					const size_t index = random->Get<size_t>(keyRooms.size());
					const auto keyRoom = keyRooms[index];
					check(keyRoom->GetItem() == Room::Item::Empty);
					keyRoom->SetItem(Room::Item::Key);
				}

				const auto lockRoom = DrawLots(random, keyRooms.begin(), keyRooms.end(), [](const std::shared_ptr<const Room>& room)
					{
						const uint32_t depthFromStart = room->GetDepthFromStart();
						return depthFromStart * 10;
					}
				);
				if (lockRoom != keyRooms.end())
				{
					Generate(*lockRoom);
				}
			}
			else
			{
				// It did not decide on a room to put the key, so it will be unlocked
				aisle->SetLock(false);
			}
		}
	}

	/*
	Choose an aisle close to the entrance.
	入り口に近い通路を選ぶ。
	*/
	Aisle* MissionGraph::SelectAisle(const std::shared_ptr<const Room>& room) const noexcept
	{
		const uint8_t roomDepth = room->GetDepthFromStart();

		std::vector<Aisle*> aisles;
		mGenerator->FindAisle(room, [&aisles, roomDepth](Aisle& edge)
			{
				if (!edge.IsLocked())
				{
					const auto& room0 = edge.GetPoint(0)->GetOwnerRoom();
					const auto& room1 = edge.GetPoint(1)->GetOwnerRoom();
					if (room0->GetDepthFromStart() <= roomDepth && room1->GetDepthFromStart() <= roomDepth)
						aisles.emplace_back(&edge);
				}
				return false;
			}
		);

		if (aisles.size() <= 0)
		{
			return nullptr;
		}
		else if (aisles.size() == 1)
		{
			return aisles[0];
		}
		else
		{
			const std::shared_ptr<Random>& random = mGenerator->GetGenerateParameter().GetRandom();
			const size_t index = random->Get(aisles.size());
			return aisles[index];
		}
	}
}
