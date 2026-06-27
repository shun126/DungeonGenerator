/**
 * Intent graph generator for dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "LayoutGraphGenerator.h"
#include "../GenerateParameter.h"
#include "../Math/Random.h"
#include <algorithm>
#include <cmath>

namespace dungeon
{
	namespace
	{
		struct WeightedZoneCandidate
		{
			int32 Index = INDEX_NONE;
			float Weight = 0.f;
		};

		int32 ClampRoomCount(const uint8_t count) noexcept
		{
			return std::max<int32>(3, count);
		}

		EDungeonAislePurpose MakeMainPathPurpose(const GenerateParameter& parameter, const int32 index, const int32 mainPathCount) noexcept
		{
			const auto& path = parameter.GetPathSettings();
			if (path.ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks &&
				index + 2 >= mainPathCount)
			{
				return EDungeonAislePurpose::Locked;
			}

			if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically)
			{
				return EDungeonAislePurpose::VerticalTransition;
			}

			return EDungeonAislePurpose::MainPath;
		}

		int32 SelectWeightedZoneIndex(const GenerateParameter& parameter, const float progress, const int32 floor) noexcept
		{
			const FDungeonZoneSettings& zones = parameter.GetZoneSettings();
			TArray<WeightedZoneCandidate> candidates;
			float totalWeight = 0.f;
			for (int32 index = 0; index < zones.Zones.Num(); ++index)
			{
				const FDungeonZoneDefinition& zone = zones.Zones[index];
				if (progress < zone.ProgressRange.Min || progress > zone.ProgressRange.Max)
				{
					continue;
				}
				if (floor < zone.FloorRange.Min || floor > zone.FloorRange.Max)
				{
					continue;
				}
				const float weight = std::max(0.f, zone.SelectionWeight);
				if (weight <= 0.f)
				{
					continue;
				}
				WeightedZoneCandidate candidate;
				candidate.Index = index;
				candidate.Weight = weight;
				candidates.Add(candidate);
				totalWeight += weight;
			}
			if (candidates.Num() == 1)
			{
				return candidates[0].Index;
			}
			if (totalWeight <= 0.f)
			{
				return INDEX_NONE;
			}

			float roll = parameter.GetRandom()->Get<float>() * totalWeight;
			for (const WeightedZoneCandidate& candidate : candidates)
			{
				roll -= candidate.Weight;
				if (roll <= 0.f)
				{
					return candidate.Index;
				}
			}
			return INDEX_NONE;
		}

		bool IsBranchSelectableRole(const EDungeonRoomGameplayRole role) noexcept
		{
			switch (role)
			{
			case EDungeonRoomGameplayRole::None:
			case EDungeonRoomGameplayRole::Combat:
			case EDungeonRoomGameplayRole::Treasure:
			case EDungeonRoomGameplayRole::Puzzle:
			case EDungeonRoomGameplayRole::Rest:
			case EDungeonRoomGameplayRole::Secret:
				return true;
			case EDungeonRoomGameplayRole::Boss:
			default:
				return false;
			}
		}

		const TArray<FDungeonRoomRoleProfile>& GetRoomRoleProfiles(const GenerateParameter& parameter) noexcept
		{
			return parameter.GetRoomRoleSettings().Roles;
		}

		EDungeonRoomGameplayRole SelectWeightedGameplayRole(const GenerateParameter& parameter, const TArray<FDungeonRoomRoleProfile>& profiles) noexcept
		{
			float totalWeight = 0.f;
			for (const FDungeonRoomRoleProfile& profile : profiles)
			{
				if (IsBranchSelectableRole(profile.Role))
				{
					totalWeight += std::max(0.f, profile.BranchSelectionWeight);
				}
			}
			if (totalWeight <= 0.f)
			{
				return EDungeonRoomGameplayRole::None;
			}

			float roll = parameter.GetRandom()->Get<float>() * totalWeight;
			for (const FDungeonRoomRoleProfile& profile : profiles)
			{
				if (!IsBranchSelectableRole(profile.Role))
				{
					continue;
				}
				roll -= std::max(0.f, profile.BranchSelectionWeight);
				if (roll <= 0.f)
				{
					return profile.Role;
				}
			}
			return EDungeonRoomGameplayRole::None;
		}

		int32 SelectBranchParentIndex(const GenerateParameter& parameter, const int32 mainPathCount, const int32 branchIndex, const bool bGoalCanReceiveExtraRoutes) noexcept
		{
			const auto& path = parameter.GetPathSettings();
			if (path.ProgressionPolicy == EDungeonProgressionPolicy::HubQuest && mainPathCount > 2 && (branchIndex % 4) != 3)
			{
				return 1;
			}

			if (path.ProgressionPolicy == EDungeonProgressionPolicy::FreeExploration && mainPathCount > 2 && (branchIndex % 5) == 0)
			{
				return mainPathCount - 1;
			}

			const int32 lastBranchParent = bGoalCanReceiveExtraRoutes ? mainPathCount - 1 : mainPathCount - 2;
			const int32 firstBranchParent = lastBranchParent >= 1 ? 1 + parameter.GetRandom()->Get<int32>(lastBranchParent) : 0;
			return std::clamp(firstBranchParent, 0, mainPathCount - 1);
		}
	}

	LayoutGraph LayoutGraphGenerator::Generate(const GenerateParameter& parameter)
	{
		const auto& settings = parameter.GetPathSettings();
		const int32 roomCount = ClampRoomCount(parameter.GetNumberOfCandidateRooms());
		const float effectiveMainRouteRatio = CalculateEffectiveMainRouteRatio(settings);
		const float effectiveLoopRouteDensity = CalculateEffectiveLoopRouteDensity(settings);
		const int32 mainPathCount = std::clamp(
			static_cast<int32>(std::round(static_cast<float>(roomCount) * effectiveMainRouteRatio)),
			2,
			roomCount
		);

		LayoutGraph graph;
		graph.Nodes.reserve(roomCount);
		graph.Edges.reserve(roomCount + static_cast<int32>(static_cast<float>(roomCount) * effectiveLoopRouteDensity) + 1);

		for (int32 index = 0; index < mainPathCount; ++index)
		{
			LayoutRoomNode node;
			node.Index = static_cast<size_t>(index);
			node.ParentIndex = index > 0 ? static_cast<size_t>(index - 1) : 0;
			node.DesiredDepth = index;
			node.DesiredBranch = 0;
			node.DesiredFloor = 0;
			node.StructuralRole = EDungeonRoomStructuralRole::Connector;
			node.GameplayRole = SelectMainPathGameplayRole(parameter, index, mainPathCount);

			if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically)
			{
				node.DesiredFloor = index;
			}
			const float progress = static_cast<float>(index) / static_cast<float>(std::max(1, mainPathCount - 1));
			node.ZoneIndex = SelectWeightedZoneIndex(parameter, progress, node.DesiredFloor);

			graph.Nodes.emplace_back(node);
			if (index > 0)
			{
				LayoutAisleEdge edge;
				edge.Room0 = static_cast<size_t>(index - 1);
				edge.Room1 = static_cast<size_t>(index);
				edge.Purpose = MakeMainPathPurpose(parameter, index - 1, mainPathCount);
				edge.bMainPath = true;
				graph.Edges.emplace_back(edge);
			}
		}

		graph.StartNodeIndex = 0;
		graph.GoalNodeIndex = static_cast<size_t>(mainPathCount - 1);
		const bool bGoalCanReceiveExtraRoutes = settings.ProgressionPolicy == EDungeonProgressionPolicy::FreeExploration;

		const int32 remainingRoomCount = roomCount - mainPathCount;
		for (int32 branchIndex = 0; branchIndex < remainingRoomCount; ++branchIndex)
		{
			const int32 nodeIndex = mainPathCount + branchIndex;
			const int32 parentIndex = SelectBranchParentIndex(parameter, mainPathCount, branchIndex, bGoalCanReceiveExtraRoutes);

			LayoutRoomNode node;
			node.Index = static_cast<size_t>(nodeIndex);
			node.ParentIndex = static_cast<size_t>(parentIndex);
			node.DesiredDepth = graph.Nodes[parentIndex].DesiredDepth + 1;
			node.DesiredBranch = branchIndex + 1;
			node.DesiredFloor = graph.Nodes[parentIndex].DesiredFloor;
			node.StructuralRole = EDungeonRoomStructuralRole::Branch;
			node.GameplayRole = SelectBranchGameplayRole(parameter, branchIndex);

			if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically)
			{
				node.DesiredFloor = nodeIndex;
			}
			const float progress = static_cast<float>(graph.Nodes[parentIndex].DesiredDepth) / static_cast<float>(std::max(1, mainPathCount - 1));
			node.ZoneIndex = SelectWeightedZoneIndex(parameter, progress, node.DesiredFloor);

			graph.Nodes.emplace_back(node);

			LayoutAisleEdge edge;
			edge.Room0 = static_cast<size_t>(parentIndex);
			edge.Room1 = static_cast<size_t>(nodeIndex);
			edge.Purpose = parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandVertically ? EDungeonAislePurpose::VerticalTransition : EDungeonAislePurpose::Branch;
			edge.bMainPath = false;
			graph.Edges.emplace_back(edge);
		}

		int32 loopCount = static_cast<int32>(std::round(static_cast<float>(roomCount) * effectiveLoopRouteDensity));
		loopCount = std::clamp(loopCount, 0, std::max(0, roomCount / 2));
		if (parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks)
		{
			loopCount = 0;
		}

		int32 attempts = 0;
		while (loopCount > 0 && attempts < roomCount * 4)
		{
			++attempts;
			const size_t room0 = static_cast<size_t>(parameter.GetRandom()->Get<int32>(0, roomCount));
			const size_t room1 = static_cast<size_t>(parameter.GetRandom()->Get<int32>(0, roomCount));
			if (!bGoalCanReceiveExtraRoutes && (room0 == graph.GoalNodeIndex || room1 == graph.GoalNodeIndex))
			{
				continue;
			}
			if (room0 == room1 || HasEdge(graph, room0, room1))
			{
				continue;
			}

			const int32 depthDistance = std::abs(graph.Nodes[room0].DesiredDepth - graph.Nodes[room1].DesiredDepth);
			if (depthDistance < 2)
			{
				continue;
			}

			LayoutAisleEdge edge;
			edge.Room0 = room0;
			edge.Room1 = room1;
			edge.Purpose = depthDistance >= 4 ? EDungeonAislePurpose::Shortcut : EDungeonAislePurpose::Loop;
			edge.bMainPath = false;
			graph.Edges.emplace_back(edge);
			--loopCount;
		}

		AssignStructuralRoles(parameter, graph);
		return graph;
	}

	EDungeonRoomGameplayRole LayoutGraphGenerator::SelectMainPathGameplayRole(const GenerateParameter& parameter, const int32 index, const int32 mainPathCount) noexcept
	{
		if (index == 0)
			return EDungeonRoomGameplayRole::None;
		if (index == mainPathCount - 1)
			return EDungeonRoomGameplayRole::None;
		const auto& path = parameter.GetPathSettings();
		if (path.ProgressionPolicy == EDungeonProgressionPolicy::BossRoute && index == mainPathCount - 2)
			return EDungeonRoomGameplayRole::Boss;
		if ((index % 5) == 0)
			return EDungeonRoomGameplayRole::Rest;
		if ((index % 3) == 0)
			return EDungeonRoomGameplayRole::Combat;
		return EDungeonRoomGameplayRole::None;
	}

	EDungeonRoomGameplayRole LayoutGraphGenerator::SelectBranchGameplayRole(const GenerateParameter& parameter, const int32 branchIndex) noexcept
	{
		const auto& profiles = GetRoomRoleProfiles(parameter);
		if (!profiles.IsEmpty())
		{
			const EDungeonRoomGameplayRole selectedRole = SelectWeightedGameplayRole(parameter, profiles);
			if (selectedRole != EDungeonRoomGameplayRole::None || std::any_of(profiles.begin(), profiles.end(), [](const FDungeonRoomRoleProfile& profile)
				{
					return profile.Role == EDungeonRoomGameplayRole::None && profile.BranchSelectionWeight > 0.f;
				}))
			{
				return selectedRole;
			}
		}

		const int32 roll = parameter.GetRandom()->Get<int32>(0, 100);
		if ((branchIndex % 5) == 4)
			return EDungeonRoomGameplayRole::Secret;
		if (roll < 45)
			return EDungeonRoomGameplayRole::Treasure;
		if (roll < 62)
			return EDungeonRoomGameplayRole::Puzzle;
		if (roll < 78)
			return EDungeonRoomGameplayRole::Rest;
		return EDungeonRoomGameplayRole::Combat;
	}

	void LayoutGraphGenerator::AssignStructuralRoles(const GenerateParameter& parameter, LayoutGraph& graph) noexcept
	{
		std::vector<int32> degree(graph.Nodes.size(), 0);
		for (const LayoutAisleEdge& edge : graph.Edges)
		{
			if (edge.Room0 < degree.size())
				++degree[edge.Room0];
			if (edge.Room1 < degree.size())
				++degree[edge.Room1];
		}

		for (size_t index = 0; index < graph.Nodes.size(); ++index)
		{
			LayoutRoomNode& node = graph.Nodes[index];
			if (index == graph.StartNodeIndex)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Start;
			}
			else if (index == graph.GoalNodeIndex)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Goal;
			}
			else if (parameter.GetPathSettings().ProgressionPolicy == EDungeonProgressionPolicy::HubQuest && node.DesiredBranch == 0 && node.DesiredDepth == 1)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Hub;
			}
			else if (degree[index] >= 3)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Hub;
			}
			else if (degree[index] <= 1)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::DeadEnd;
			}
			else if (node.DesiredBranch > 0)
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Branch;
			}
			else
			{
				node.StructuralRole = EDungeonRoomStructuralRole::Connector;
			}
		}
	}

	bool LayoutGraphGenerator::HasEdge(const LayoutGraph& graph, const size_t room0, const size_t room1) noexcept
	{
		return std::find_if(graph.Edges.begin(), graph.Edges.end(), [room0, room1](const LayoutAisleEdge& edge)
			{
				return
					(edge.Room0 == room0 && edge.Room1 == room1) ||
					(edge.Room0 == room1 && edge.Room1 == room0);
			}
		) != graph.Edges.end();
	}
}
