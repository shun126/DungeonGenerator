/*
MissionGraph
Generate dungeon strategy information

Algorithm for generating keys and doors
1. set a goal door in the passage connecting to the goal room
2. collect reachable aisle and rooms separated by doors
3. place a key (or goal key) somewhere in the reachable room
4. place a door somewhere in the reachable aisle
5. if more keys and doors can be placed, go back to 2.

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <memory>
#include <vector>

namespace dungeon
{
	class Aisle;
	class Generator;
	class Point;
	class Room;

	/*
	Generate dungeon strategy information
	*/
	class MissionGraph
	{
	public:
		// constructor
		MissionGraph(
			const std::shared_ptr<Generator>& generator,
			const std::shared_ptr<Room>& startRoom,
			const std::shared_ptr<Room>& goalRoom,
			const uint8_t maxKeyCount) noexcept;

		// destructor
		virtual ~MissionGraph() = default;

	private:
		/*
		Generate関数パラメータ
		*/
		struct GenerateParameter final
		{
			const std::shared_ptr<Room>& mStartRoom;
			uint8_t mMaxKeyCount;
			uint8_t mKeyCount = 0;

			GenerateParameter(const std::shared_ptr<Room>& startRoom, const uint8_t maxKeyCount)
				: mStartRoom(startRoom)
				, mMaxKeyCount(maxKeyCount)
			{
			}
		};

		// generate mission graph
		void Generate(GenerateParameter& parameter) noexcept;

		
		Aisle* SelectNearestStartAisle(const std::shared_ptr<const Room>& room) const noexcept;
		void CollectDeepAisle(std::vector<Aisle*>& aisles, const std::shared_ptr<const Room>& room) const noexcept;

		static uint32_t DetermineUniqueKeyPlacementProbability(const uint8_t branchId, const std::shared_ptr<const Room>& room) noexcept;

	private:
		std::shared_ptr<Generator> mGenerator;
	};
}
