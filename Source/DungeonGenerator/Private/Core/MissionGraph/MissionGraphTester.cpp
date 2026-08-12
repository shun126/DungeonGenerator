/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Tests whether a MissionGraph is solvable for every legal lock-opening order.
 * すべての合法な開錠順序でMissionGraphを攻略できるかテストします。
 */

#include "MissionGraphTester.h"
#include "../Debug/Debug.h"
#include "../RoomGeneration/Aisle.h"
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <vector>

namespace dungeon
{
	namespace
	{
		constexpr size_t MaxCommonLockCount = 16;
		constexpr size_t MaxLockCount = MaxCommonLockCount + 1;

		enum class LockType : uint8_t
		{
			None,
			Common,
			Unique,
		};

		struct RoomData final
		{
			Room::Parts Parts = Room::Parts::Unidentified;
			Room::Item Item = Room::Item::Empty;
		};

		struct AisleData final
		{
			size_t Room0 = 0;
			size_t Room1 = 0;
			LockType Lock = LockType::None;
		};

		struct MissionGraphData final
		{
			std::vector<RoomData> Rooms;
			std::vector<AisleData> Aisles;
			size_t StartRoom = 0;
			size_t GoalRoom = 0;
			size_t CommonLockCount = 0;
			size_t UniqueLockCount = 0;
		};

		struct RegionData final
		{
			uint8_t CommonKeys = 0;
			uint8_t UniqueKeys = 0;
		};

		struct RegionAisleData final
		{
			uint8_t Region0 = 0;
			uint8_t Region1 = 0;
			LockType Lock = LockType::None;
			uint32_t Bit = 0;
		};

		struct RegionGraphData final
		{
			std::vector<RegionData> Regions;
			std::vector<RegionAisleData> Aisles;
			uint8_t StartRegion = 0;
			uint8_t GoalRegion = 0;
			uint32_t CommonLockMask = 0;
			uint32_t UniqueLockMask = 0;
			uint32_t AllLockMask = 0;
		};

		class DisjointSet final
		{
		public:
			explicit DisjointSet(const size_t count)
				: mParents(count)
				, mRanks(count, 0)
			{
				for (size_t index = 0; index < count; ++index)
				{
					mParents[index] = index;
				}
			}

			size_t Find(const size_t index) noexcept
			{
				if (mParents[index] != index)
				{
					mParents[index] = Find(mParents[index]);
				}
				return mParents[index];
			}

			void Merge(const size_t index0, const size_t index1) noexcept
			{
				size_t root0 = Find(index0);
				size_t root1 = Find(index1);
				if (root0 == root1)
				{
					return;
				}

				if (mRanks[root0] < mRanks[root1])
				{
					std::swap(root0, root1);
				}
				mParents[root1] = root0;
				if (mRanks[root0] == mRanks[root1])
				{
					++mRanks[root0];
				}
			}

		private:
			std::vector<size_t> mParents;
			std::vector<uint8_t> mRanks;
		};

		bool BuildMissionGraphData(
			const std::list<std::shared_ptr<Room>>& rooms,
			const std::vector<Aisle>& aisles,
			MissionGraphData& outGraph) noexcept
		{
			std::unordered_map<Identifier, size_t> roomIndices;
			outGraph.Rooms.reserve(rooms.size());
			size_t startRoomCount = 0;
			size_t goalRoomCount = 0;
			for (const std::shared_ptr<Room>& room : rooms)
			{
				if (!room)
				{
					return false;
				}

				RoomData data;
				data.Parts = room->GetParts();
				data.Item = room->GetItem();

				const size_t roomIndex = outGraph.Rooms.size();
				if (!roomIndices.emplace(room->GetIdentifier(), roomIndex).second)
				{
					return false;
				}
				outGraph.Rooms.emplace_back(data);

				if (data.Parts == Room::Parts::Start)
				{
					outGraph.StartRoom = roomIndex;
					++startRoomCount;
				}
				else if (data.Parts == Room::Parts::Goal)
				{
					outGraph.GoalRoom = roomIndex;
					++goalRoomCount;
				}
			}

			if (startRoomCount != 1 || goalRoomCount != 1)
			{
				return false;
			}

			outGraph.Aisles.reserve(aisles.size());
			for (const Aisle& aisle : aisles)
			{
				const auto& point0 = aisle.GetPoint(0);
				const auto& point1 = aisle.GetPoint(1);
				if (!point0 || !point1)
				{
					return false;
				}

				const auto& room0 = point0->GetOwnerRoom();
				const auto& room1 = point1->GetOwnerRoom();
				if (!room0 || !room1)
				{
					return false;
				}

				const auto room0Index = roomIndices.find(room0->GetIdentifier());
				const auto room1Index = roomIndices.find(room1->GetIdentifier());
				if (room0Index == roomIndices.end() || room1Index == roomIndices.end() || room0Index->second == room1Index->second)
				{
					return false;
				}

				AisleData data;
				data.Room0 = room0Index->second;
				data.Room1 = room1Index->second;
				if (aisle.IsUniqueLocked())
				{
					data.Lock = LockType::Unique;
					++outGraph.UniqueLockCount;
				}
				else if (aisle.IsLocked())
				{
					data.Lock = LockType::Common;
					++outGraph.CommonLockCount;
				}
				outGraph.Aisles.emplace_back(data);
			}

			return true;
		}

