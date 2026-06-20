/**
 * Layout evaluator for dungeon layout candidates.
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
	 * Scores concrete layout candidates before voxel generation.
	 * ボクセル生成前の具体的なレイアウト候補を採点します。
	 */
	class LayoutEvaluator final
	{
	public:
		static void Evaluate(const GenerateParameter& parameter, const int32 candidateIndex, LayoutCandidate& candidate) noexcept;

	private:
		static bool IsSpecialGameplayRole(EDungeonRoomGameplayRole gameplayRole) noexcept;
		static bool CanReachGoal(const LayoutGraph& graph) noexcept;
		static float ScoreRatio(float actual, float target) noexcept;
	};
}
