/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Intent graph generator for dungeon layouts.
 * LayoutGraphGenerator を表します。
 */

#pragma once
#include "LayoutGraph.h"

namespace dungeon
{
	struct GenerateParameter;

	/**
	 * Represents LayoutGraphGenerator.
	 * 部屋配置前の抽象グラフを生成します。
	 */
	class LayoutGraphGenerator final
	{
	public:
		/**
		 * Generates an abstract layout graph.
		 * 抽象レイアウトグラフを生成します。
		 */
		static LayoutGraph Generate(const GenerateParameter& parameter);

		/**
		 * Selects goal and start rooms from concrete positions and updates their structural roles.
		 * 配置済みの座標からゴール部屋と開始部屋を選び、構造的役割を更新します。
		 */
		/**
		 * サイズを変更できない開始部屋とゴール部屋の接続数を、門を置ける数まで減らします
		 * グラフの辺を削除するため、通路の一覧を構築する前に呼び出して下さい
		 * @param[in]		parameter	生成パラメータ
		 * @param[in,out]	graph		レイアウトグラフ
		 */
		static void LimitEndpointGateCapacity(const GenerateParameter& parameter, LayoutGraph& graph) noexcept;

		/**
		 * 枝の通路を、より近い部屋へつなぎ替えます
		 * 接続先は部屋を配置する前に決まるため、遠い部屋どうしがつながる事があります
		 * 部屋の配置後、通路の一覧を構築する前に呼び出して下さい
		 * @param[in,out]	graph	レイアウトグラフ
		 * @param[in]		rooms	配置済みの部屋
		 */
		static void OptimizeBranchParents(LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms) noexcept;

		/**
		 * 配置済みの座標からゴール部屋と開始部屋を選び、構造的役割を更新します
		 * 通路の一覧を構築した後に呼び出す場合はbAislesBuiltにtrueを渡して下さい
		 * 鍵をかけられる橋を復活させる処理がグラフの辺を削除するため、
		 * 通路から辺を逆引きできなくなり、生成が失敗します
		 * @param[in]		parameter		生成パラメータ
		 * @param[in,out]	graph			レイアウトグラフ
		 * @param[in]		rooms			配置済みの部屋
		 * @param[in]		bAislesBuilt	通路の一覧を構築済みならtrue
		 * @return			trueならば適用成功
		 */
		static bool ApplyEndpointPolicies(const GenerateParameter& parameter, LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms, const bool bAislesBuilt = false) noexcept;

	private:
		static bool ApplyStartRoomPolicy(const GenerateParameter& parameter, LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& rooms) noexcept;
		static bool RebuildMainRoute(const GenerateParameter& parameter, LayoutGraph& graph, const std::vector<std::shared_ptr<Room>>& rooms) noexcept;
		static EDungeonRoomGameplayRole SelectMainPathGameplayRole(const GenerateParameter& parameter, int32 index, int32 mainPathCount) noexcept;
		static EDungeonRoomGameplayRole SelectBranchGameplayRole(const GenerateParameter& parameter, int32 branchIndex) noexcept;
		static void AssignStructuralRoles(const GenerateParameter& parameter, LayoutGraph& graph) noexcept;
		static bool HasEdge(const LayoutGraph& graph, size_t room0, size_t room1) noexcept;
	};
}
