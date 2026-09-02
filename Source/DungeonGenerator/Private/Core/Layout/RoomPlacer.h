/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Room placer for intent-driven dungeon layouts.
 * RoomPlacer を表します。
 */

#pragma once
#include "LayoutGraph.h"

namespace dungeon
{
	struct GenerateParameter;

	/**
	 * Converts an abstract layout graph into concrete room locations.
	 * 抽象レイアウトグラフを具体的な部屋座標へ変換します。
	 */
	class RoomPlacer final
	{
	public:
		/**
		 * Converts an abstract graph into rooms placed on deterministic horizontal routes and floor layers.
		 * 抽象グラフを決定的な水平経路と階層へ配置した部屋へ変換します。
		 */
		static std::list<std::shared_ptr<Room>> Place(const GenerateParameter& parameter, LayoutGraph& graph);

		/**
		 * Reassigns node and room zones from their current finalized floor locations.
		 * 現在の確定済み階層位置からノードと部屋のZoneを再割り当てします。
		 */
		static void AssignZones(const GenerateParameter& parameter, LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms) noexcept;

		/**
		 * Returns the distance between shared floor origins.
		 * 共通階層原点どうしの間隔を返します。
		 */
		static int32 CalculateVerticalSpacing(const GenerateParameter& parameter) noexcept;

		/**
		 * Returns the automatically derived floor count used by Free layouts.
		 * Free配置で自動計算する階層数を返します。
		 */
		static int32 CalculateAutoFreeFloorCount(const GenerateParameter& parameter) noexcept;

		/**
		 * Snaps a Z coordinate to a valid floor origin for the current expansion policy.
		 * Z座標を現在の展開方針で有効な階層原点へスナップします。
		 */
		static int32 SnapToFloorOrigin(const GenerateParameter& parameter, int32 z) noexcept;

	private:
		static int32 CalculateSpacing(const GenerateParameter& parameter) noexcept;
		static FIntVector MakeMainPathLocation(
			const GenerateParameter& parameter,
			const std::vector<std::shared_ptr<Room>>& indexedRooms,
			const LayoutRoomNode& node,
			const std::shared_ptr<Room>& room);
		static FIntVector MakeBranchLocation(const GenerateParameter& parameter, const LayoutGraph& graph, const std::vector<FIntVector>& locations, const LayoutRoomNode& node, int32 spacing);
	};
}
