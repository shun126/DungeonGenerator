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

#include "RoomPlacer.h"
#include "../GenerateParameter.h"
#include "../Math/PerlinNoise.h"
#include "../Math/Random.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace dungeon
{
	namespace
	{
		struct FWeightedZoneCandidate final
		{
			int32 Index = INDEX_NONE;
			float Weight = 0.f;
		};

		enum class EHorizontalDirection : uint8
		{
			East,
			South,
			West,
			North,
		};

		struct FMainPathPlacementCandidate final
		{
			FIntVector Location = FIntVector::ZeroValue;
			int32 CollisionCount = 0;
			int32 ImmediateReversePenalty = 0;
			double SpreadCost = 0.;
			double RandomScore = 0.;
		};

		/**
		 * Returns whether the next direction immediately reverses the previous main-path step.
		 * 次の方向が直前の主経路移動を即座に逆戻りする場合にtrueを返します。
		 */
		bool IsImmediateReverse(
			const std::vector<std::shared_ptr<Room>>& indexedRooms,
			const LayoutRoomNode& node,
			const EHorizontalDirection direction) noexcept
		{
			if (node.ParentIndex >= indexedRooms.size())
				return false;

			const auto& parentRoom = indexedRooms[node.ParentIndex];
			if (parentRoom == nullptr || node.ParentIndex == 0)
				return false;

			const auto& grandParentRoom = indexedRooms[node.ParentIndex - 1];
			if (grandParentRoom == nullptr)
				return false;

			const auto parentCenter = parentRoom->GetGroundCenter();
			const auto grandParentCenter = grandParentRoom->GetGroundCenter();
			const auto deltaX = parentCenter.X - grandParentCenter.X;
			const auto deltaY = parentCenter.Y - grandParentCenter.Y;
			if (std::abs(deltaX) >= std::abs(deltaY))
			{
				if (deltaX > 0.)
					return direction == EHorizontalDirection::West;
				if (deltaX < 0.)
					return direction == EHorizontalDirection::East;
			}
			else
			{
				if (deltaY > 0.)
					return direction == EHorizontalDirection::North;
				if (deltaY < 0.)
					return direction == EHorizontalDirection::South;
			}
			return false;
		}

		/**
		 * Places a room one configured margin away from its parent in a cardinal direction.
		 * 部屋を親部屋から設定余白だけ離した東西南北の位置へ配置します。
		 */
		FIntVector MakeCardinalLocation(
			const Room& parentRoom,
			const Room& room,
			const EHorizontalDirection direction,
			const int32 margin) noexcept
		{
			FIntVector location = FIntVector::ZeroValue;
			switch (direction)
			{
			case EHorizontalDirection::East:
				location.X = parentRoom.GetRight() + margin;
				location.Y = static_cast<int32>(std::round(parentRoom.GetCenter().Y - static_cast<float>(room.GetDepth()) * 0.5f));
				break;
			case EHorizontalDirection::South:
				location.X = static_cast<int32>(std::round(parentRoom.GetCenter().X - static_cast<float>(room.GetWidth()) * 0.5f));
				location.Y = parentRoom.GetBottom() + margin;
				break;
			case EHorizontalDirection::West:
				location.X = parentRoom.GetLeft() - margin - room.GetWidth();
				location.Y = static_cast<int32>(std::round(parentRoom.GetCenter().Y - static_cast<float>(room.GetDepth()) * 0.5f));
				break;
			case EHorizontalDirection::North:
				location.X = static_cast<int32>(std::round(parentRoom.GetCenter().X - static_cast<float>(room.GetWidth()) * 0.5f));
				location.Y = parentRoom.GetTop() - margin - room.GetDepth();
				break;
			}
			return location;
		}

		/**
		 * Returns whether the right placement candidate is preferred.
		 * 右側の配置候補を優先する場合にtrueを返します。
		 */
		bool IsBetterMainPathPlacement(
			const FMainPathPlacementCandidate& current,
			const FMainPathPlacementCandidate& next) noexcept
		{
			if (next.CollisionCount != current.CollisionCount)
				return next.CollisionCount < current.CollisionCount;
			if (next.ImmediateReversePenalty != current.ImmediateReversePenalty)
				return next.ImmediateReversePenalty < current.ImmediateReversePenalty;
			if (next.SpreadCost != current.SpreadCost)
				return next.SpreadCost < current.SpreadCost;
			return next.RandomScore > current.RandomScore;
		}

		/**
		 * Returns the sampling scale used by the smooth Free-mode height field.
		 * Freeモードの滑らかな高さ配置で使うサンプリングスケールを返します。
		 */
		float CalculateHeightFieldScale(const int32 spacing) noexcept
		{
			return static_cast<float>(std::max(1, spacing) * 4);
		}

		/**
		 * Converts an XY location into a smooth floor index.
		 * XY座標を滑らかな階層インデックスへ変換します。
		 */
		int32 CalculateHeightFieldFloor(const GenerateParameter& parameter, const PerlinNoise& noise, const FIntVector& location, const int32 spacing, const float offsetX, const float offsetY) noexcept
		{
			const int32 floorCount = RoomPlacer::CalculateAutoFreeFloorCount(parameter);
			if (floorCount <= 1)
			{
				return 0;
			}

			const float scale = CalculateHeightFieldScale(spacing);
			const float sampleX = static_cast<float>(location.X) / scale + offsetX;
			const float sampleY = static_cast<float>(location.Y) / scale + offsetY;
			const float noiseValue = noise.OctaveNoise(3, sampleX, sampleY);
			const float normalized = std::clamp((noiseValue + 1.f) * 0.5f, 0.f, 1.f);
			return std::clamp(static_cast<int32>(std::round(normalized * static_cast<float>(floorCount - 1))), 0, floorCount - 1);
		}

		/**
		 * Returns a deterministic X offset that reduces crowding in vertical layouts.
		 * 垂直初期配置の密集を緩和するための決定的なX方向オフセットを返します。
		 */
		int32 CalculateVerticalXJitter(const GenerateParameter& parameter) noexcept
		{
			const auto maxRoomWidth = static_cast<int32>(parameter.GetMaxRoomWidth());
			return parameter.GetRandom()->Get<int32>(0, maxRoomWidth * 2 + 1) - maxRoomWidth;
		}

		/**
		 * Returns an unused vertical layer near the requested graph depth.
		 * 指定された深さに近い未使用の垂直レイヤーを返します。
		 */
		int32 FindAvailableVerticalLayer(const LayoutRoomNode& node, const std::vector<int32>& usedLayers) noexcept
		{
			auto layer = node.DesiredDepth;
			if (node.DesiredBranch > 0)
			{
				layer += node.DesiredBranch;
			}

			while (std::find(usedLayers.begin(), usedLayers.end(), layer) != usedLayers.end())
			{
				++layer;
			}
			return layer;
		}

		/**
		 * Returns the route progress used to match a placed room to a zone.
		 * 配置済みの部屋をZoneに一致させるためのルート進行度を返します。
		 */
		float CalculateZoneProgress(const LayoutGraph& graph, const LayoutRoomNode& node, const int32 mainPathCount) noexcept
		{
			const auto progressDepth = node.DesiredBranch == 0 || node.ParentIndex >= graph.Nodes.size()
				? node.DesiredDepth
				: graph.Nodes[node.ParentIndex].DesiredDepth;
			return static_cast<float>(progressDepth) / static_cast<float>(std::max(1, mainPathCount - 1));
		}

		/**
		 * Selects a weighted zone matching the established route progress and floor.
		 * 確定したルート進行度と階層に一致するZoneを重み付きで選択します。
		 */
		int32 SelectWeightedZoneIndex(const FDungeonZoneSettings& zones, const float selectionRoll, const float progress, const int32 floor) noexcept
		{
			TArray<FWeightedZoneCandidate> candidates;
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

				candidates.Add({ index, weight });
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

			float roll = selectionRoll * totalWeight;
			for (const FWeightedZoneCandidate& candidate : candidates)
			{
				roll -= candidate.Weight;
				if (roll <= 0.f)
				{
					return candidate.Index;
				}
			}
			return INDEX_NONE;
		}
	}

	std::list<std::shared_ptr<Room>> RoomPlacer::Place(const GenerateParameter& parameter, LayoutGraph& graph)
	{
		std::list<std::shared_ptr<Room>> rooms;
		std::vector<std::shared_ptr<Room>> indexedRooms;
		std::vector<FIntVector> locations;
		std::vector<int32> usedVerticalLayers;
		indexedRooms.resize(graph.Nodes.size());
		locations.resize(graph.Nodes.size(), FIntVector::ZeroValue);
		usedVerticalLayers.reserve(graph.Nodes.size());

		const int32 spacing = CalculateSpacing(parameter);
		const int32 verticalSpacing = CalculateVerticalSpacing(parameter);
		auto heightRandom = std::make_shared<Random>(*parameter.GetRandom());
		PerlinNoise heightNoise(heightRandom);
		const float heightOffsetX = heightRandom->Get<float>() * 256.f;
		const float heightOffsetY = heightRandom->Get<float>() * 256.f;
		for (LayoutRoomNode& node : graph.Nodes)
		{
			auto room = std::make_shared<Room>(parameter, FIntVector::ZeroValue);
			FIntVector location;
			if (node.DesiredBranch == 0)
			{
				location = MakeMainPathLocation(parameter, indexedRooms, node, room);
			}
			else
			{
				location = MakeBranchLocation(parameter, graph, locations, node, spacing);
			}

			switch (parameter.GetExpansionPolicy())
			{
			case ExpansionPolicy::Flat:
				break;
			case ExpansionPolicy::ExpandVertically:
				{
					const auto layer = FindAvailableVerticalLayer(node, usedVerticalLayers);
					usedVerticalLayers.emplace_back(layer);
					location.X = CalculateVerticalXJitter(parameter);
					location.Z = layer * verticalSpacing;
				}
				location.Y = 0;
				break;
			case ExpansionPolicy::ExpandAnyDirection:
				node.DesiredFloor = CalculateHeightFieldFloor(parameter, heightNoise, location, spacing, heightOffsetX, heightOffsetY);
				location.Z = node.DesiredFloor * verticalSpacing;
				break;
			}
			node.DesiredFloor = location.Z / verticalSpacing;

			room->SetX(location.X);
			room->SetY(location.Y);
			room->SetZ(location.Z);
			room->SetStructuralRole(node.StructuralRole);
			room->SetGameplayRole(node.GameplayRole);
			room->SetMainPathRoom(node.DesiredBranch == 0);
			switch (node.StructuralRole)
			{
			case EDungeonRoomStructuralRole::Start:
				room->SetParts(Room::Parts::Start);
				break;
			case EDungeonRoomStructuralRole::Goal:
				room->SetParts(Room::Parts::Goal);
				break;
			default:
				room->SetParts(Room::Parts::Unidentified);
				break;
			}

			locations[node.Index] = location;
			indexedRooms[node.Index] = room;
			rooms.emplace_back(std::move(room));
		}

		Random zoneRandom(*parameter.GetRandom());
		for (LayoutRoomNode& node : graph.Nodes)
			node.ZoneSelectionRoll = zoneRandom.Get<float>();
		AssignZones(parameter, graph, rooms);

		return rooms;
	}

	void RoomPlacer::AssignZones(const GenerateParameter& parameter, LayoutGraph& graph, const std::list<std::shared_ptr<Room>>& rooms) noexcept
	{
		const int32 mainPathCount = static_cast<int32>(std::count_if(
			graph.Nodes.begin(),
			graph.Nodes.end(),
			[](const LayoutRoomNode& node)
			{
				return node.DesiredBranch == 0;
			}));
		const int32 verticalSpacing = CalculateVerticalSpacing(parameter);
		auto roomIterator = rooms.cbegin();
		for (LayoutRoomNode& node : graph.Nodes)
		{
			if (roomIterator == rooms.cend())
				break;

			const std::shared_ptr<Room>& room = *roomIterator++;
			if (room == nullptr)
				continue;

			node.DesiredFloor = room->GetZ() / verticalSpacing;
			const float progress = CalculateZoneProgress(graph, node, mainPathCount);
			node.ZoneIndex = SelectWeightedZoneIndex(parameter.GetZoneSettings(), node.ZoneSelectionRoll, progress, node.DesiredFloor);
			room->SetZoneIndex(node.ZoneIndex);
		}
	}

	int32 RoomPlacer::CalculateVerticalSpacing(const GenerateParameter& parameter) noexcept
	{
		return std::max<int32>(1, static_cast<int32>(parameter.GetMaxRoomHeight() + parameter.GetVerticalRoomMargin()));
	}

	int32 RoomPlacer::CalculateAutoFreeFloorCount(const GenerateParameter& parameter) noexcept
	{
		const int32 roomCount = static_cast<int32>(parameter.GetNumberOfCandidateRooms());
		return std::clamp((roomCount + 7) / 8, 1, 3);
	}

	int32 RoomPlacer::SnapToFloorOrigin(const GenerateParameter& parameter, const int32 z) noexcept
	{
		const int32 spacing = CalculateVerticalSpacing(parameter);
		auto layer = static_cast<int32>(std::round(static_cast<double>(z) / static_cast<double>(spacing)));
		if (parameter.GetExpansionPolicy() == ExpansionPolicy::Flat)
		{
			layer = 0;
		}
		else if (parameter.GetExpansionPolicy() == ExpansionPolicy::ExpandAnyDirection)
		{
			layer = std::clamp(layer, 0, CalculateAutoFreeFloorCount(parameter) - 1);
		}
		return layer * spacing;
	}

	int32 RoomPlacer::CalculateSpacing(const GenerateParameter& parameter) noexcept
	{
		const auto width = (parameter.GetMaxRoomWidth() + parameter.GetMinRoomWidth()) / 2;
		const auto depth = (parameter.GetMaxRoomDepth() + parameter.GetMinRoomDepth()) / 2;
		const auto margin = parameter.GetHorizontalRoomMargin();
		return std::max<int32>(1, std::max(width, depth) + margin);
	}

	FIntVector RoomPlacer::MakeMainPathLocation(
		const GenerateParameter& parameter,
		const std::vector<std::shared_ptr<Room>>& indexedRooms,
		const LayoutRoomNode& node,
		const std::shared_ptr<Room>& room)
	{
		if (node.DesiredDepth == 0 || node.ParentIndex >= indexedRooms.size() || indexedRooms[node.ParentIndex] == nullptr || room == nullptr)
			return FIntVector::ZeroValue;

		static constexpr std::array<EHorizontalDirection, 4> directions =
		{
			EHorizontalDirection::East,
			EHorizontalDirection::South,
			EHorizontalDirection::West,
			EHorizontalDirection::North,
		};

		const auto& parentRoom = indexedRooms[node.ParentIndex];
		const int32 margin = static_cast<int32>(parameter.GetHorizontalRoomMargin());
		auto hasBestCandidate = false;
		FMainPathPlacementCandidate bestCandidate;
		for (const auto direction : directions)
		{
			FMainPathPlacementCandidate candidate;
			candidate.Location = MakeCardinalLocation(*parentRoom, *room, direction, margin);
			candidate.ImmediateReversePenalty = IsImmediateReverse(indexedRooms, node, direction) ? 1 : 0;
			candidate.RandomScore = parameter.GetRandom()->Get<double>();

			Room candidateRoom(*room);
			candidateRoom.SetX(candidate.Location.X);
			candidateRoom.SetY(candidate.Location.Y);
			for (const auto& placedRoom : indexedRooms)
			{
				if (placedRoom == nullptr || placedRoom == parentRoom)
					continue;
				if (candidateRoom.HorizontalIntersect(*placedRoom, parameter.GetHorizontalRoomMargin()))
					++candidate.CollisionCount;
			}

			const auto center = candidateRoom.GetGroundCenter();
			candidate.SpreadCost = std::abs(static_cast<double>(center.X)) + std::abs(static_cast<double>(center.Y));
			if (!hasBestCandidate || IsBetterMainPathPlacement(bestCandidate, candidate))
			{
				bestCandidate = candidate;
				hasBestCandidate = true;
			}
		}
		return bestCandidate.Location;
	}

	FIntVector RoomPlacer::MakeBranchLocation(const GenerateParameter& parameter, const LayoutGraph& graph, const std::vector<FIntVector>& locations, const LayoutRoomNode& node, const int32 spacing)
	{
		const FIntVector parentLocation = locations[node.ParentIndex];
		const int32 side = (node.DesiredBranch % 2) == 0 ? 1 : -1;
		const int32 length = 1 + (node.DesiredBranch % 3);
		const int32 jitter = std::max(1, spacing / 6);
		const int32 xBias = graph.Nodes[node.ParentIndex].DesiredBranch == 0 ? spacing / 3 : spacing;
		const int32 jitterX = parameter.GetRandom()->Get<int32>(0, jitter * 2 + 1) - jitter;
		const int32 jitterY = parameter.GetRandom()->Get<int32>(0, jitter * 2 + 1) - jitter;
		return FIntVector(
			parentLocation.X + xBias + jitterX,
			parentLocation.Y + side * spacing * length + jitterY,
			0
		);
	}
}
