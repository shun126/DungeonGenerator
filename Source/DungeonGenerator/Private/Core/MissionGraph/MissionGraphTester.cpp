/**
 * Tests whether a MissionGraph is solvable.
 * ミッショングラフが攻略可能かテストします。
 *
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#include "MissionGraphTester.h"
#include "../Debug/Debug.h"
#include "../RoomGeneration/Aisle.h"
#include <algorithm>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace dungeon
{
	namespace
	{
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
			bool bHasStartRoom = false;
			bool bHasGoalRoom = false;
		};

		struct SearchState final
		{
			std::vector<uint8_t> ReachableRooms;
			std::vector<uint8_t> CollectedRooms;
			std::vector<uint8_t> OpenedAisles;
			uint8_t CommonKeys = 0;
			uint8_t UniqueKeys = 0;
		};

		bool IsAisleOpen(const SearchState& state, const AisleData& aisle, const size_t aisleIndex) noexcept
		{
			return aisle.Lock == LockType::None || state.OpenedAisles[aisleIndex] != 0;
		}

		bool IsSuccess(const MissionGraphData& graph, const SearchState& state) noexcept
		{
			if (state.ReachableRooms[graph.GoalRoom] == 0 || state.CommonKeys != 0 || state.UniqueKeys != 0)
			{
				return false;
			}

			for (size_t aisleIndex = 0; aisleIndex < graph.Aisles.size(); ++aisleIndex)
			{
				if (graph.Aisles[aisleIndex].Lock != LockType::None && state.OpenedAisles[aisleIndex] == 0)
				{
					return false;
				}
			}
			return true;
		}

		std::string MakeVisitedKey(const SearchState& state)
		{
			std::string key;
			key.reserve(state.CollectedRooms.size() + state.OpenedAisles.size() + 2);
			for (const uint8_t collected : state.CollectedRooms)
			{
				key.push_back(collected != 0 ? '1' : '0');
			}
			key.push_back('|');
			for (const uint8_t opened : state.OpenedAisles)
			{
				key.push_back(opened != 0 ? '1' : '0');
			}
			key.push_back('|');
			key.push_back(static_cast<char>(state.CommonKeys));
			key.push_back(static_cast<char>(state.UniqueKeys));
			return key;
		}

		void ExpandReachableRooms(const MissionGraphData& graph, SearchState& state) noexcept
		{
			bool changed;
			do
			{
				changed = false;
				for (size_t aisleIndex = 0; aisleIndex < graph.Aisles.size(); ++aisleIndex)
				{
					const AisleData& aisle = graph.Aisles[aisleIndex];
					if (!IsAisleOpen(state, aisle, aisleIndex))
					{
						continue;
					}

					if (state.ReachableRooms[aisle.Room0] != 0 && state.ReachableRooms[aisle.Room1] == 0)
					{
						state.ReachableRooms[aisle.Room1] = 1;
						changed = true;
					}
					if (state.ReachableRooms[aisle.Room1] != 0 && state.ReachableRooms[aisle.Room0] == 0)
					{
						state.ReachableRooms[aisle.Room0] = 1;
						changed = true;
					}
				}

				for (size_t roomIndex = 0; roomIndex < graph.Rooms.size(); ++roomIndex)
				{
					if (state.ReachableRooms[roomIndex] == 0 || state.CollectedRooms[roomIndex] != 0)
					{
						continue;
					}

					switch (graph.Rooms[roomIndex].Item)
					{
					case Room::Item::Key:
						++state.CommonKeys;
						state.CollectedRooms[roomIndex] = 1;
						changed = true;
						break;
					case Room::Item::UniqueKey:
						++state.UniqueKeys;
						state.CollectedRooms[roomIndex] = 1;
						changed = true;
						break;
					default:
						break;
					}
				}
			} while (changed);
		}

		bool HasEarlyGoalAccess(const MissionGraphData& graph, const SearchState& state) noexcept
		{
			return state.ReachableRooms[graph.GoalRoom] != 0 && !IsSuccess(graph, state);
		}

		bool HasBypassedLock(const MissionGraphData& graph, const SearchState& state) noexcept
		{
			for (size_t aisleIndex = 0; aisleIndex < graph.Aisles.size(); ++aisleIndex)
			{
				const AisleData& aisle = graph.Aisles[aisleIndex];
				if (aisle.Lock != LockType::None &&
					state.OpenedAisles[aisleIndex] == 0 &&
					state.ReachableRooms[aisle.Room0] != 0 &&
					state.ReachableRooms[aisle.Room1] != 0)
				{
					return true;
				}
			}
			return false;
		}

		bool CanOpenUniqueLock(const MissionGraphData& graph, const SearchState& state) noexcept
		{
			if (state.UniqueKeys == 0 || state.CommonKeys != 0)
			{
				return false;
			}

			for (size_t aisleIndex = 0; aisleIndex < graph.Aisles.size(); ++aisleIndex)
			{
				if (graph.Aisles[aisleIndex].Lock == LockType::Common && state.OpenedAisles[aisleIndex] == 0)
				{
					return false;
				}
			}
			return true;
		}

		bool BuildMissionGraphData(
			const std::list<std::shared_ptr<Room>>& rooms,
			const std::vector<Aisle>& aisles,
			MissionGraphData& outGraph) noexcept
		{
			std::unordered_map<Identifier, size_t> roomIndices;
			outGraph.Rooms.reserve(rooms.size());
			for (const std::shared_ptr<Room>& room : rooms)
			{
				if (!room)
				{
					continue;
				}

				RoomData data;
				data.Parts = room->GetParts();
				data.Item = room->GetItem();

				const size_t roomIndex = outGraph.Rooms.size();
				roomIndices.emplace(room->GetIdentifier(), roomIndex);
				outGraph.Rooms.emplace_back(data);

				if (data.Parts == Room::Parts::Start)
				{
					outGraph.StartRoom = roomIndex;
					outGraph.bHasStartRoom = true;
				}
				else if (data.Parts == Room::Parts::Goal)
				{
					outGraph.GoalRoom = roomIndex;
					outGraph.bHasGoalRoom = true;
				}
			}

			if (!outGraph.bHasStartRoom || !outGraph.bHasGoalRoom)
			{
				return false;
			}

			outGraph.Aisles.reserve(aisles.size());
			for (const Aisle& aisle : aisles)
			{
				const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (!room0 || !room1)
				{
					return false;
				}

				const auto room0Index = roomIndices.find(room0->GetIdentifier());
				const auto room1Index = roomIndices.find(room1->GetIdentifier());
				if (room0Index == roomIndices.end() || room1Index == roomIndices.end())
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
				commonKeyCount == graph.CommonLockCount &&
				uniqueKeyCount == graph.UniqueLockCount &&
				graph.UniqueLockCount == 1;
		}

		bool CanSolveMissionGraph(const MissionGraphData& graph)
		{
			if (graph.Rooms.empty() || !ValidateKeyLockCounts(graph))
			{
				return false;
			}

			SearchState initialState;
			initialState.ReachableRooms.resize(graph.Rooms.size(), 0);
			initialState.CollectedRooms.resize(graph.Rooms.size(), 0);
			initialState.OpenedAisles.resize(graph.Aisles.size(), 0);
			initialState.ReachableRooms[graph.StartRoom] = 1;
			ExpandReachableRooms(graph, initialState);
			if (HasEarlyGoalAccess(graph, initialState) || HasBypassedLock(graph, initialState))
			{
				return false;
			}
			if (IsSuccess(graph, initialState))
			{
				return true;
			}

			std::queue<SearchState> queue;
			std::unordered_set<std::string> visited;
			visited.emplace(MakeVisitedKey(initialState));
			queue.emplace(std::move(initialState));

			while (!queue.empty())
			{
				SearchState state = std::move(queue.front());
				queue.pop();

				for (size_t aisleIndex = 0; aisleIndex < graph.Aisles.size(); ++aisleIndex)
				{
					const AisleData& aisle = graph.Aisles[aisleIndex];
					if (aisle.Lock == LockType::None || state.OpenedAisles[aisleIndex] != 0)
					{
						continue;
					}
					if (state.ReachableRooms[aisle.Room0] == 0 && state.ReachableRooms[aisle.Room1] == 0)
					{
						continue;
					}

					SearchState nextState = state;
					if (aisle.Lock == LockType::Common)
					{
						if (nextState.CommonKeys == 0)
						{
							continue;
						}
						--nextState.CommonKeys;
					}
					else if (aisle.Lock == LockType::Unique)
					{
						if (!CanOpenUniqueLock(graph, nextState))
						{
							continue;
						}
						--nextState.UniqueKeys;
					}

					nextState.OpenedAisles[aisleIndex] = 1;
					ExpandReachableRooms(graph, nextState);
					if (HasEarlyGoalAccess(graph, nextState) || HasBypassedLock(graph, nextState))
					{
						return false;
					}
					if (IsSuccess(graph, nextState))
					{
						return true;
					}

					const std::string key = MakeVisitedKey(nextState);
					if (visited.insert(key).second)
					{
						queue.emplace(std::move(nextState));
					}
				}
			}

			return false;
		}
	}

	MissionGraphTester::MissionGraphTester(const std::list<std::shared_ptr<Room>>& rooms, const std::vector<Aisle>& aisles)
	{
		MissionGraphData graph;
		if (!BuildMissionGraphData(rooms, aisles, graph))
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph test failed. Start or goal room is missing."));
			return;
		}

		mResult = CanSolveMissionGraph(graph);
		if (mResult)
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph test succeeded."));
		}
		else
		{
			DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph test failed. The generated key-lock route is not solvable."));
		}
	}

	bool MissionGraphTester::Success() const
	{
		return mResult;
	}
}
