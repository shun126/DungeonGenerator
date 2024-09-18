/**
ミッショングラフが攻略可能かテストします

@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
 */

#pragma once
#include "../RoomGeneration/Room.h"
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>

namespace dungeon
{
	class Aisle;

	class MissionGraphTester final
	{
	public:
		MissionGraphTester(const std::list<std::shared_ptr<Room>>& rooms, const std::vector<Aisle>& aisles);
		~MissionGraphTester() = default;

		bool Success() const;

	private:
		struct InitializeParameter final
		{
			const std::list<std::shared_ptr<Room>>& mRooms;
			const std::vector<Aisle>& mAisles;
			std::unordered_set<const Aisle*> mPassableAisles;

			InitializeParameter(const std::list<std::shared_ptr<Room>>& rooms, const std::vector<Aisle>& aisles)
				: mRooms(rooms)
				, mAisles(aisles)
			{
			}
		};

		struct Inventory final
		{
			uint8_t mNumberOfKeys = 0;
			uint8_t mNumberOfUniqueKeys = 0;
		};

		struct Node final
		{
			Identifier mIdentifier;
			Room::Parts mRoomParts = Room::Parts::Unidentified;
			Room::Item mRoomKey = Room::Item::Empty;
			Room::Item mRoomItem = Room::Item::Empty;
			std::vector<std::shared_ptr<Node>> mNodes;

			bool SeekGoal(Inventory& inventory, bool& shouldRetry) const;
		};

		static void Initialize(const std::shared_ptr<Node>& parentNode, const std::shared_ptr<Room>& parentRoom, InitializeParameter& parameter);

	private:
		std::shared_ptr<Node> mRootNode;
		bool mResult = false;
	};
}