		bool ValidateKeyLockCounts(const MissionGraphData& graph) noexcept
		{
			size_t commonKeyCount = 0;
			size_t uniqueKeyCount = 0;
			for (const RoomData& room : graph.Rooms)
			{
				if (room.Item == Room::Item::Key)
				{
					++commonKeyCount;
				}
				else if (room.Item == Room::Item::UniqueKey)
				{
					++uniqueKeyCount;
				}
			}

			return
				graph.CommonLockCount <= MaxCommonLockCount &&
				commonKeyCount == graph.CommonLockCount &&
				uniqueKeyCount == 1 &&
				graph.UniqueLockCount == 1;
		}

		bool BuildRegionGraph(const MissionGraphData& graph, RegionGraphData& outGraph) noexcept
		{
			DisjointSet connectedRooms(graph.Rooms.size());
			DisjointSet unlockedRegions(graph.Rooms.size());
			for (const AisleData& aisle : graph.Aisles)
			{
				connectedRooms.Merge(aisle.Room0, aisle.Room1);
				if (aisle.Lock == LockType::None)
				{
					unlockedRegions.Merge(aisle.Room0, aisle.Room1);
				}
			}

			const size_t connectedRoot = connectedRooms.Find(0);
			for (size_t roomIndex = 1; roomIndex < graph.Rooms.size(); ++roomIndex)
			{
				if (connectedRooms.Find(roomIndex) != connectedRoot)
				{
					return false;
				}
			}

			const size_t invalidRegion = std::numeric_limits<size_t>::max();
			std::vector<size_t> rootToRegion(graph.Rooms.size(), invalidRegion);
			std::vector<uint8_t> roomToRegion(graph.Rooms.size(), 0);
			for (size_t roomIndex = 0; roomIndex < graph.Rooms.size(); ++roomIndex)
			{
				const size_t root = unlockedRegions.Find(roomIndex);
				if (rootToRegion[root] == invalidRegion)
				{
					rootToRegion[root] = outGraph.Regions.size();
					outGraph.Regions.emplace_back();
				}
				if (rootToRegion[root] >= 32)
				{
					return false;
				}

				const uint8_t regionIndex = static_cast<uint8_t>(rootToRegion[root]);
				roomToRegion[roomIndex] = regionIndex;
				RegionData& region = outGraph.Regions[regionIndex];
				if (graph.Rooms[roomIndex].Item == Room::Item::Key)
				{
					++region.CommonKeys;
				}
				else if (graph.Rooms[roomIndex].Item == Room::Item::UniqueKey)
				{
					++region.UniqueKeys;
				}
			}
			outGraph.StartRegion = roomToRegion[graph.StartRoom];
			outGraph.GoalRegion = roomToRegion[graph.GoalRoom];

			outGraph.Aisles.reserve(graph.CommonLockCount + graph.UniqueLockCount);
			for (const AisleData& aisle : graph.Aisles)
			{
				if (aisle.Lock == LockType::None)
				{
					continue;
				}
				if (outGraph.Aisles.size() >= MaxLockCount)
				{
					return false;
				}

				RegionAisleData regionAisle;
				regionAisle.Region0 = roomToRegion[aisle.Room0];
				regionAisle.Region1 = roomToRegion[aisle.Room1];
				regionAisle.Lock = aisle.Lock;
				regionAisle.Bit = uint32_t{ 1 } << outGraph.Aisles.size();
				if (regionAisle.Region0 == regionAisle.Region1)
				{
					return false;
				}

				if (aisle.Lock == LockType::Common)
				{
					outGraph.CommonLockMask |= regionAisle.Bit;
				}
				else
				{
					outGraph.UniqueLockMask |= regionAisle.Bit;
				}
				outGraph.AllLockMask |= regionAisle.Bit;
				outGraph.Aisles.emplace_back(regionAisle);
			}

			return outGraph.Regions.size() <= MaxLockCount + 1;
		}

		uint8_t CountBits(uint32_t bits) noexcept
		{
			uint8_t count = 0;
			while (bits != 0)
			{
				bits &= bits - 1;
				++count;
			}
			return count;
		}

