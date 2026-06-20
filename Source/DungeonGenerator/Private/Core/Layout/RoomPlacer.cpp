/**
 * Room placer for intent-driven dungeon layouts.
 *
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "RoomPlacer.h"
#include "../GenerateParameter.h"
#include "../Math/PerlinNoise.h"
#include "../Math/Random.h"
#include <algorithm>
#include <cmath>

namespace dungeon
{
	namespace
	{
		/*
		 * Returns the vertical distance used by initial vertical room placement.
		 * 垂直初期配置で使用する高さ方向の間隔を返します。
		 */
		int32 CalculateVerticalSpacing(const GenerateParameter& parameter) noexcept
		{
			return std::max<int32>(1, static_cast<int32>(parameter.GetMaxRoomHeight() + parameter.GetVerticalRoomMargin()));
		}

		/*
		 * Returns the sampling scale used by smooth Free-mode height placement.
		 * Freeモードの滑らかな高さ配置で使うサンプリングスケールを返します。
		 */
		float CalculateHeightFieldScale(const int32 spacing) noexcept
		{
			return static_cast<float>(std::max(1, spacing) * 4);
		}

		/*
		 * Returns the internal floor-layer count used by Free floor placement.
		 * Free床配置で使用する内部階層数を返します。
		 */
		int32 CalculateAutoFreeFloorCount(const GenerateParameter& parameter) noexcept
		{
			const int32 roomCount = static_cast<int32>(parameter.GetNumberOfCandidateRooms());
			return std::clamp((roomCount + 7) / 8, 1, 6);
		}

		/*
		 * Converts an XY location into a smooth floor index.
		 * XY座標を滑らかな階層インデックスへ変換します。
		 */
		int32 CalculateHeightFieldFloor(const GenerateParameter& parameter, const PerlinNoise& noise, const FIntVector& location, const int32 spacing, const float offsetX, const float offsetY) noexcept
		{
			const int32 floorCount = CalculateAutoFreeFloorCount(parameter);
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

		/*
		 * Returns a deterministic X offset used to loosen vertical initial placement.
		 * 垂直初期配置の密集を緩和するための決定的なX方向オフセットを返します。
		 */
		int32 CalculateVerticalXJitter(const GenerateParameter& parameter) noexcept
		{
			const auto maxRoomWidth = static_cast<int32>(parameter.GetMaxRoomWidth());
			return parameter.GetRandom()->Get<int32>(0, maxRoomWidth * 2 + 1) - maxRoomWidth;
		}

		/*
		 * Returns an unused vertical layer near the requested depth.
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
			FIntVector location;
			if (node.DesiredBranch == 0)
			{
				location = MakeMainPathLocation(parameter, node.DesiredDepth, spacing);
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

			auto room = std::make_shared<Room>(parameter, location);
			room->SetStructuralRole(node.StructuralRole);
			room->SetGameplayRole(node.GameplayRole);
			room->SetZoneIndex(node.ZoneIndex);
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

		return rooms;
	}

	int32 RoomPlacer::CalculateSpacing(const GenerateParameter& parameter) noexcept
	{
		const auto width = (parameter.GetMaxRoomWidth() + parameter.GetMinRoomWidth()) / 2;
		const auto depth = (parameter.GetMaxRoomDepth() + parameter.GetMinRoomDepth()) / 2;
		const auto margin = parameter.GetHorizontalRoomMargin();
		return std::max<int32>(1, std::max(width, depth) + margin);
	}

	FIntVector RoomPlacer::MakeMainPathLocation(const GenerateParameter& parameter, const int32 nodeIndex, const int32 spacing)
	{
		const auto jitter = std::max(1, spacing / 5);
		const int32 x = nodeIndex * spacing;
		const int32 turn = nodeIndex / 3;
		const int32 y = ((turn % 2) == 0 ? turn : -turn) * (spacing / 2);
		const int32 jitterX = parameter.GetRandom()->Get<int32>(0, jitter * 2 + 1) - jitter;
		const int32 jitterY = parameter.GetRandom()->Get<int32>(0, jitter * 2 + 1) - jitter;
		return FIntVector(
			x + jitterX,
			y + jitterY,
			0
		);
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
