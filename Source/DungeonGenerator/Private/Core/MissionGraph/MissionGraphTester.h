/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "../RoomGeneration/Room.h"
#include <list>
#include <memory>
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
		bool mResult = false;
	};
}
