/*
MissionGraph
ダンジョンの攻略情報を生成します

鍵と扉の生成アルゴリズム
1. ゴール部屋に接続する通路にゴール鍵を設定する
2. 通路に到達可能な部屋のどこかにゴール鍵を置く（別ブランチは抽選の優先度を上げる）
3. ゴール部屋よりも浅い深度の通路に鍵を設定する
4. 通路に到達可能な部屋のどこかに鍵を置く（別ブランチは抽選の優先度を上げる）
5. 深度1より探索深度が不快なら3に戻る

敵配置の生成アルゴリズム

部屋の装飾アルゴリズム

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MissionGraph.h"
#include "../Generator.h"
#include "../DrawLots.h"
#include <vector>

namespace dungeon
{
	MissionGraph::MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<const Point>& goal) noexcept
		: mGenerator(generator)
	{
		Generate(goal->GetOwnerRoom());
	}

	/*
	TODO:乱数を共通化して下さい
	*/
	void MissionGraph::Generate(const std::shared_ptr<const Room>& room, const uint8_t count) noexcept
	{
#if 0
		{
			const FString path = FPaths::ProjectSavedDir() + TEXT("/dungeon_aisle.txt");
			mGenerator->DumpAisle(TCHAR_TO_UTF8(*path));
		}
#endif

		if (Aisle* aisle = SelectAisle(room))
		{
			if (count == 0)
				aisle->SetUniqueLock(true);
			else
				aisle->SetLock(true);

			const size_t halfRoomCount = mGenerator->GetRoomCount() / 2;
			const uint8_t roomBranch = room->GetBranchId();
			const uint8_t roomDepth = room->GetDepthFromStart();
			const auto& room0 = aisle->GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle->GetPoint(1)->GetOwnerRoom();

			std::vector<std::shared_ptr<Room>> keyRooms;
			keyRooms = mGenerator->FindByRoute(room == room0 ? room1 : room0);
			if (keyRooms.size() > 0)
			{
				// TODO:DrawLots内で乱数を使っています
				const auto keyRoom = DrawLots(keyRooms.begin(), keyRooms.end(), [halfRoomCount, roomBranch, roomDepth](const std::shared_ptr<const Room>& room)
					{
						const auto deltaBranch = std::abs(roomBranch - room->GetBranchId());
						const auto addition = room->GetParts() == Room::Parts::Hanare ? halfRoomCount : 0;
						return deltaBranch + room->GetDepthFromStart() + addition;
					}
				);
				if (keyRoom != keyRooms.end())
				{
					// 鍵を置く部屋が決まった
					check((*keyRoom)->GetItem() == Room::Item::Empty);
					(*keyRoom)->SetItem(count == 0 ? Room::Item::UniqueKey : Room::Item::Key);
				}
				else
				{
					// 鍵を置く部屋が決まらなかった
					if (count == 0)
						aisle->SetUniqueLock(false);
					else
						aisle->SetLock(false);
				}

				// 次の鍵
				if (roomDepth >= 3)
				{
					const auto& depths = mGenerator->FindByDepth(roomDepth - 2);
					const auto index = std::rand() % depths.size();
					Generate(depths[index], count + 1);
				}
			}
			else
			{
				if (count == 0)
					aisle->SetUniqueLock(false);
				else
					aisle->SetLock(false);
			}
		}
	}

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
					if (room0->GetDepthFromStart() <= roomDepth || room1->GetDepthFromStart() <= roomDepth)
						aisles.emplace_back(&edge);
				}
				return false;
			}
		);

		const size_t size = aisles.size();
		return size > 0 ? aisles[0 % size] : nullptr;
	}

	Aisle* MissionGraph::SelectAisle(const std::shared_ptr<const Point>& point) const noexcept
	{
		return SelectAisle(point->GetOwnerRoom());
	}
}
