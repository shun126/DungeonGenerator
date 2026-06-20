/**
 * Room placer for intent-driven dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "LayoutGraph.h"

namespace dungeon
{
	struct GenerateParameter;

	/*
	 * Converts an abstract layout graph into concrete room locations.
	 * 抽象レイアウトグラフを具体的な部屋座標へ変換します。
	 */
	class RoomPlacer final
	{
	public:
		static std::list<std::shared_ptr<Room>> Place(const GenerateParameter& parameter, LayoutGraph& graph);

	private:
		static int32 CalculateSpacing(const GenerateParameter& parameter) noexcept;
		static FIntVector MakeMainPathLocation(const GenerateParameter& parameter, int32 nodeIndex, int32 spacing);
		static FIntVector MakeBranchLocation(const GenerateParameter& parameter, const LayoutGraph& graph, const std::vector<FIntVector>& locations, const LayoutRoomNode& node, int32 spacing);
	};
}