		bool CanSolveMissionGraphForEveryOrder(const RegionGraphData& graph)
		{
			const size_t stateCapacity = size_t{ 1 } << graph.Aisles.size();
			std::vector<uint8_t> visited(stateCapacity, 0);
			std::vector<uint32_t> reachableRegionsByState(stateCapacity, 0);
			std::vector<uint8_t> collectedCommonKeysByState(stateCapacity, 0);
			std::vector<uint8_t> collectedUniqueKeysByState(stateCapacity, 0);
			std::vector<uint32_t> queue;
			queue.reserve(stateCapacity);
			visited[0] = 1;
			reachableRegionsByState[0] = uint32_t{ 1 } << graph.StartRegion;
			collectedCommonKeysByState[0] = graph.Regions[graph.StartRegion].CommonKeys;
			collectedUniqueKeysByState[0] = graph.Regions[graph.StartRegion].UniqueKeys;
			queue.emplace_back(0);
			bool reachedSuccess = false;

			for (size_t queueIndex = 0; queueIndex < queue.size(); ++queueIndex)
			{
				const uint32_t openedLocks = queue[queueIndex];
				const uint32_t reachableRegions = reachableRegionsByState[openedLocks];
				const uint8_t collectedCommonKeys = collectedCommonKeysByState[openedLocks];
				const uint8_t collectedUniqueKeys = collectedUniqueKeysByState[openedLocks];
				const bool goalReached = (reachableRegions & (uint32_t{ 1 } << graph.GoalRegion)) != 0;

				const uint8_t openedCommonLocks = CountBits(openedLocks & graph.CommonLockMask);
				const uint8_t openedUniqueLocks = CountBits(openedLocks & graph.UniqueLockMask);
				if (collectedCommonKeys < openedCommonLocks || collectedUniqueKeys < openedUniqueLocks)
				{
					return false;
				}
				const uint16_t commonKeys = collectedCommonKeys - openedCommonLocks;
				const uint16_t uniqueKeys = collectedUniqueKeys - openedUniqueLocks;
				const bool success =
					goalReached &&
					commonKeys == 0 &&
					uniqueKeys == 0 &&
					openedLocks == graph.AllLockMask;
				if (goalReached && !success)
				{
					return false;
				}
				if (success)
				{
					reachedSuccess = true;
					continue;
				}

				for (const RegionAisleData& aisle : graph.Aisles)
				{
					if ((openedLocks & aisle.Bit) != 0)
					{
						continue;
					}
					const bool region0Reached = (reachableRegions & (uint32_t{ 1 } << aisle.Region0)) != 0;
					const bool region1Reached = (reachableRegions & (uint32_t{ 1 } << aisle.Region1)) != 0;
					if (region0Reached && region1Reached)
					{
						return false;
					}
				}

				bool hasLegalTransition = false;
				for (const RegionAisleData& aisle : graph.Aisles)
				{
					if ((openedLocks & aisle.Bit) != 0)
					{
						continue;
					}
					const bool region0Reached = (reachableRegions & (uint32_t{ 1 } << aisle.Region0)) != 0;
					const bool region1Reached = (reachableRegions & (uint32_t{ 1 } << aisle.Region1)) != 0;
					if (region0Reached == region1Reached)
					{
						continue;
					}

					const bool canOpen = aisle.Lock == LockType::Common ?
						commonKeys > 0 :
						uniqueKeys > 0 && commonKeys == 0 && (openedLocks & graph.CommonLockMask) == graph.CommonLockMask;
					if (!canOpen)
					{
						continue;
					}

					hasLegalTransition = true;
					const uint32_t nextState = openedLocks | aisle.Bit;
					if (visited[nextState] == 0)
					{
						const uint8_t nextRegion = region0Reached ? aisle.Region1 : aisle.Region0;
						visited[nextState] = 1;
						reachableRegionsByState[nextState] = reachableRegions | (uint32_t{ 1 } << nextRegion);
						collectedCommonKeysByState[nextState] = collectedCommonKeys + graph.Regions[nextRegion].CommonKeys;
						collectedUniqueKeysByState[nextState] = collectedUniqueKeys + graph.Regions[nextRegion].UniqueKeys;
						queue.emplace_back(nextState);
					}
				}

				if (!hasLegalTransition)
				{
					return false;
				}
			}

			return reachedSuccess;
		}

		bool CanSolveMissionGraph(const MissionGraphData& graph)
		{
			if (graph.Rooms.empty() || !ValidateKeyLockCounts(graph))
			{
				return false;
			}

			RegionGraphData regionGraph;
			return BuildRegionGraph(graph, regionGraph) && CanSolveMissionGraphForEveryOrder(regionGraph);
		}
	}

	MissionGraphTester::MissionGraphTester(const std::list<std::shared_ptr<Room>>& rooms, const std::vector<Aisle>& aisles)
	{
		MissionGraphData graph;
		if (!BuildMissionGraphData(rooms, aisles, graph))
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph test failed. The graph input is invalid."));
			return;
		}

		mResult = CanSolveMissionGraph(graph);
		if (mResult)
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph test succeeded."));
		}
		else
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph test failed. The generated key-lock route is not solvable for every lock-opening order."));
		}
	}

	bool MissionGraphTester::Success() const
	{
		return mResult;
	}
}
