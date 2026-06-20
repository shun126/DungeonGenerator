/**
 * Intent graph generator for dungeon layouts.
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
	 * Generates an abstract graph before room placement.
	 * 部屋配置前の抽象グラフを生成します。
	 */
	class LayoutGraphGenerator final
	{
	public:
		static LayoutGraph Generate(const GenerateParameter& parameter);

	private:
		static EDungeonRoomGameplayRole SelectMainPathGameplayRole(const GenerateParameter& parameter, int32 index, int32 mainPathCount) noexcept;
		static EDungeonRoomGameplayRole SelectBranchGameplayRole(const GenerateParameter& parameter, int32 branchIndex) noexcept;
		static void AssignStructuralRoles(const GenerateParameter& parameter, LayoutGraph& graph) noexcept;
		static bool HasEdge(const LayoutGraph& graph, size_t room0, size_t room1) noexcept;
	};
}
