/*
MissionGraph
Generate dungeon strategy information

Algorithm for generating keys and doors
1. set a goal door in the passage connecting to the goal room
2. collect reachable aisle and rooms separated by doors
3. place a key (or goal key) somewhere in the reachable room
4. place a door somewhere in the reachable aisle
5. if more keys and doors can be placed, go back to 2.

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <memory>

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
		MissionGraph(const std::shared_ptr<Generator>& generator, const std::shared_ptr<const Point>& goal) noexcept;

		// destructor
		virtual ~MissionGraph() = default;

	private:
		// generate mission graph
		void Generate(const std::shared_ptr<const Room>& room) noexcept;

		// choose an aisle close to the entrance.
		Aisle* SelectAisle(const std::shared_ptr<const Room>& room) const noexcept;

	private:
		std::shared_ptr<Generator> mGenerator;
	};
}
