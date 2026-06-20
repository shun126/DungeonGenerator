/**
 * Aisle planner for intent-driven dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "AislePlanner.h"
#include <algorithm>

namespace dungeon
{
	namespace
	{
		/*
		 * Returns the aisle purpose after concrete room heights are known.
		 * 実際の部屋高さが確定した後の通路目的を返します。
		 */
		EDungeonAislePurpose ResolveAislePurpose(const LayoutAisleEdge& edge, const std::shared_ptr<Room>& room0, const std::shared_ptr<Room>& room1) noexcept
		{
			if (edge.Purpose == EDungeonAislePurpose::Locked)
			{
				return edge.Purpose;
			}
			return room0->GetZ() != room1->GetZ() ? EDungeonAislePurpose::VerticalTransition : edge.Purpose;
		}
	}

	bool AislePlanner::Plan(const LayoutGraph& graph, std::list<std::shared_ptr<Room>>& rooms, std::vector<Aisle>& aisles, std::shared_ptr<const Point>& startPoint, std::shared_ptr<const Point>& goalPoint) noexcept
	{
		if (graph.Nodes.empty() || rooms.empty())
		{
			return false;
		}

		std::vector<std::shared_ptr<Room>> indexedRooms;
		indexedRooms.reserve(rooms.size());
		for (const std::shared_ptr<Room>& room : rooms)
		{
			room->ResetGateCount();
			room->ResetReservationNumber();
			room->SetItem(Room::Item::Empty);
			room->SetParts(Room::Parts::Unidentified);
			indexedRooms.emplace_back(room);
		}

		if (indexedRooms.size() != graph.Nodes.size() || graph.StartNodeIndex >= indexedRooms.size() || graph.GoalNodeIndex >= indexedRooms.size())
		{
			return false;
		}

		aisles.clear();
		aisles.reserve(graph.Edges.size());
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 >= indexedRooms.size() || edge.Room1 >= indexedRooms.size())
			{
				continue;
			}

			const std::shared_ptr<Room>& room0 = indexedRooms[edge.Room0];
			const std::shared_ptr<Room>& room1 = indexedRooms[edge.Room1];
			room0->AddGateCount(1);
			room1->AddGateCount(1);
			if (edge.bMainPath)
			{
				room0->SetMainPathRoom(true);
				room1->SetMainPathRoom(true);
			}
			if (edge.Purpose == EDungeonAislePurpose::Locked)
			{
				room0->SetLockedRouteRoom(true);
				room1->SetLockedRouteRoom(true);
			}

			auto point0 = std::make_shared<Point>(room0);
			auto point1 = std::make_shared<Point>(room1);
			aisles.emplace_back(edge.bMainPath, point0, point1, ResolveAislePurpose(edge, room0, room1));
		}

		ApplyRoomParts(graph, indexedRooms);

		startPoint = std::make_shared<Point>(indexedRooms[graph.StartNodeIndex]);
		goalPoint = std::make_shared<Point>(indexedRooms[graph.GoalNodeIndex]);
		return startPoint != nullptr && goalPoint != nullptr;
	}

	void AislePlanner::ApplyRoomParts(const LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& indexedRooms) noexcept
	{
		std::vector<int32> degree(indexedRooms.size(), 0);
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 < degree.size())
				++degree[edge.Room0];
			if (edge.Room1 < degree.size())
				++degree[edge.Room1];
		}

		for (size_t index = 0; index < indexedRooms.size(); ++index)
		{
			const std::shared_ptr<Room>& room = indexedRooms[index];
			switch (graph.Nodes[index].StructuralRole)
			{
			case EDungeonRoomStructuralRole::Start:
				room->SetParts(Room::Parts::Start);
				break;
			case EDungeonRoomStructuralRole::Goal:
				room->SetParts(Room::Parts::Goal);
				break;
			default:
				room->SetParts(degree[index] <= 1 ? Room::Parts::Hanare : Room::Parts::Hall);
				break;
			}
		}
	}
}
