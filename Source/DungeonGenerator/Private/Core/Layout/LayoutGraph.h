/**
 * Intent-driven dungeon layout graph types.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "../Math/Point.h"
#include "../RoomGeneration/Aisle.h"
#include "../RoomGeneration/Room.h"
#include "Parameter/DungeonLayoutTypes.h"
#include <list>
#include <memory>
#include <vector>

namespace dungeon
{
	/*
	 * Node used before concrete room placement.
	 * 具体的な部屋配置の前に使うレイアウトノードです。
	 */
	struct LayoutRoomNode final
	{
		size_t Index = 0;
		size_t ParentIndex = 0;
		EDungeonRoomStructuralRole StructuralRole = EDungeonRoomStructuralRole::Connector;
		EDungeonRoomGameplayRole GameplayRole = EDungeonRoomGameplayRole::None;
		int32 DesiredDepth = 0;
		int32 DesiredBranch = 0;
		int32 DesiredFloor = 0;
		int32 ZoneIndex = INDEX_NONE;
		float Intensity = 1.f;
	};

	/*
	 * Edge used before concrete aisle generation.
	 * 具体的な通路生成の前に使うレイアウトエッジです。
	 */
	struct LayoutAisleEdge final
	{
		size_t Room0 = 0;
		size_t Room1 = 0;
		EDungeonAislePurpose Purpose = EDungeonAislePurpose::MainPath;
		bool bMainPath = false;
	};

	/*
	 * Intent graph that describes rooms and route purposes.
	 * 部屋と経路目的を表す意図グラフです。
	 */
	struct LayoutGraph final
	{
		std::vector<LayoutRoomNode> Nodes;
		std::vector<LayoutAisleEdge> Edges;
		size_t StartNodeIndex = 0;
		size_t GoalNodeIndex = 0;
	};

	/*
	 * Concrete generated layout candidate.
	 * 実体化されたレイアウト候補です。
	 */
	struct LayoutCandidate final
	{
		LayoutGraph Graph;
		std::list<std::shared_ptr<Room>> Rooms;
		std::vector<Aisle> Aisles;
		std::shared_ptr<const Point> StartPoint;
		std::shared_ptr<const Point> GoalPoint;
		FDungeonLayoutMetrics Metrics;
		FDungeonLayoutScore Score;
	};
}
