/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Intent-driven dungeon layout graph types.
 * LayoutGraph を表します。
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
	constexpr float MainRouteBiasInfluence = 0.16f;
	constexpr float LoopRouteDensityInfluence = 0.35f;

	/**
	 * Node used before concrete room placement.
	 * 具体的な部屋配置の前に使うレイアウトノードです。
	 */
	struct LayoutRoomNode final
	{
		size_t Index = 0;
		size_t ParentIndex = 0;
		EDungeonRoomStructuralRole StructuralRole = EDungeonRoomStructuralRole::Connector;
		EDungeonRoomGameplayRole GameplayRole = EDungeonRoomGameplayRole::None;
		/**
		 * Gameplay role restored when this node is not part of the selected main route.
		 * このノードが選択された主経路に含まれない場合に復元するゲームプレイロールです。
		 */
		EDungeonRoomGameplayRole NonMainPathGameplayRole = EDungeonRoomGameplayRole::None;
		int32 DesiredDepth = 0;
		int32 DesiredBranch = 0;
		int32 DesiredFloor = 0;
		int32 ZoneIndex = INDEX_NONE;
		/**
		 * Stable weighted-selection roll reused when the final floor changes.
		 * 最終的な階層が変化した場合に再利用する、重み付きZone選択用の固定乱数値です。
		 */
		float ZoneSelectionRoll = 0.f;
		float Intensity = 1.f;
	};

	/**
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

	/**
	 * Intent graph that describes rooms and route purposes.
	 * 部屋と経路目的を表す意図グラフです。
	 */
	struct LayoutGraph final
	{
		std::vector<LayoutRoomNode> Nodes;
		std::vector<LayoutAisleEdge> Edges;
		/**
		 * Node indices marked as start rooms. The first entry is the primary start used for traversal.
		 * 開始部屋として扱うノード番号です。先頭要素は経路探索に使用する代表開始部屋です。
		 */
		std::vector<size_t> StartNodeIndices;
		size_t StartNodeIndex = 0;
		size_t GoalNodeIndex = 0;
	};

	/**
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

	struct RouteShapeProfile
	{
		float BaseMainRouteRatio = 0.55f;
		float MinEffectiveMainRouteRatio = 0.10f;
		float MaxEffectiveMainRouteRatio = 1.00f;
		float LoopRouteDensity = 0.10f;
		float MinLoopRouteDensity = 0.00f;
		float MaxLoopRouteDensity = 1.00f;
	};

	RouteShapeProfile GetRouteShapeProfile(const EDungeonProgressionPolicy policy) noexcept;
	float CalculateEffectiveMainRouteRatio(const FDungeonPathSettings& settings) noexcept;
	float CalculateEffectiveLoopRouteDensity(const FDungeonPathSettings& settings) noexcept;
}
