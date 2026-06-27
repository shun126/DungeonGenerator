/**
 * Layout evaluator for dungeon layout candidates.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "LayoutEvaluator.h"
#include "../GenerateParameter.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace dungeon
{
	namespace
	{
		constexpr float criticalPathScoreWeight = 1.00f;
		constexpr float loopScoreWeight = 0.60f;
		constexpr float specialDeadEndScoreWeight = 0.50f;
		constexpr float aisleDistanceScoreWeight = 0.75f;
		constexpr float policyScoreWeight = 1.25f;

		int32 CountGraphDegree(const LayoutGraph& graph, const size_t nodeIndex) noexcept
		{
			int32 degree = 0;
			for (const LayoutAisleEdge& edge : graph.Edges)
			{
				if (edge.Room0 == nodeIndex || edge.Room1 == nodeIndex)
				{
					++degree;
				}
			}
			return degree;
		}

		bool HasBossNearGoal(const LayoutGraph& graph) noexcept
		{
			if (graph.GoalNodeIndex >= graph.Nodes.size())
			{
				return false;
			}

			const int32 goalDepth = graph.Nodes[graph.GoalNodeIndex].DesiredDepth;
			return std::any_of(graph.Nodes.begin(), graph.Nodes.end(), [goalDepth](const LayoutRoomNode& node)
				{
					return node.GameplayRole == EDungeonRoomGameplayRole::Boss && node.DesiredBranch == 0 && node.DesiredDepth >= goalDepth - 2;
				}
			);
		}

		int32 CountEarlyHubBranches(const LayoutGraph& graph) noexcept
		{
			int32 branchCount = 0;
			for (const LayoutAisleEdge& edge : graph.Edges)
			{
				const size_t room0 = edge.Room0;
				const size_t room1 = edge.Room1;
				if (room0 >= graph.Nodes.size() || room1 >= graph.Nodes.size())
				{
					continue;
				}

				const bool bRoom0EarlyHub = graph.Nodes[room0].DesiredBranch == 0 && graph.Nodes[room0].DesiredDepth == 1;
				const bool bRoom1EarlyHub = graph.Nodes[room1].DesiredBranch == 0 && graph.Nodes[room1].DesiredDepth == 1;
				const bool bRoom0Branch = graph.Nodes[room0].DesiredBranch > 0;
				const bool bRoom1Branch = graph.Nodes[room1].DesiredBranch > 0;
				if ((bRoom0EarlyHub && bRoom1Branch) || (bRoom1EarlyHub && bRoom0Branch))
				{
					++branchCount;
				}
			}
			return branchCount;
		}

		/*
		 * Returns the empty gap between two one-dimensional room ranges.
		 * 1次元の部屋範囲同士にある空き距離を返します。
		 */
		int32 CalculateIntervalGap(const int32 min0, const int32 max0, const int32 min1, const int32 max1) noexcept
		{
			if (max0 < min1)
				return min1 - max0;
			if (max1 < min0)
				return min0 - max1;
			return 0;
		}

		/*
		 * Returns a purpose weight used when scoring aisle length.
		 * 通路距離を採点するときに使う通路目的ごとの重みを返します。
		 */
		float GetAisleDistanceWeight(const Aisle& aisle) noexcept
		{
			if (aisle.IsMain())
			{
				return 1.25f;
			}

			switch (aisle.GetPurpose())
			{
			case EDungeonAislePurpose::Locked:
			case EDungeonAislePurpose::VerticalTransition:
				return 1.10f;
			case EDungeonAislePurpose::Branch:
				return 0.85f;
			case EDungeonAislePurpose::Loop:
			case EDungeonAislePurpose::Shortcut:
				return 0.65f;
			case EDungeonAislePurpose::MainPath:
			default:
				return 1.00f;
			}
		}

		/*
		 * Calculates aisle distance as the sum of room shell gaps on each axis.
		 * 各軸の部屋外周間ギャップ合計として通路距離を計算します。
		 */
		int32 CalculateRoomPairAisleDistance(const Room& room0, const Room& room1) noexcept
		{
			return
				CalculateIntervalGap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight()) +
				CalculateIntervalGap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom()) +
				CalculateIntervalGap(room0.GetBackground(), room0.GetForeground(), room1.GetBackground(), room1.GetForeground());
		}

		float ScoreProgressionPolicyExpression(const GenerateParameter& parameter, const LayoutCandidate& candidate, const FDungeonLayoutMetrics& metrics) noexcept
		{
			const EDungeonProgressionPolicy policy = parameter.GetPathSettings().ProgressionPolicy;
			const int32 goalDegree = CountGraphDegree(candidate.Graph, candidate.Graph.GoalNodeIndex);
			const float endpointScore = goalDegree == 1 ? 1.f : 0.f;
			const float openGoalScore = goalDegree > 1 ? 1.f : 0.f;
			const float loopScore = metrics.RoomCount > 0 ? std::clamp(static_cast<float>(metrics.LoopCount) / std::max(1.f, static_cast<float>(metrics.RoomCount) * 0.25f), 0.f, 1.f) : 0.f;

			switch (policy)
			{
			case EDungeonProgressionPolicy::FreeExploration:
				return loopScore * 0.70f + openGoalScore * 0.50f;
			case EDungeonProgressionPolicy::KeysAndLocks:
				return (metrics.LockedRouteCount > 0 ? 0.80f : -0.80f) + (metrics.LoopCount == 0 ? 0.50f : -0.50f) + endpointScore * 0.30f;
			case EDungeonProgressionPolicy::BossRoute:
				return (HasBossNearGoal(candidate.Graph) ? 0.80f : -0.80f) + endpointScore * 0.40f + (metrics.LoopCount <= 2 ? 0.25f : 0.f);
			case EDungeonProgressionPolicy::HubQuest:
				return std::clamp(static_cast<float>(CountEarlyHubBranches(candidate.Graph)) / 3.f, 0.f, 1.f) * 0.85f + endpointScore * 0.35f;
			case EDungeonProgressionPolicy::StartToGoal:
			default:
				return endpointScore * 0.50f + (metrics.LoopCount <= std::max(1, metrics.RoomCount / 5) ? 0.30f : 0.f);
			}
		}
	}

	void LayoutEvaluator::Evaluate(const GenerateParameter& parameter, const int32 candidateIndex, LayoutCandidate& candidate) noexcept
	{
		const auto& settings = parameter.GetPathSettings();
		FDungeonLayoutMetrics metrics;
		metrics.RoomCount = static_cast<int32>(candidate.Graph.Nodes.size());
		metrics.AisleCount = static_cast<int32>(candidate.Aisles.size());
		metrics.bMissionSolvable = CanReachGoal(candidate.Graph);

		std::vector<int32> degree(candidate.Graph.Nodes.size(), 0);
		for (const LayoutAisleEdge& edge : candidate.Graph.Edges)
		{
			if (edge.Room0 < degree.size())
				++degree[edge.Room0];
			if (edge.Room1 < degree.size())
				++degree[edge.Room1];
		}

		for (const Aisle& aisle : candidate.Aisles)
		{
			if (aisle.GetPoint(0) && aisle.GetPoint(1))
			{
				const auto room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (room0 && room1)
				{
					const float weightedDistance = static_cast<float>(CalculateRoomPairAisleDistance(*room0, *room1)) * GetAisleDistanceWeight(aisle);
					metrics.TotalAisleDistance += weightedDistance;
					metrics.MaxAisleDistance = std::max(metrics.MaxAisleDistance, weightedDistance);
					if (aisle.IsMain())
					{
						metrics.MainPathAisleDistance += weightedDistance;
					}
				}
			}

			if (aisle.IsMain())
			{
				++metrics.CriticalPathLength;
			}

			switch (aisle.GetPurpose())
			{
			case EDungeonAislePurpose::Branch:
				++metrics.BranchCount;
				break;
			case EDungeonAislePurpose::Loop:
			case EDungeonAislePurpose::Shortcut:
				++metrics.LoopCount;
				break;
			case EDungeonAislePurpose::VerticalTransition:
				++metrics.VerticalTransitionCount;
				break;
			case EDungeonAislePurpose::Locked:
				++metrics.LockedRouteCount;
				break;
			default:
				break;
			}
		}
		if (metrics.CriticalPathLength > 0)
		{
			++metrics.CriticalPathLength;
		}

		for (size_t index = 0; index < candidate.Graph.Nodes.size(); ++index)
		{
			const LayoutRoomNode& node = candidate.Graph.Nodes[index];
			if (node.GameplayRole == EDungeonRoomGameplayRole::Secret)
			{
				++metrics.SecretRoomCount;
			}
			if (node.ZoneIndex != INDEX_NONE)
			{
				metrics.ZoneCount = std::max(metrics.ZoneCount, node.ZoneIndex + 1);
			}
			metrics.AverageIntensity += node.Intensity;

			if (degree[index] <= 1 && index != candidate.Graph.StartNodeIndex && index != candidate.Graph.GoalNodeIndex)
			{
				++metrics.DeadEndCount;
				if (IsSpecialGameplayRole(node.GameplayRole))
				{
					++metrics.SpecialDeadEndCount;
				}
			}
		}
		if (!candidate.Graph.Nodes.empty())
		{
			metrics.AverageIntensity /= static_cast<float>(candidate.Graph.Nodes.size());
		}

		if (metrics.DeadEndCount > 0)
		{
			metrics.SpecialDeadEndCoverage = static_cast<float>(metrics.SpecialDeadEndCount) / static_cast<float>(metrics.DeadEndCount);
		}
		if (metrics.AisleCount > 0)
		{
			metrics.AverageAisleDistance = metrics.TotalAisleDistance / static_cast<float>(metrics.AisleCount);
		}

		if (candidate.StartPoint && candidate.GoalPoint)
		{
			metrics.StartGoalDistance = static_cast<float>(Point::Dist(*candidate.StartPoint, *candidate.GoalPoint));
		}

		FDungeonLayoutScore score;
		score.CandidateIndex = candidateIndex;
		score.bAccepted = metrics.RoomCount >= 3 && metrics.AisleCount >= metrics.RoomCount - 1 && metrics.bMissionSolvable;
		score.Reason = score.bAccepted ? TEXT("Accepted") : TEXT("Rejected: disconnected or undersized layout");

		const float effectiveMainRouteRatio = CalculateEffectiveMainRouteRatio(settings);
		const float effectiveLoopRouteDensity = CalculateEffectiveLoopRouteDensity(settings);
		const float targetCriticalPath = std::max(2.f, static_cast<float>(metrics.RoomCount) * effectiveMainRouteRatio);
		const bool bKeysAndLocks = parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks;
		const float targetLoops = bKeysAndLocks ? 0.f : std::max(1.f, static_cast<float>(metrics.RoomCount) * effectiveLoopRouteDensity);
		const float aisleDistanceScale = std::max(1.f, static_cast<float>(std::max(parameter.GetMaxRoomWidth(), parameter.GetMaxRoomDepth()) + parameter.GetHorizontalRoomMargin()));
		const float aisleDistanceScore = 1.f / (1.f + metrics.AverageAisleDistance / aisleDistanceScale + metrics.MaxAisleDistance / (aisleDistanceScale * 2.f));

		score.TotalScore =
			criticalPathScoreWeight * ScoreRatio(static_cast<float>(metrics.CriticalPathLength), targetCriticalPath) +
			loopScoreWeight * ScoreRatio(static_cast<float>(metrics.LoopCount), targetLoops) +
			specialDeadEndScoreWeight * metrics.SpecialDeadEndCoverage +
			aisleDistanceScoreWeight * aisleDistanceScore +
			policyScoreWeight * ScoreProgressionPolicyExpression(parameter, candidate, metrics);
		if (!parameter.GetZoneSettings().Zones.IsEmpty())
		{
			score.TotalScore += ScoreRatio(static_cast<float>(metrics.ZoneCount), static_cast<float>(parameter.GetZoneSettings().Zones.Num()));
		}
		if (bKeysAndLocks)
		{
			score.TotalScore += metrics.LockedRouteCount > 0 ? 1.f : -1.f;
		}

		if (!score.bAccepted)
		{
			score.TotalScore -= 1000.f;
		}

		candidate.Metrics = metrics;
		candidate.Score = score;
	}

	bool LayoutEvaluator::IsSpecialGameplayRole(const EDungeonRoomGameplayRole gameplayRole) noexcept
	{
		return
			gameplayRole == EDungeonRoomGameplayRole::Treasure ||
			gameplayRole == EDungeonRoomGameplayRole::Secret ||
			gameplayRole == EDungeonRoomGameplayRole::Puzzle ||
			gameplayRole == EDungeonRoomGameplayRole::Rest;
	}

	bool LayoutEvaluator::CanReachGoal(const LayoutGraph& graph) noexcept
	{
		if (graph.Nodes.empty() || graph.StartNodeIndex >= graph.Nodes.size() || graph.GoalNodeIndex >= graph.Nodes.size())
		{
			return false;
		}

		std::vector<std::vector<size_t>> routes(graph.Nodes.size());
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 < routes.size() && edge.Room1 < routes.size())
			{
				routes[edge.Room0].emplace_back(edge.Room1);
				routes[edge.Room1].emplace_back(edge.Room0);
			}
		}

		std::vector<bool> visited(graph.Nodes.size(), false);
		std::queue<size_t> queue;
		queue.emplace(graph.StartNodeIndex);
		visited[graph.StartNodeIndex] = true;

		while (!queue.empty())
		{
			const size_t current = queue.front();
			queue.pop();
			if (current == graph.GoalNodeIndex)
			{
				return true;
			}

			for (const size_t next : routes[current])
			{
				if (!visited[next])
				{
					visited[next] = true;
					queue.emplace(next);
				}
			}
		}

		return false;
	}

	float LayoutEvaluator::ScoreRatio(const float actual, const float target) noexcept
	{
		if (target <= std::numeric_limits<float>::epsilon())
		{
			return actual <= std::numeric_limits<float>::epsilon() ? 1.f : 0.f;
		}

		const float delta = std::abs(actual - target) / target;
		return std::clamp(1.f - delta, 0.f, 1.f);
	}
}
