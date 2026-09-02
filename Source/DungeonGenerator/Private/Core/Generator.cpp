/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Generator.h"
#include "GenerateParameter.h"
#include "Debug/Config.h"
#include "Debug/Debug.h"
#include "Debug/MeasureTime.h"
#include "Helper/Finalizer.h"
#include "Math/Math.h"
#include "Math/VectorUtility.h"
#include "Layout/AislePlanner.h"
#include "Layout/LayoutEvaluator.h"
#include "Layout/LayoutGraphGenerator.h"
#include "Layout/RoomPlacer.h"
#include "PathGeneration/DelaunayTriangulation3D.h"
#include "PathGeneration/MinimumSpanningTree.h"
#include "PathGeneration/PathGoalCondition.h"
#include "Voxelization/RoomStructureGenerator.h"
#include "Voxelization/Voxel.h"

#include "MissionGraph/MissionGraph.h"
#include "MissionGraph/MissionGraphTester.h"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <queue>

namespace
{
	enum class ERoomSeparationAxis : uint8
	{
		X,
		Y,
		Z,
	};

	struct FRoomSeparationCandidate final
	{
		FIntVector Location = FIntVector::ZeroValue;
		uint32 CollisionCount = 0;
		uint64 CollisionDepth = 0;
		uint64 MaxCollisionDepth = 0;
		double MainPathSpacingDeviationCost = 0.;
		double MaxMainPathSpacingDeviationCost = 0.;
		double ConnectedDistanceCost = 0.;
		double MaxConnectedDistanceCost = 0.;
		double LayoutSpreadCost = 0.;
		double FixedRoomMoveCost = 0.;
		double DistanceSquared = 0.;
		double DirectionScore = 0.;
		double OuterScore = 0.;
		double RandomScore = 0.;
	};

	struct FRoomSpacingMargins final
	{
		uint8 Horizontal = 0;
		uint8 Vertical = 0;
	};

	/*
	 * 部屋の中心Yが0に最も近くなる整数Y座標を返します。
	 */
	int32_t GetNearestVerticalRoomY(const dungeon::Room& room) noexcept
	{
		return -room.GetDepth() / 2;
	}

	/*
	 * Applies floor-mode-specific constraints after choosing a separation candidate.
	 * 分離候補を選んだ後に、床モードごとの位置制約を適用します。
	 */
	void ApplySeparationConstraints(FIntVector& location, const dungeon::Room& room, const dungeon::GenerateParameter& parameter) noexcept
	{
		const auto expansionPolicy = parameter.GetExpansionPolicy();
		if (expansionPolicy == dungeon::ExpansionPolicy::Flat)
		{
			location.Z = 0;
		}
		else if (expansionPolicy == dungeon::ExpansionPolicy::ExpandVertically)
		{
			location.Y = GetNearestVerticalRoomY(room);
			location.Z = dungeon::RoomPlacer::SnapToFloorOrigin(parameter, location.Z);
		}
		else
		{
			location.Z = dungeon::RoomPlacer::SnapToFloorOrigin(parameter, location.Z);
		}
	}

	/*
	 * 部屋の現在位置と候補位置のグリッド距離の二乗を返します。
	 */
	double GetLocationDistanceSquared(const dungeon::Room& room, const FIntVector& location) noexcept
	{
		const auto dx = static_cast<double>(location.X - room.GetX());
		const auto dy = static_cast<double>(location.Y - room.GetY());
		const auto dz = static_cast<double>(location.Z - room.GetZ());
		return dx * dx + dy * dy + dz * dz;
	}

	/*
	 * Builds a direction used only to break ties between equally short separation candidates.
	 * 同じ移動距離の分離候補を選ぶときだけ使う方向を作成します。
	 */
	FVector MakeSeparationTieBreakDirection(const dungeon::Room& fixedRoom, const dungeon::Room& movableRoom, const bool activateOuterMovement) noexcept
	{
		auto direction = movableRoom.GetCenter() - fixedRoom.GetCenter();
		if (dungeon::math::IsZero(direction.SizeSquared()) == true)
		{
			direction = movableRoom.GetCenter();
		}
		if (activateOuterMovement)
		{
			auto outerDirection = movableRoom.GetCenter();
			if (outerDirection.Normalize() == true)
			{
				direction += outerDirection;
			}
		}
		direction.Normalize();
		return direction;
	}

	/*
	 * Scores how well a candidate follows the preferred separation direction.
	 * 候補位置が優先したい分離方向にどれだけ沿っているかを採点します。
	 */
	double GetCandidateDirectionScore(const dungeon::Room& room, const FIntVector& location, const FVector& direction) noexcept
	{
		const FVector offset(
			static_cast<double>(location.X - room.GetX()),
			static_cast<double>(location.Y - room.GetY()),
			static_cast<double>(location.Z - room.GetZ())
		);
		return FVector::DotProduct(offset, direction);
	}

	/*
	 * Scores outward movement from the origin when the separation process has stalled.
	 * 分離処理が停滞したときに、原点から外側へ向かう動きを採点します。
	 */
	double GetCandidateOuterScore(const dungeon::Room& room, const FIntVector& location, const bool activateOuterMovement) noexcept
	{
		if (activateOuterMovement == false)
		{
			return 0.;
		}

		const auto currentCenter = room.GetCenter();
		const FVector nextCenter(
			static_cast<double>(location.X) + static_cast<double>(room.GetWidth()) * 0.5,
			static_cast<double>(location.Y) + static_cast<double>(room.GetDepth()) * 0.5,
			static_cast<double>(location.Z) + static_cast<double>(room.GetHeight()) * 0.5
		);
		return nextCenter.SizeSquared() - currentCenter.SizeSquared();
	}

	/*
	 * 右側の候補を現在の最良候補として採用すべき場合にtrueを返します。
	 */
	bool IsBetterSeparationCandidate(const FRoomSeparationCandidate& current, const FRoomSeparationCandidate& next) noexcept
	{
		constexpr double epsilon = 1.e-6;
		if (next.CollisionCount < current.CollisionCount)
		{
			return true;
		}
		if (next.CollisionCount > current.CollisionCount)
		{
			return false;
		}
		if (next.CollisionDepth < current.CollisionDepth)
		{
			return true;
		}
		if (next.CollisionDepth > current.CollisionDepth)
		{
			return false;
		}
		if (next.MaxCollisionDepth < current.MaxCollisionDepth)
		{
			return true;
		}
		if (next.MaxCollisionDepth > current.MaxCollisionDepth)
		{
			return false;
		}
		if (next.MainPathSpacingDeviationCost < current.MainPathSpacingDeviationCost - epsilon)
		{
			return true;
		}
		if (next.MainPathSpacingDeviationCost > current.MainPathSpacingDeviationCost + epsilon)
		{
			return false;
		}
		if (next.MaxMainPathSpacingDeviationCost < current.MaxMainPathSpacingDeviationCost - epsilon)
		{
			return true;
		}
		if (next.MaxMainPathSpacingDeviationCost > current.MaxMainPathSpacingDeviationCost + epsilon)
		{
			return false;
		}
		if (next.ConnectedDistanceCost < current.ConnectedDistanceCost - epsilon)
		{
			return true;
		}
		if (next.ConnectedDistanceCost > current.ConnectedDistanceCost + epsilon)
		{
			return false;
		}
		if (next.MaxConnectedDistanceCost < current.MaxConnectedDistanceCost - epsilon)
		{
			return true;
		}
		if (next.MaxConnectedDistanceCost > current.MaxConnectedDistanceCost + epsilon)
		{
			return false;
		}
		if (next.FixedRoomMoveCost < current.FixedRoomMoveCost - epsilon)
		{
			return true;
		}
		if (next.FixedRoomMoveCost > current.FixedRoomMoveCost + epsilon)
		{
			return false;
		}
		if (next.LayoutSpreadCost < current.LayoutSpreadCost - epsilon)
		{
			return true;
		}
		if (next.LayoutSpreadCost > current.LayoutSpreadCost + epsilon)
		{
			return false;
		}
		if (next.DistanceSquared < current.DistanceSquared - epsilon)
		{
			return true;
		}
		if (next.DistanceSquared > current.DistanceSquared + epsilon)
		{
			return false;
		}
		if (next.DirectionScore > current.DirectionScore + epsilon)
		{
			return true;
		}
		if (next.DirectionScore < current.DirectionScore - epsilon)
		{
			return false;
		}
		if (next.OuterScore > current.OuterScore + epsilon)
		{
			return true;
		}
		if (next.OuterScore < current.OuterScore - epsilon)
		{
			return false;
		}
		return next.RandomScore > current.RandomScore;
	}

	/*
	 * 1つの軸で可動部屋を固定部屋のすぐ外側に置く分離候補を作成します。
	 */
	FRoomSeparationCandidate MakeSeparationCandidate(
		const dungeon::Room& fixedRoom,
		const dungeon::Room& movableRoom,
		const dungeon::GenerateParameter& parameter,
		const ERoomSeparationAxis axis,
		const bool positiveSide,
		const uint8 horizontalRoomMargin,
		const uint8 verticalRoomMargin,
		const FVector& direction,
		const bool activateOuterMovement,
		const double randomScore) noexcept
	{
		FIntVector location(movableRoom.GetX(), movableRoom.GetY(), movableRoom.GetZ());
		ApplySeparationConstraints(location, movableRoom, parameter);

		switch (axis)
		{
		case ERoomSeparationAxis::X:
			location.X = positiveSide ?
				fixedRoom.GetRight() + horizontalRoomMargin :
				fixedRoom.GetLeft() - horizontalRoomMargin - movableRoom.GetWidth();
			break;
		case ERoomSeparationAxis::Y:
			location.Y = positiveSide ?
				fixedRoom.GetBottom() + horizontalRoomMargin :
				fixedRoom.GetTop() - horizontalRoomMargin - movableRoom.GetDepth();
			break;
		case ERoomSeparationAxis::Z:
			location.Z = positiveSide ?
				fixedRoom.GetForeground() + verticalRoomMargin :
				fixedRoom.GetBackground() - verticalRoomMargin - movableRoom.GetHeight();
			break;
		default:
			checkNoEntry();
			break;
		}
		ApplySeparationConstraints(location, movableRoom, parameter);

		FRoomSeparationCandidate candidate;
		candidate.Location = location;
		candidate.DistanceSquared = GetLocationDistanceSquared(movableRoom, location);
		candidate.DirectionScore = GetCandidateDirectionScore(movableRoom, location, direction);
		candidate.OuterScore = GetCandidateOuterScore(movableRoom, location, activateOuterMovement);
		candidate.RandomScore = randomScore;
		return candidate;
	}

	/*
	 * 部屋分離で使用する実効余白を返します。
	 */
	FRoomSpacingMargins GetEffectiveRoomSeparationMargins(const dungeon::GenerateParameter& parameter, const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		FRoomSpacingMargins margins;
		margins.Horizontal = static_cast<uint8>(parameter.GetHorizontalRoomMargin());
		if (margins.Horizontal < room0.GetHorizontalRoomMargin())
			margins.Horizontal = room0.GetHorizontalRoomMargin();
		if (margins.Horizontal < room1.GetHorizontalRoomMargin())
			margins.Horizontal = room1.GetHorizontalRoomMargin();

		margins.Vertical = static_cast<uint8>(parameter.GetVerticalRoomMargin());
		if (margins.Vertical < room0.GetVerticalRoomMargin())
			margins.Vertical = room0.GetVerticalRoomMargin();
		if (margins.Vertical < room1.GetVerticalRoomMargin())
			margins.Vertical = room1.GetVerticalRoomMargin();
		return margins;
	}

	/*
	 * 1次元の部屋範囲が重なっている場合にtrueを返します。
	 */
	bool RoomIntervalsOverlap(const int32_t min0, const int32_t max0, const int32_t min1, const int32_t max1) noexcept
	{
		return max0 > min1 && min0 < max1;
	}

	/*
	 * 1次元範囲同士の重なり長さを返します。
	 */
	uint64 GetIntervalOverlapDepth(const int32_t min0, const int32_t max0, const int32_t min1, const int32_t max1) noexcept
	{
		const auto overlap = std::min(max0, max1) - std::max(min0, min1);
		return overlap > 0 ? static_cast<uint64>(overlap) : 0;
	}

	/*
	 * 1次元の部屋範囲同士にある空き距離を返します。
	 */
	int32_t CalculateIntervalGap(const int32_t min0, const int32_t max0, const int32_t min1, const int32_t max1) noexcept
	{
		if (max0 < min1)
			return min1 - max0;
		if (max1 < min0)
			return min0 - max1;
		return 0;
	}

	/*
	 * 通路距離を最小化するときに使う通路目的ごとの重みを返します。
	 */
	double GetAisleDistanceWeight(const dungeon::Aisle& aisle) noexcept
	{
		if (aisle.IsMain())
		{
			return 1.25;
		}

		double weight;
		switch (aisle.GetPurpose())
		{
		case EDungeonAislePurpose::Locked:
		case EDungeonAislePurpose::VerticalTransition:
			weight = 1.10;
			break;
		case EDungeonAislePurpose::Branch:
			weight = 0.85;
			break;
		case EDungeonAislePurpose::Loop:
		case EDungeonAislePurpose::Shortcut:
			weight = 0.65;
			break;
		case EDungeonAislePurpose::MainPath:
		default:
			weight = 1.00;
			break;
		}

		// 階層をまたぐ通路は経路が長くなりやすいので、目的ごとの重みへ上乗せします
		if (aisle.IsVerticalTransition())
		{
			weight *= 1.10;
		}
		return weight;
	}

	/*
	 * 各軸の部屋外周間ギャップ合計として通路距離を計算します。
	 */
	int32_t CalculateRoomPairAisleDistance(const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		return
			CalculateIntervalGap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight()) +
			CalculateIntervalGap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom()) +
			CalculateIntervalGap(room0.GetBackground(), room0.GetForeground(), room1.GetBackground(), room1.GetForeground());
	}

	/**
	 * Returns the horizontal gap between room bounds for main-path spacing evaluation.
	 * 主経路の均等配置評価に使う、部屋外周間の水平距離を返します。
	 */
	int32_t CalculateRoomPairHorizontalDistance(const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		return
			CalculateIntervalGap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight()) +
			CalculateIntervalGap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom());
	}

	/**
	 * Returns the target horizontal gap between rooms on the main path.
	 * 主経路で目標にする部屋外周間の水平距離を返します。
	 */
	int32_t GetMainPathTargetSpacing(const dungeon::GenerateParameter& parameter, const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		return std::max(
			static_cast<int32_t>(parameter.GetHorizontalRoomMargin()),
			std::max(static_cast<int32_t>(room0.GetHorizontalRoomMargin()), static_cast<int32_t>(room1.GetHorizontalRoomMargin()))
		);
	}

	/**
	 * Calculates the total and maximum horizontal spacing deviation across the main path.
	 * レイアウト全体の主経路水平間隔偏差と、その最大値を計算します。
	 */
	double CalculateLayoutMainPathSpacingDeviation(
		const std::vector<dungeon::Aisle>& aisles,
		const dungeon::GenerateParameter& parameter,
		double& outMaxDeviation) noexcept
	{
		double totalDeviation = 0.;
		outMaxDeviation = 0.;
		for (const auto& aisle : aisles)
		{
			if (!aisle.IsMain() || aisle.GetPoint(0) == nullptr || aisle.GetPoint(1) == nullptr)
				continue;

			const auto room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room0 == nullptr || room1 == nullptr)
				continue;

			const auto deviation = std::abs(static_cast<double>(
				CalculateRoomPairHorizontalDistance(*room0, *room1) -
				GetMainPathTargetSpacing(parameter, *room0, *room1)
			));
			totalDeviation += deviation;
			outMaxDeviation = std::max(outMaxDeviation, deviation);
		}
		return totalDeviation;
	}

	/**
	 * Calculates main-path spacing deviation when one room is moved to a candidate location.
	 * 指定部屋を候補位置へ置いた場合の接続主経路水平間隔偏差を計算します。
	 */
	double CalculateRoomMainPathSpacingDeviation(
		const std::vector<dungeon::Aisle>& aisles,
		const dungeon::GenerateParameter& parameter,
		const std::shared_ptr<dungeon::Room>& movingRoom,
		const FIntVector& location,
		double& outMaxDeviation) noexcept
	{
		outMaxDeviation = 0.;
		if (movingRoom == nullptr)
			return 0.;

		dungeon::Room candidateRoom(*movingRoom);
		candidateRoom.SetX(location.X);
		candidateRoom.SetY(location.Y);
		candidateRoom.SetZ(location.Z);

		double totalDeviation = 0.;
		for (const auto& aisle : aisles)
		{
			if (!aisle.IsMain() || aisle.GetPoint(0) == nullptr || aisle.GetPoint(1) == nullptr)
				continue;

			const auto room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto room1 = aisle.GetPoint(1)->GetOwnerRoom();
			std::shared_ptr<dungeon::Room> connectedRoom;
			if (room0 == movingRoom)
				connectedRoom = room1;
			else if (room1 == movingRoom)
				connectedRoom = room0;
			if (connectedRoom == nullptr)
				continue;

			const auto deviation = std::abs(static_cast<double>(
				CalculateRoomPairHorizontalDistance(candidateRoom, *connectedRoom) -
				GetMainPathTargetSpacing(parameter, candidateRoom, *connectedRoom)
			));
			totalDeviation += deviation;
			outMaxDeviation = std::max(outMaxDeviation, deviation);
		}
		return totalDeviation;
	}

	/*
	 * レイアウト最適化で部屋を動かしてはいけない場合にtrueを返します。
	 */
	bool IsFixedRoomForLayoutOptimization(const dungeon::Room& room, const dungeon::GenerateParameter& parameter) noexcept
	{
		if (room.GetParts() == dungeon::Room::Parts::Start && parameter.IsGenerateStartRoomReserved())
			return true;

		if (room.GetParts() != dungeon::Room::Parts::Goal || !parameter.IsGenerateGoalRoomReserved())
			return false;

		const FIntVector& goalRoomSize = parameter.GetGoalRoomSize();
		return room.GetWidth() == goalRoomSize.X &&
			room.GetDepth() == goalRoomSize.Y &&
			room.GetHeight() == goalRoomSize.Z;
	}

	/**
	 * Changes a room size while keeping its ground center on the nearest grid-aligned position.
	 * 部屋の床面中心を最も近いグリッド位置に維持しながら、部屋サイズを変更します。
	 */
	void SetRoomSizePreservingGroundCenter(dungeon::Room& room, const FIntVector& size) noexcept
	{
		const int32 centerXTwice = room.GetX() * 2 + room.GetWidth();
		const int32 centerYTwice = room.GetY() * 2 + room.GetDepth();
		room.SetX(FMath::FloorToInt(static_cast<double>(centerXTwice - size.X) * 0.5));
		room.SetY(FMath::FloorToInt(static_cast<double>(centerYTwice - size.Y) * 0.5));
		room.SetWidth(size.X);
		room.SetDepth(size.Y);
		room.SetHeight(size.Z);
	}

	/*
	 * 移動可能だが不要なずれを避けたい部屋の緩い移動ペナルティを返します。
	 */
	double GetRoomMovePenalty(const dungeon::Room& room) noexcept
	{
		double penalty = 1.;
		if (room.IsValidReservationNumber())
		{
			penalty += 0.35;
		}
		if (room.GetParts() == dungeon::Room::Parts::Goal)
		{
			penalty += 0.15;
		}
		if (room.IsMainPathRoom())
		{
			penalty += 0.10;
		}
		return penalty;
	}

	/*
	 * movingRoomに通路で接続されている反対側の部屋を返します。
	 */
	std::shared_ptr<dungeon::Room> GetConnectedRoom(const dungeon::Aisle& aisle, const std::shared_ptr<dungeon::Room>& movingRoom) noexcept
	{
		if (movingRoom == nullptr || aisle.GetPoint(0) == nullptr || aisle.GetPoint(1) == nullptr)
		{
			return nullptr;
		}

		const auto room0 = aisle.GetPoint(0)->GetOwnerRoom();
		const auto room1 = aisle.GetPoint(1)->GetOwnerRoom();
		if (room0 == movingRoom)
		{
			return room1;
		}
		if (room1 == movingRoom)
		{
			return room0;
		}
		return nullptr;
	}

	/*
	 * レイアウト内の全通路に対する重み付き距離コストを計算します。
	 */
	double CalculateLayoutAisleDistanceCost(const std::vector<dungeon::Aisle>& aisles) noexcept
	{
		double cost = 0.;
		for (const auto& aisle : aisles)
		{
			if (aisle.GetPoint(0) == nullptr || aisle.GetPoint(1) == nullptr)
				continue;

			const auto room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room0 == nullptr || room1 == nullptr)
				continue;

			cost += static_cast<double>(CalculateRoomPairAisleDistance(*room0, *room1)) * GetAisleDistanceWeight(aisle);
		}
		return cost;
	}

	/*
	 * 1つの部屋を候補位置に置いた場合の接続通路距離コストを計算します。
	 */
	double CalculateRoomConnectedDistanceCost(const std::vector<dungeon::Aisle>& aisles, const std::shared_ptr<dungeon::Room>& movingRoom, const FIntVector& location) noexcept
	{
		if (movingRoom == nullptr)
			return 0.;

		dungeon::Room candidateRoom(*movingRoom);
		candidateRoom.SetX(location.X);
		candidateRoom.SetY(location.Y);
		candidateRoom.SetZ(location.Z);

		double cost = 0.;
		for (const auto& aisle : aisles)
		{
			const auto connectedRoom = GetConnectedRoom(aisle, movingRoom);
			if (connectedRoom == nullptr)
				continue;

			cost += static_cast<double>(CalculateRoomPairAisleDistance(candidateRoom, *connectedRoom)) * GetAisleDistanceWeight(aisle);
		}
		return cost;
	}

	/*
	 * 1つの部屋を候補位置に置いた場合の最大接続通路距離を計算します。
	 */
	double CalculateMaxConnectedDistanceCost(const std::vector<dungeon::Aisle>& aisles, const std::shared_ptr<dungeon::Room>& movingRoom, const FIntVector& location) noexcept
	{
		if (movingRoom == nullptr)
			return 0.;

		dungeon::Room candidateRoom(*movingRoom);
		candidateRoom.SetX(location.X);
		candidateRoom.SetY(location.Y);
		candidateRoom.SetZ(location.Z);

		double maxCost = 0.;
		for (const auto& aisle : aisles)
		{
			const auto connectedRoom = GetConnectedRoom(aisle, movingRoom);
			if (connectedRoom == nullptr)
				continue;

			maxCost = std::max(maxCost, static_cast<double>(CalculateRoomPairAisleDistance(candidateRoom, *connectedRoom)) * GetAisleDistanceWeight(aisle));
		}
		return maxCost;
	}

	/*
	 * 余白で拡張した候補部屋と別の部屋の重なり体積を返します。
	 */
	uint64 GetRoomCollisionDepth(const dungeon::Room& candidateRoom, const dungeon::Room& otherRoom, const FRoomSpacingMargins margins) noexcept
	{
		const auto overlapX = GetIntervalOverlapDepth(candidateRoom.GetLeft() - margins.Horizontal, candidateRoom.GetRight() + margins.Horizontal, otherRoom.GetLeft(), otherRoom.GetRight());
		const auto overlapY = GetIntervalOverlapDepth(candidateRoom.GetTop() - margins.Horizontal, candidateRoom.GetBottom() + margins.Horizontal, otherRoom.GetTop(), otherRoom.GetBottom());
		const auto overlapZ = GetIntervalOverlapDepth(candidateRoom.GetBackground() - margins.Vertical, candidateRoom.GetForeground() + margins.Vertical, otherRoom.GetBackground(), otherRoom.GetForeground());
		return overlapX * overlapY * overlapZ;
	}

	/**
	 * Scores candidate collisions after applying every placement constraint.
	 * すべての配置制約を適用した後の候補位置の交差を採点します。
	 */
	void ScoreSeparationCandidateCollisions(
		FRoomSeparationCandidate& candidate,
		const dungeon::GenerateParameter& parameter,
		const std::list<std::shared_ptr<dungeon::Room>>& rooms,
		const std::shared_ptr<dungeon::Room>& movableRoom) noexcept
	{
		dungeon::Room candidateRoom(*movableRoom);
		candidateRoom.SetX(candidate.Location.X);
		candidateRoom.SetY(candidate.Location.Y);
		candidateRoom.SetZ(candidate.Location.Z);

		candidate.CollisionCount = 0;
		candidate.CollisionDepth = 0;
		candidate.MaxCollisionDepth = 0;
		for (const auto& otherRoom : rooms)
		{
			if (otherRoom == nullptr || otherRoom == movableRoom)
				continue;

			const auto margins = GetEffectiveRoomSeparationMargins(parameter, candidateRoom, *otherRoom);
			if (candidateRoom.Intersect(*otherRoom, margins.Horizontal, margins.Vertical))
			{
				++candidate.CollisionCount;
				const auto collisionDepth = GetRoomCollisionDepth(candidateRoom, *otherRoom, margins);
				candidate.CollisionDepth += collisionDepth;
				candidate.MaxCollisionDepth = std::max(candidate.MaxCollisionDepth, collisionDepth);
			}
		}
	}

	struct FLayoutCollisionScore final
	{
		uint32 CollisionCount = 0;
		uint64 CollisionDepth = 0;
		uint64 MaxCollisionDepth = 0;
	};

	struct FRoomCollisionEdge final
	{
		std::shared_ptr<dungeon::Room> Room0;
		std::shared_ptr<dungeon::Room> Room1;
	};

	/**
	 * Returns true when the next hard-constraint score strictly improves the current score.
	 * 次のハード制約スコアが現在のスコアより厳密に改善する場合にtrueを返します。
	 */
	bool IsBetterCollisionScore(const FLayoutCollisionScore& current, const FLayoutCollisionScore& next) noexcept
	{
		if (next.CollisionCount != current.CollisionCount)
			return next.CollisionCount < current.CollisionCount;
		if (next.CollisionDepth != current.CollisionDepth)
			return next.CollisionDepth < current.CollisionDepth;
		return next.MaxCollisionDepth < current.MaxCollisionDepth;
	}

	/**
	 * Returns true when two hard-constraint scores are identical.
	 * 2つのハード制約スコアが同一の場合にtrueを返します。
	 */
	bool IsSameCollisionScore(const FLayoutCollisionScore& score0, const FLayoutCollisionScore& score1) noexcept
	{
		return
			score0.CollisionCount == score1.CollisionCount &&
			score0.CollisionDepth == score1.CollisionDepth &&
			score0.MaxCollisionDepth == score1.MaxCollisionDepth;
	}

	/**
	 * Collects every colliding room pair and calculates the global hard-constraint score.
	 * 衝突している全部屋ペアを収集し、全体のハード制約スコアを計算します。
	 */
	FLayoutCollisionScore CollectRoomCollisions(
		const dungeon::GenerateParameter& parameter,
		const std::vector<std::shared_ptr<dungeon::Room>>& rooms,
		std::vector<FRoomCollisionEdge>* outEdges = nullptr) noexcept
	{
		FLayoutCollisionScore score;
		if (outEdges != nullptr)
		{
			outEdges->clear();
		}

		for (size_t roomIndex = 0; roomIndex < rooms.size(); ++roomIndex)
		{
			const auto& room0 = rooms[roomIndex];
			if (room0 == nullptr)
				continue;

			for (size_t otherRoomIndex = roomIndex + 1; otherRoomIndex < rooms.size(); ++otherRoomIndex)
			{
				const auto& room1 = rooms[otherRoomIndex];
				if (room1 == nullptr)
					continue;

				const auto margins = GetEffectiveRoomSeparationMargins(parameter, *room0, *room1);
				if (!room0->Intersect(*room1, margins.Horizontal, margins.Vertical))
					continue;

				const auto collisionDepth = GetRoomCollisionDepth(*room0, *room1, margins);
				++score.CollisionCount;
				score.CollisionDepth += collisionDepth;
				score.MaxCollisionDepth = std::max(score.MaxCollisionDepth, collisionDepth);
				if (outEdges != nullptr)
				{
					outEdges->emplace_back(FRoomCollisionEdge{ room0, room1 });
				}
			}
		}
		return score;
	}

	/**
	 * Calculates the maximum collision depth among pairs that do not contain the excluded room.
	 * 除外部屋を含まないペアの最大衝突量を計算します。
	 */
	uint64 CalculateUnaffectedMaximumCollisionDepth(
		const dungeon::GenerateParameter& parameter,
		const std::vector<std::shared_ptr<dungeon::Room>>& rooms,
		const std::shared_ptr<dungeon::Room>& excludedRoom) noexcept
	{
		uint64 maximumDepth = 0;
		for (size_t roomIndex = 0; roomIndex < rooms.size(); ++roomIndex)
		{
			const auto& room0 = rooms[roomIndex];
			if (room0 == nullptr || room0 == excludedRoom)
				continue;

			for (size_t otherRoomIndex = roomIndex + 1; otherRoomIndex < rooms.size(); ++otherRoomIndex)
			{
				const auto& room1 = rooms[otherRoomIndex];
				if (room1 == nullptr || room1 == excludedRoom)
					continue;

				const auto margins = GetEffectiveRoomSeparationMargins(parameter, *room0, *room1);
				if (room0->Intersect(*room1, margins.Horizontal, margins.Vertical))
				{
					maximumDepth = std::max(maximumDepth, GetRoomCollisionDepth(*room0, *room1, margins));
				}
			}
		}
		return maximumDepth;
	}

	/**
	 * Appends a constrained location unless the same location is already present.
	 * 制約適用後の座標が未登録の場合だけ候補へ追加します。
	 */
	void AppendUniqueSeparationLocation(
		std::vector<FIntVector>& locations,
		FIntVector location,
		const dungeon::Room& room,
		const dungeon::GenerateParameter& parameter)
	{
		ApplySeparationConstraints(location, room, parameter);
		if (std::find(locations.begin(), locations.end(), location) == locations.end())
		{
			locations.emplace_back(location);
		}
	}

	/**
	 * Expands a horizontal candidate across every valid Free floor.
	 * 水平候補をFreeモードの全有効階層へ展開します。
	 */
	void AppendFloorLocationVariants(
		std::vector<FIntVector>& locations,
		const FIntVector& baseLocation,
		const dungeon::Room& room,
		const dungeon::GenerateParameter& parameter)
	{
		if (parameter.GetExpansionPolicy() != dungeon::ExpansionPolicy::ExpandAnyDirection)
		{
			AppendUniqueSeparationLocation(locations, baseLocation, room, parameter);
			return;
		}

		const int32 floorCount = dungeon::RoomPlacer::CalculateAutoFreeFloorCount(parameter);
		const int32 floorSpacing = dungeon::RoomPlacer::CalculateVerticalSpacing(parameter);
		for (int32 floorIndex = 0; floorIndex < floorCount; ++floorIndex)
		{
			FIntVector location = baseLocation;
			location.Z = floorIndex * floorSpacing;
			AppendUniqueSeparationLocation(locations, location, room, parameter);
		}
	}

	/**
	 * Returns true when a room can be placed at the location without colliding with placed rooms.
	 * 指定座標の部屋が配置済み部屋と衝突しない場合にtrueを返します。
	 */
	bool IsCollisionFreeLocation(
		const dungeon::GenerateParameter& parameter,
		const std::shared_ptr<dungeon::Room>& room,
		const FIntVector& location,
		const std::vector<std::shared_ptr<dungeon::Room>>& placedRooms) noexcept
	{
		dungeon::Room candidateRoom(*room);
		candidateRoom.SetX(location.X);
		candidateRoom.SetY(location.Y);
		candidateRoom.SetZ(location.Z);
		for (const auto& otherRoom : placedRooms)
		{
			if (otherRoom == nullptr || otherRoom == room)
				continue;

			const auto margins = GetEffectiveRoomSeparationMargins(parameter, candidateRoom, *otherRoom);
			if (candidateRoom.Intersect(*otherRoom, margins.Horizontal, margins.Vertical))
				return false;
		}
		return true;
	}

	/**
	 * Calculates a deterministic hash from room identifiers and locations.
	 * 部屋識別子と座標から決定的なハッシュを計算します。
	 */
	uint64 CalculateRoomLocationHash(const std::vector<std::shared_ptr<dungeon::Room>>& rooms) noexcept
	{
		uint64 hash = 1469598103934665603ull;
		const auto mix = [&hash](const uint32 value)
			{
				hash ^= value;
				hash *= 1099511628211ull;
			};
		for (const auto& room : rooms)
		{
			if (room == nullptr)
				continue;
			mix(static_cast<uint16_t>(room->GetIdentifier()));
			mix(static_cast<uint32>(room->GetX()));
			mix(static_cast<uint32>(room->GetY()));
			mix(static_cast<uint32>(room->GetZ()));
		}
		return hash;
	}

	void ScoreSeparationCandidateAisleDistance(
		FRoomSeparationCandidate& candidate,
		const dungeon::GenerateParameter& parameter,
		const std::vector<dungeon::Aisle>& aisles,
		const std::shared_ptr<dungeon::Room>& movingRoom) noexcept;

	enum class ECollisionResolutionStatus : uint8
	{
		Completed,
		ImmutableConflict,
		FallbackFailed,
	};

	/**
	 * Identifies the exact stage at which deterministic fallback stopped resolving collisions.
	 * 決定的フォールバックが衝突を解消できなくなった正確な段階を示します。
	 */
	enum class ECollisionFallbackFailureReason : uint8
	{
		None,
		NoCandidate,
		PostValidationCollision,
		RetryLimit,
	};

	/**
	 * Accumulates collision-resolution work and preserves the final failure context for logging.
	 * 衝突解消の処理量と、ログ出力に必要な最終失敗時の情報を保持します。
	 */
	struct FCollisionResolutionDiagnostics final
	{
		bool Moved = false;
		size_t LocalMoveCount = 0;
		size_t RepackedRoomCount = 0;
		size_t MaximumCollisionGroupSize = 0;
		size_t FallbackRoundCount = 0;
		ECollisionFallbackFailureReason FailureReason = ECollisionFallbackFailureReason::None;
		std::shared_ptr<dungeon::Room> FailedRoom;
		std::vector<FRoomCollisionEdge> RemainingCollisionEdges;
	};

	/**
	 * Counts the aisles connected to one room.
	 * 1つの部屋へ接続している通路数を返します。
	 */
	size_t CountRoomConnections(
		const std::vector<dungeon::Aisle>& aisles,
		const std::shared_ptr<dungeon::Room>& room) noexcept
	{
		return static_cast<size_t>(std::count_if(aisles.begin(), aisles.end(), [&room](const dungeon::Aisle& aisle)
			{
				return GetConnectedRoom(aisle, room) != nullptr;
			}
		));
	}

	/**
	 * Builds deterministic connected components from the current collision graph.
	 * 現在の衝突グラフから決定的な連結成分を作成します。
	 */
	std::vector<std::vector<size_t>> BuildCollisionComponents(
		const std::vector<std::shared_ptr<dungeon::Room>>& rooms,
		const std::vector<FRoomCollisionEdge>& collisionEdges)
	{
		std::unordered_map<const dungeon::Room*, size_t> roomIndices;
		for (size_t roomIndex = 0; roomIndex < rooms.size(); ++roomIndex)
		{
			roomIndices.emplace(rooms[roomIndex].get(), roomIndex);
		}

		std::vector<std::vector<size_t>> adjacency(rooms.size());
		std::vector<uint8> participates(rooms.size(), 0);
		/*
		 * Room vector indices are used as graph vertices so traversal order remains independent of pointers.
		 * ポインター値に依存しない走査順を維持するため、部屋配列のインデックスをグラフ頂点に使用します。
		 */
		for (const auto& edge : collisionEdges)
		{
			const auto room0Index = roomIndices.find(edge.Room0.get());
			const auto room1Index = roomIndices.find(edge.Room1.get());
			if (room0Index == roomIndices.end() || room1Index == roomIndices.end())
				continue;

			adjacency[room0Index->second].emplace_back(room1Index->second);
			adjacency[room1Index->second].emplace_back(room0Index->second);
			participates[room0Index->second] = 1;
			participates[room1Index->second] = 1;
		}

		std::vector<std::vector<size_t>> components;
		std::vector<uint8> visited(rooms.size(), 0);
		/*
		 * Breadth-first traversal starts in room order, making component construction deterministic.
		 * 部屋順に幅優先探索を開始することで、衝突成分を決定的な順序で構築します。
		 */
		for (size_t firstRoomIndex = 0; firstRoomIndex < rooms.size(); ++firstRoomIndex)
		{
			if (participates[firstRoomIndex] == 0 || visited[firstRoomIndex] != 0)
				continue;

			auto& component = components.emplace_back();
			std::queue<size_t> pending;
			pending.emplace(firstRoomIndex);
			visited[firstRoomIndex] = 1;
			while (!pending.empty())
			{
				const size_t roomIndex = pending.front();
				pending.pop();
				component.emplace_back(roomIndex);
				for (const size_t adjacentIndex : adjacency[roomIndex])
				{
					if (visited[adjacentIndex] == 0)
					{
						visited[adjacentIndex] = 1;
						pending.emplace(adjacentIndex);
					}
				}
			}
		}
		return components;
	}

	/**
	 * Appends contact positions around one placed room for every permitted axis.
	 * 配置済み部屋の外周へ、許可された全軸の接触候補を追加します。
	 */
	void AppendContactLocations(
		std::vector<FIntVector>& locations,
		const dungeon::GenerateParameter& parameter,
		const dungeon::Room& anchorRoom,
		const dungeon::Room& movingRoom)
	{
		const auto margins = GetEffectiveRoomSeparationMargins(parameter, anchorRoom, movingRoom);
		const auto direction = MakeSeparationTieBreakDirection(anchorRoom, movingRoom, false);
		const auto appendAxis = [&](const ERoomSeparationAxis axis)
			{
				for (const bool positiveSide : { true, false })
				{
					const auto candidate = MakeSeparationCandidate(
						anchorRoom,
						movingRoom,
						parameter,
						axis,
						positiveSide,
						margins.Horizontal,
						margins.Vertical,
						direction,
						false,
						0.);
					AppendFloorLocationVariants(locations, candidate.Location, movingRoom, parameter);
				}
			};

		appendAxis(ERoomSeparationAxis::X);
		if (parameter.GetExpansionPolicy() != dungeon::ExpansionPolicy::ExpandVertically)
			appendAxis(ERoomSeparationAxis::Y);
		if (parameter.GetExpansionPolicy() == dungeon::ExpansionPolicy::ExpandAnyDirection)
			appendAxis(ERoomSeparationAxis::Z);
	}

	/**
	 * Appends positions beyond the occupied bounds; at least one of them is collision-free.
	 * 占有範囲の外側へ、少なくとも1つは衝突しない候補を追加します。
	 */
	void AppendGuaranteedOuterLocations(
		std::vector<FIntVector>& locations,
		const dungeon::GenerateParameter& parameter,
		const std::shared_ptr<dungeon::Room>& movingRoom,
		const std::vector<std::shared_ptr<dungeon::Room>>& placedRooms)
	{
		if (placedRooms.empty())
			return;

		int32 minimumLeft = std::numeric_limits<int32>::max();
		int32 maximumRight = std::numeric_limits<int32>::lowest();
		int32 minimumTop = std::numeric_limits<int32>::max();
		int32 maximumBottom = std::numeric_limits<int32>::lowest();
		int32 maximumHorizontalMargin = 0;
		/*
		 * Use the largest effective margin so every outer candidate clears every already placed room.
		 * 最大の実効余白を使用し、外周候補が配置済みの全部屋から確実に離れるようにします。
		 */
		for (const auto& placedRoom : placedRooms)
		{
			minimumLeft = std::min(minimumLeft, placedRoom->GetLeft());
			maximumRight = std::max(maximumRight, placedRoom->GetRight());
			minimumTop = std::min(minimumTop, placedRoom->GetTop());
			maximumBottom = std::max(maximumBottom, placedRoom->GetBottom());
			maximumHorizontalMargin = std::max<int32>(
				maximumHorizontalMargin,
				GetEffectiveRoomSeparationMargins(parameter, *movingRoom, *placedRoom).Horizontal);
		}

		FIntVector location(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
		location.X = maximumRight + maximumHorizontalMargin;
		AppendFloorLocationVariants(locations, location, *movingRoom, parameter);
		location.X = minimumLeft - maximumHorizontalMargin - movingRoom->GetWidth();
		AppendFloorLocationVariants(locations, location, *movingRoom, parameter);
		if (parameter.GetExpansionPolicy() != dungeon::ExpansionPolicy::ExpandVertically)
		{
			location = FIntVector(movingRoom->GetX(), maximumBottom + maximumHorizontalMargin, movingRoom->GetZ());
			AppendFloorLocationVariants(locations, location, *movingRoom, parameter);
			location.Y = minimumTop - maximumHorizontalMargin - movingRoom->GetDepth();
			AppendFloorLocationVariants(locations, location, *movingRoom, parameter);
		}
	}

	/**
	 * Rebuilds a stalled collision component without moving immutable rooms.
	 * 停滞した衝突成分を固定部屋を動かさずに再配置します。
	 */
	bool RepackCollisionComponent(
		const dungeon::GenerateParameter& parameter,
		const std::vector<dungeon::Aisle>& aisles,
		const std::vector<std::shared_ptr<dungeon::Room>>& rooms,
		const std::vector<size_t>& component,
		FCollisionResolutionDiagnostics& diagnostics)
	{
		std::unordered_set<const dungeon::Room*> componentRooms;
		for (const size_t roomIndex : component)
		{
			componentRooms.emplace(rooms[roomIndex].get());
		}

		std::vector<std::shared_ptr<dungeon::Room>> placedRooms;
		std::vector<std::shared_ptr<dungeon::Room>> movableRooms;
		for (const auto& room : rooms)
		{
			if (
				componentRooms.find(room.get()) == componentRooms.end() ||
				IsFixedRoomForLayoutOptimization(*room, parameter))
			{
				placedRooms.emplace_back(room);
			}
			else
			{
				movableRooms.emplace_back(room);
			}
		}

		/*
		 * Place structurally important rooms first, then use the identifier as a stable final tie-breaker.
		 * 構造上重要な部屋から配置し、最後は識別子で安定した優先順位を決定します。
		 */
		std::stable_sort(movableRooms.begin(), movableRooms.end(), [&aisles](const auto& room0, const auto& room1)
			{
				if (room0->IsMainPathRoom() != room1->IsMainPathRoom())
					return room0->IsMainPathRoom();
				const size_t connections0 = CountRoomConnections(aisles, room0);
				const size_t connections1 = CountRoomConnections(aisles, room1);
				if (connections0 != connections1)
					return connections0 > connections1;
				return static_cast<uint16_t>(room0->GetIdentifier()) < static_cast<uint16_t>(room1->GetIdentifier());
			}
		);

		for (const auto& movingRoom : movableRooms)
		{
			std::vector<FIntVector> locations;
			/*
			 * Candidate order is fixed: current position, contact positions, then guaranteed outer positions.
			 * 候補順は現在位置、接触位置、外周位置の順に固定し、同一Seedの再現性を維持します。
			 */
			AppendFloorLocationVariants(
				locations,
				FIntVector(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ()),
				*movingRoom,
				parameter);
			for (const auto& placedRoom : placedRooms)
			{
				AppendContactLocations(locations, parameter, *placedRoom, *movingRoom);
			}
			AppendGuaranteedOuterLocations(locations, parameter, movingRoom, placedRooms);

			auto hasBestCandidate = false;
			FRoomSeparationCandidate bestCandidate;
			/*
			 * Validate against every placed room with pair-specific effective margins before scoring.
			 * スコア計算前に、部屋ペアごとの実効余白を含めて配置済みの全部屋との非交差を検証します。
			 */
			for (const auto& location : locations)
			{
				if (!IsCollisionFreeLocation(parameter, movingRoom, location, placedRooms))
					continue;

				FRoomSeparationCandidate candidate;
				candidate.Location = location;
				candidate.DistanceSquared = GetLocationDistanceSquared(*movingRoom, location);
				ScoreSeparationCandidateAisleDistance(candidate, parameter, aisles, movingRoom);
				if (!hasBestCandidate || IsBetterSeparationCandidate(bestCandidate, candidate))
				{
					hasBestCandidate = true;
					bestCandidate = candidate;
				}
			}
			if (!hasBestCandidate)
			{
				diagnostics.FailureReason = ECollisionFallbackFailureReason::NoCandidate;
				diagnostics.FailedRoom = movingRoom;
				return false;
			}

			movingRoom->SetX(bestCandidate.Location.X);
			movingRoom->SetY(bestCandidate.Location.Y);
			movingRoom->SetZ(bestCandidate.Location.Z);
			placedRooms.emplace_back(movingRoom);
			diagnostics.Moved = true;
			++diagnostics.RepackedRoomCount;
		}
		return true;
	}

	/**
	 * Resolves room collisions with monotonic local moves and deterministic component repacking.
	 * 単調改善する局所移動と決定的な成分再配置で部屋衝突を解消します。
	 */
	ECollisionResolutionStatus ResolveRoomCollisionsRobust(
		const dungeon::GenerateParameter& parameter,
		const std::list<std::shared_ptr<dungeon::Room>>& roomList,
		const std::vector<dungeon::Aisle>& aisles,
		FCollisionResolutionDiagnostics& diagnostics)
	{
		std::vector<std::shared_ptr<dungeon::Room>> rooms(roomList.begin(), roomList.end());
		std::stable_sort(rooms.begin(), rooms.end(), [](const auto& room0, const auto& room1)
			{
				return static_cast<uint16_t>(room0->GetIdentifier()) < static_cast<uint16_t>(room1->GetIdentifier());
			}
		);

		std::vector<FRoomCollisionEdge> collisionEdges;
		auto collisionScore = CollectRoomCollisions(parameter, rooms, &collisionEdges);

		/*
		 * A collision between two immutable rooms cannot be repaired by moving anything else, because no
		 * other room sits between them. Their size is fixed by the sub-level, but their location is not,
		 * so the two rooms involved are allowed to move even though every other pass keeps them in place.
		 * This happens when applying the reserved sizes grows a room into the margin of the room above it.
		 * 固定された二部屋の衝突は、間に他の部屋が無いため他を動かしても解消できません。
		 * サイズはサブレベルによって決まっていますが位置は決まっていないので、
		 * 衝突している二部屋に限り、他の処理では固定している移動を許可します。
		 * 予約サイズの適用で部屋が上の階の余白へ伸びた場合に発生します。
		 */
		const auto collectMovableFixedRooms = [&parameter](const std::vector<FRoomCollisionEdge>& edges)
			{
				std::unordered_set<const dungeon::Room*> result;
				for (const auto& edge : edges)
				{
					if (
						IsFixedRoomForLayoutOptimization(*edge.Room0, parameter) &&
						IsFixedRoomForLayoutOptimization(*edge.Room1, parameter))
					{
						result.emplace(edge.Room0.get());
						result.emplace(edge.Room1.get());
					}
				}
				return result;
			};
		auto movableFixedRooms = collectMovableFixedRooms(collisionEdges);

		std::unordered_set<uint64> visitedLayouts;
		visitedLayouts.emplace(CalculateRoomLocationHash(rooms));
		const size_t maximumLocalMoveCount = std::max<size_t>(16, rooms.size() * 4);
		/*
		 * The fast path accepts only moves that strictly improve the global collision score.
		 * 高速経路では、全体の衝突スコアを必ず改善する移動だけを採用します。
		 */
		while (collisionScore.CollisionCount > 0 && diagnostics.LocalMoveCount < maximumLocalMoveCount)
		{
			auto hasBestCandidate = false;
			std::shared_ptr<dungeon::Room> bestMovingRoom;
			FRoomSeparationCandidate bestCandidate;
			FLayoutCollisionScore bestScore = collisionScore;

			const auto evaluateMove = [&](const std::shared_ptr<dungeon::Room>& anchorRoom, const std::shared_ptr<dungeon::Room>& movingRoom)
				{
					if (
						IsFixedRoomForLayoutOptimization(*movingRoom, parameter) &&
						movableFixedRooms.find(movingRoom.get()) == movableFixedRooms.end())
					{
						return;
					}

					FRoomSeparationCandidate currentRoomCandidate;
					currentRoomCandidate.Location = FIntVector(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
					ScoreSeparationCandidateCollisions(currentRoomCandidate, parameter, roomList, movingRoom);
					const uint64 unaffectedMaximumDepth = CalculateUnaffectedMaximumCollisionDepth(parameter, rooms, movingRoom);
					const auto margins = GetEffectiveRoomSeparationMargins(parameter, *anchorRoom, *movingRoom);
					const auto direction = MakeSeparationTieBreakDirection(*anchorRoom, *movingRoom, false);

					const auto evaluateAxis = [&](const ERoomSeparationAxis axis)
						{
							for (const bool positiveSide : { true, false })
							{
								auto candidate = MakeSeparationCandidate(
									*anchorRoom, *movingRoom, parameter, axis, positiveSide,
									margins.Horizontal, margins.Vertical, direction, false, 0.);
								ScoreSeparationCandidateCollisions(candidate, parameter, roomList, movingRoom);

								FLayoutCollisionScore nextScore;
								nextScore.CollisionCount = collisionScore.CollisionCount - currentRoomCandidate.CollisionCount + candidate.CollisionCount;
								nextScore.CollisionDepth = collisionScore.CollisionDepth - currentRoomCandidate.CollisionDepth + candidate.CollisionDepth;
								nextScore.MaxCollisionDepth = std::max(unaffectedMaximumDepth, candidate.MaxCollisionDepth);
								if (!IsBetterCollisionScore(collisionScore, nextScore))
									continue;

								candidate.CollisionCount = nextScore.CollisionCount;
								candidate.CollisionDepth = nextScore.CollisionDepth;
								candidate.MaxCollisionDepth = nextScore.MaxCollisionDepth;
								ScoreSeparationCandidateAisleDistance(candidate, parameter, aisles, movingRoom);
								if (
									!hasBestCandidate ||
									IsBetterCollisionScore(bestScore, nextScore) ||
									(IsSameCollisionScore(bestScore, nextScore) && IsBetterSeparationCandidate(bestCandidate, candidate)))
								{
									hasBestCandidate = true;
									bestMovingRoom = movingRoom;
									bestCandidate = candidate;
									bestScore = nextScore;
								}
							}
						};

					switch (parameter.GetExpansionPolicy())
					{
					case dungeon::ExpansionPolicy::Flat:
						evaluateAxis(ERoomSeparationAxis::X);
						evaluateAxis(ERoomSeparationAxis::Y);
						break;
					case dungeon::ExpansionPolicy::ExpandVertically:
						evaluateAxis(ERoomSeparationAxis::X);
						break;
					case dungeon::ExpansionPolicy::ExpandAnyDirection:
					default:
						evaluateAxis(ERoomSeparationAxis::X);
						evaluateAxis(ERoomSeparationAxis::Y);
						evaluateAxis(ERoomSeparationAxis::Z);
						break;
					}
				};

			for (const auto& edge : collisionEdges)
			{
				evaluateMove(edge.Room0, edge.Room1);
				evaluateMove(edge.Room1, edge.Room0);
			}
			if (!hasBestCandidate || bestMovingRoom == nullptr)
				break;

			bestMovingRoom->SetX(bestCandidate.Location.X);
			bestMovingRoom->SetY(bestCandidate.Location.Y);
			bestMovingRoom->SetZ(bestCandidate.Location.Z);
			diagnostics.Moved = true;
			++diagnostics.LocalMoveCount;
			if (!visitedLayouts.emplace(CalculateRoomLocationHash(rooms)).second)
				break;
			collisionScore = CollectRoomCollisions(parameter, rooms, &collisionEdges);
			movableFixedRooms = collectMovableFixedRooms(collisionEdges);
		}

		/*
		 * A repeated-layout exit occurs immediately after moving a room, so the cached score and
		 * edges can describe the preceding layout. Rebuild them before selecting fallback groups.
		 * 同一配置の検出は部屋移動直後に発生するため、キャッシュ済みスコアと辺が移動前を
		 * 表す場合があります。フォールバック対象の選択前に現在配置から再構築します。
		 */
		collisionScore = CollectRoomCollisions(parameter, rooms, &collisionEdges);
		if (collisionScore.CollisionCount == 0)
			return ECollisionResolutionStatus::Completed;

		const size_t maximumFallbackRoundCount = std::max<size_t>(1, rooms.size());
		/*
		 * Rebuild components after each round because moving one component can change the remaining graph.
		 * 一つの成分の移動で残りの衝突グラフが変化するため、各ラウンド後に成分を再構築します。
		 * The room-count limit guarantees termination even when constraints cannot be satisfied.
		 * 部屋数を上限にすることで、制約を満たせない場合でも必ず終了します。
		 */
		for (size_t fallbackRound = 0; fallbackRound < maximumFallbackRoundCount; ++fallbackRound)
		{
			++diagnostics.FallbackRoundCount;
			for (const auto& edge : collisionEdges)
			{
				if (
					IsFixedRoomForLayoutOptimization(*edge.Room0, parameter) &&
					IsFixedRoomForLayoutOptimization(*edge.Room1, parameter))
				{
					diagnostics.RemainingCollisionEdges = collisionEdges;
					return ECollisionResolutionStatus::ImmutableConflict;
				}
			}

			for (const auto& component : BuildCollisionComponents(rooms, collisionEdges))
			{
				diagnostics.MaximumCollisionGroupSize = std::max(diagnostics.MaximumCollisionGroupSize, component.size());
				if (!RepackCollisionComponent(parameter, aisles, rooms, component, diagnostics))
				{
					diagnostics.RemainingCollisionEdges = collisionEdges;
					return ECollisionResolutionStatus::FallbackFailed;
				}
			}

			collisionScore = CollectRoomCollisions(parameter, rooms, &collisionEdges);
			if (collisionScore.CollisionCount == 0)
				return ECollisionResolutionStatus::Completed;

			diagnostics.FailureReason = ECollisionFallbackFailureReason::PostValidationCollision;
		}

		diagnostics.FailureReason = ECollisionFallbackFailureReason::RetryLimit;
		diagnostics.RemainingCollisionEdges = collisionEdges;
		return ECollisionResolutionStatus::FallbackFailed;
	}

	/*
	 * Scores how much a collision-resolution candidate preserves connected room distance.
	 * 衝突解消候補が接続部屋との距離をどれだけ維持できるかを採点します。
	 */
	void ScoreSeparationCandidateAisleDistance(
		FRoomSeparationCandidate& candidate,
		const dungeon::GenerateParameter& parameter,
		const std::vector<dungeon::Aisle>& aisles,
		const std::shared_ptr<dungeon::Room>& movingRoom) noexcept
	{
		candidate.MainPathSpacingDeviationCost = CalculateRoomMainPathSpacingDeviation(
			aisles,
			parameter,
			movingRoom,
			candidate.Location,
			candidate.MaxMainPathSpacingDeviationCost
		);
		candidate.ConnectedDistanceCost = CalculateRoomConnectedDistanceCost(aisles, movingRoom, candidate.Location);
		candidate.MaxConnectedDistanceCost = CalculateMaxConnectedDistanceCost(aisles, movingRoom, candidate.Location);
		candidate.LayoutSpreadCost =
			std::abs(static_cast<double>(candidate.Location.X)) +
			std::abs(static_cast<double>(candidate.Location.Y)) +
			std::abs(static_cast<double>(candidate.Location.Z));
		candidate.FixedRoomMoveCost = IsFixedRoomForLayoutOptimization(*movingRoom, parameter) ?
			std::numeric_limits<double>::max() :
			GetLocationDistanceSquared(*movingRoom, candidate.Location) * GetRoomMovePenalty(*movingRoom);
	}

	/*
	 * 詰める対象ではない軸で部屋同士が重なっている場合にtrueを返します。
	 */
	bool CanCompactAlongAxis(const dungeon::Room& room0, const dungeon::Room& room1, const ERoomSeparationAxis axis) noexcept
	{
		const auto overlapX = RoomIntervalsOverlap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight());
		const auto overlapY = RoomIntervalsOverlap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom());
		const auto overlapZ = RoomIntervalsOverlap(room0.GetBackground(), room0.GetForeground(), room1.GetBackground(), room1.GetForeground());

		switch (axis)
		{
		case ERoomSeparationAxis::X:
			return overlapY && overlapZ;
		case ERoomSeparationAxis::Y:
			return overlapX && overlapZ;
		case ERoomSeparationAxis::Z:
			return overlapX && overlapY;
		default:
			checkNoEntry();
			return false;
		}
	}

	/*
	 * 1つの軸上にある2つの部屋の現在の隙間を返します。
	 */
	int32_t GetRoomAxisGap(const dungeon::Room& room0, const dungeon::Room& room1, const ERoomSeparationAxis axis) noexcept
	{
		switch (axis)
		{
		case ERoomSeparationAxis::X:
			return room0.GetRight() <= room1.GetLeft() ? room1.GetLeft() - room0.GetRight() : room0.GetLeft() - room1.GetRight();
		case ERoomSeparationAxis::Y:
			return room0.GetBottom() <= room1.GetTop() ? room1.GetTop() - room0.GetBottom() : room0.GetTop() - room1.GetBottom();
		case ERoomSeparationAxis::Z:
			return room0.GetForeground() <= room1.GetBackground() ? room1.GetBackground() - room0.GetForeground() : room0.GetBackground() - room1.GetForeground();
		default:
			checkNoEntry();
			return 0;
		}
	}

	/*
	 * 移動する部屋を基準部屋から指定余白の位置へ置く目標座標を返します。
	 */
	bool MakeRoomCompactionLocation(const dungeon::Room& anchorRoom, const dungeon::Room& movingRoom, const dungeon::GenerateParameter& parameter, const ERoomSeparationAxis axis, const int32_t margin, FIntVector& outLocation) noexcept
	{
		outLocation = FIntVector(movingRoom.GetX(), movingRoom.GetY(), movingRoom.GetZ());
		ApplySeparationConstraints(outLocation, movingRoom, parameter);

		switch (axis)
		{
		case ERoomSeparationAxis::X:
			if (movingRoom.GetLeft() >= anchorRoom.GetRight())
				outLocation.X = anchorRoom.GetRight() + margin;
			else if (movingRoom.GetRight() <= anchorRoom.GetLeft())
				outLocation.X = anchorRoom.GetLeft() - margin - movingRoom.GetWidth();
			else
				return false;
			ApplySeparationConstraints(outLocation, movingRoom, parameter);
			return outLocation.X != movingRoom.GetX();

		case ERoomSeparationAxis::Y:
			if (movingRoom.GetTop() >= anchorRoom.GetBottom())
				outLocation.Y = anchorRoom.GetBottom() + margin;
			else if (movingRoom.GetBottom() <= anchorRoom.GetTop())
				outLocation.Y = anchorRoom.GetTop() - margin - movingRoom.GetDepth();
			else
				return false;
			ApplySeparationConstraints(outLocation, movingRoom, parameter);
			return outLocation.Y != movingRoom.GetY();

		case ERoomSeparationAxis::Z:
			if (movingRoom.GetBackground() >= anchorRoom.GetForeground())
				outLocation.Z = anchorRoom.GetForeground() + margin;
			else if (movingRoom.GetForeground() <= anchorRoom.GetBackground())
				outLocation.Z = anchorRoom.GetBackground() - margin - movingRoom.GetHeight();
			else
				return false;
			ApplySeparationConstraints(outLocation, movingRoom, parameter);
			return outLocation.Z != movingRoom.GetZ();

		default:
			checkNoEntry();
			return false;
		}
	}

	/*
	 * 1つ目の部屋を2つ目の部屋より先に移動候補として試す場合にtrueを返します。
	 */
	bool PreferFirstRoomForCompaction(const dungeon::Room& room0, const dungeon::Room& room1, const dungeon::GenerateParameter& parameter) noexcept
	{
		const auto fixed0 = IsFixedRoomForLayoutOptimization(room0, parameter);
		const auto fixed1 = IsFixedRoomForLayoutOptimization(room1, parameter);
		if (fixed0 != fixed1)
			return fixed1;

		const auto center0 = room0.GetCenter();
		const auto center1 = room1.GetCenter();
		const auto distance0 = center0.SizeSquared();
		const auto distance1 = center1.SizeSquared();
		if (distance0 != distance1)
			return distance0 > distance1;

		return static_cast<uint16_t>(room0.GetIdentifier()) > static_cast<uint16_t>(room1.GetIdentifier());
	}
}

namespace dungeon
{
	void Generator::Reset()
	{
		mVoxel.reset();
		mLayoutGraph = LayoutGraph();
		mRooms.clear();
		mFloorHeight.clear();
		mStartRoom.reset();
		mGoalRoom.reset();

		mStartPoint.reset();
		mGoalPoint.reset();

		mAisles.clear();
		mDeepestDepthFromStart = 0;
		mLastError = Generator::Error::Success;
		mLastLayoutMetrics = FDungeonLayoutMetrics();
		mLastLayoutScore = FDungeonLayoutScore();
	}

	bool Generator::Generate(const GenerateParameter& parameter) noexcept
	{
		Identifier::ResetCounter();
		mLastError = Error::Success;
		mWarningFlags = 0;
		mAbandonedAisleIdentifiers.clear();
		mLastErrorRoomParts = Room::Parts::Unidentified;
		mGenerateParameter = parameter;

		// 生成
		if (!GenerateImpl())
		{
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
			// 部屋の情報をダンプ
			for (const auto& room : mRooms)
			{
				DUNGEON_GENERATOR_LOG(TEXT("Room: %d,Position(%d, %d, %d) Size(%d, %d, %d) Parts=%d, DepthFromStart=%d"),
					room->GetIdentifier().Get(),
					room->GetX(), room->GetY(), room->GetZ(),
					room->GetWidth(), room->GetDepth(), room->GetHeight(),
					room->GetParts(),
					room->GetDepthFromStart()
				);
			}

			// 通路の情報をダンプ
			for (const auto& aisle : mAisles)
			{
				DUNGEON_GENERATOR_LOG(TEXT("Aisle: %s - %s")
					, UTF8_TO_TCHAR(aisle.GetPoint(0)->GetOwnerRoom()->GetName().data())
					, UTF8_TO_TCHAR(aisle.GetPoint(1)->GetOwnerRoom()->GetName().data())
				);
			}
#endif
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			// GenerateImplが失敗したならば、必ず失敗の理由が記録されているはずです
			check(mLastError != Error::Success);
#endif
		}

		// エラー情報を記録
		if (mLastError != Generator::Error::Success)
		{
			if (mVoxel && mVoxel->GetLastError() != Voxel::Error::Success)
			{
				uint8_t errorIndex = static_cast<uint8_t>(Error::___StartVoxelError);
				errorIndex += static_cast<uint8_t>(mVoxel->GetLastError());
				mLastError = static_cast<Error>(errorIndex);
			}
		}
		else
		{
			UpdateMeshAttributes();

#if defined(DEBUG_GENERATE_ARTIFACT_FILE)
			/*
			 * 完成したダンジョンだけを成果物として残します。
			 * 途中で失敗して別の乱数の種で作り直したダンジョンは経歴に含めません。
			 * 画像と構造図が同じ名前になるよう、パスは一度だけ作って拡張子だけを変えます。
			 */
			if (mVoxel)
			{
				const std::string artifactPath = dungeon::CreateArtifactBasePath(mGenerateParameter.GetGeneratedRandomSeed());
				mVoxel->GenerateImageForArtifact(artifactPath + ".bmp", mFloorHeight);
				DumpRoomDiagram(artifactPath + ".md");
			}
#endif
		}

		return mLastError == Error::Success;
	}


	bool Generator::GenerateImpl() noexcept
	{
		// 意図グラフから距離を考慮した部屋配置を選びます
		auto layoutCandidates = BuildIntentLayoutCandidates();
		if (SelectDistanceAwareLayout(1, layoutCandidates) == false)
			return false;

		// 接続距離を悪化させにくい候補を選びながら部屋の重なりを解消します
		if (ResolveLayoutCollisions(2, 0) == ResolveLayoutCollisionsResult::Failed)
			return false;

		// 常時ロードするサブレベルを通常部屋へ割り当てます
		if (AdjustReservedSubLevels(3) == false)
			return false;

		// Adjust ordinary room sizes before selecting the final endpoints.
		AdjustRoomSize(4);

		// Resolve overlaps introduced by reserved ordinary sublevels and size adjustments.
		ResolveLayoutCollisionsResult resolveLayoutCollisionsResult;
		uint8_t subPhase = 1;
		do {
			resolveLayoutCollisionsResult = ResolveLayoutCollisions(5, subPhase);
			if (resolveLayoutCollisionsResult == ResolveLayoutCollisionsResult::Failed)
				return false;
			++subPhase;
		} while (resolveLayoutCollisionsResult != ResolveLayoutCollisionsResult::Completed);

		// 通路距離を局所的に最小化します
		if (!OptimizeAisleDistance(6))
			return false;
		if (!FinalizeEndpointLayout(7))
			return false;
		RoomPlacer::AssignZones(mGenerateParameter, mLayoutGraph, mRooms);

		// 部屋が全て収まるように空間を拡張します
		if (ExpandSpace(8) == false)
			return false;

		// Pointの同期
		AdjustPoints();

		// ブランチIDと各部屋の深さの生成
		if (MarkBranchIdAndDepthFromStart() == false)
			return false;

		// 階層情報と全体の深さの生成
		if (DetectFloorHeightAndDepthFromStart() == false)
			return false;

		// 部屋と通路に意味付けする
		if (mGenerateParameter.UseMissionGraph())
		{
			float maxKeyCount = std::sqrt(static_cast<float>(mRooms.size()));
			maxKeyCount = std::ceil(maxKeyCount);
			if (maxKeyCount < 2)
				maxKeyCount = 2;

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			MEASURE_TIME_START(stopwatch);
#endif
			MissionGraph missionGraph(shared_from_this(), mStartRoom, mGoalRoom, maxKeyCount);
			RefreshLockedRouteRoomFlags();
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			MEASURE_TIME_LAP(stopwatch, TEXT("MissionGraph"));
#endif

#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
			// デバッグ情報を出力
			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/DungeonStructureDiagram.md");
#endif

			/*
			 * ユニーク鍵を置ける部屋が無いレイアウトでは、MissionGraphはロックを一つも配置しません。
			 * 鍵もロックも無いダンジョンは検証する対象そのものが無く、MissionGraphTesterは
			 * 鍵とロックが1組ある事を前提としているため必ず不合格になります。
			 * ダンジョンとしては完成していて到達可能なので、警告だけを残して生成は成功させます。
			 */
			if (missionGraph.Placed() == false)
			{
				DUNGEON_GENERATOR_WARNING(TEXT("MissionGraph: the dungeon was generated without keys and locks."));
				mWarningFlags |= static_cast<uint8_t>(Warning::KeysAndLocksNotPlaced);
			}
			else
			{
				// クリアできるミッションかテストします
				const MissionGraphTester missionGraphTester(mRooms, mAisles);
				if (missionGraphTester.Success() == false)
				{
					DUNGEON_GENERATOR_ERROR(TEXT("MissionGraph validation failed. The generated key-lock route is not solvable."));
					mLastError = Error::MissionGraphValidationFailed;
					return false;
				}
			}
		}
		else
		{
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: Skipped"));

#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
			// デバッグ情報を出力
			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/DungeonStructureDiagram.md");
#endif
		}

		// スタート部屋とゴール部屋のコールバックを呼ぶ
		InvokeRoomCallbacks();

		// ボクセル情報を生成します
		if (GenerateVoxel(9) == false)
			return false;

		/*
		 * 生成を諦めた通路があると、その通路でしかつながっていない部屋へ到達できなくなります。
		 * ミニマップには表示されるため、利用者からは入れない部屋として見えます。
		 * 生成を失敗させて別の乱数の種で作り直させます。
		 */
		if (VerifyRoomReachability() == false)
			return false;

		return true;
	}

	/**
	 * Reapplies endpoint policies to the final room positions and synchronizes endpoint references.
	 * 最終的な部屋位置へ開始・ゴールポリシーを再適用し、開始・ゴール参照を同期します。
	 */
	bool Generator::RefreshEndpointPoliciesFromCurrentLayout() noexcept
	{
		// 通路の一覧を構築した後なので、グラフの辺を削除させません
		if (!LayoutGraphGenerator::ApplyEndpointPolicies(mGenerateParameter, mLayoutGraph, mRooms, true))
		{
			mLastError = Error::SeparateRoomsFailed;
			return false;
		}

		const std::vector<std::shared_ptr<Room>> indexedRooms(mRooms.begin(), mRooms.end());
		if (mLayoutGraph.StartNodeIndex >= indexedRooms.size() || mLayoutGraph.GoalNodeIndex >= indexedRooms.size())
		{
			mLastError = Error::SeparateRoomsFailed;
			return false;
		}

		mStartRoom = indexedRooms[mLayoutGraph.StartNodeIndex];
		mGoalRoom = indexedRooms[mLayoutGraph.GoalNodeIndex];
		if (mStartRoom == nullptr || mGoalRoom == nullptr)
		{
			mLastError = Error::SeparateRoomsFailed;
			return false;
		}

		mStartPoint = std::make_shared<Point>(mStartRoom);
		mGoalPoint = std::make_shared<Point>(mGoalRoom);

		for (Aisle& aisle : mAisles)
		{
			if (aisle.GetPoint(0) == nullptr || aisle.GetPoint(1) == nullptr)
			{
				mLastError = Error::SeparateRoomsFailed;
				return false;
			}
			const std::shared_ptr<Room> room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const std::shared_ptr<Room> room1 = aisle.GetPoint(1)->GetOwnerRoom();
			const auto edgeIterator = std::find_if(mLayoutGraph.Edges.cbegin(), mLayoutGraph.Edges.cend(), [&indexedRooms, &room0, &room1](const LayoutAisleEdge& edge)
				{
					return edge.Room0 < indexedRooms.size() && edge.Room1 < indexedRooms.size() &&
						((indexedRooms[edge.Room0] == room0 && indexedRooms[edge.Room1] == room1) ||
							(indexedRooms[edge.Room0] == room1 && indexedRooms[edge.Room1] == room0));
				});
			if (edgeIterator == mLayoutGraph.Edges.cend())
			{
				mLastError = Error::SeparateRoomsFailed;
				return false;
			}
			aisle.SetMain(edgeIterator->bMainPath);
			aisle.SetPurpose(edgeIterator->Purpose);
		}
		return true;
	}

	/**
	 * Finalizes endpoint identities before applying registered Start and Goal room sizes.
	 * 登録済みの開始部屋とゴール部屋のサイズを適用する前に、端点となる部屋を確定します。
	 */
	bool Generator::FinalizeEndpointLayout(const size_t phase) noexcept
	{
		if (!RefreshEndpointPoliciesFromCurrentLayout())
			return false;
		if (!mGenerateParameter.IsGenerateStartRoomReserved() && !mGenerateParameter.IsGenerateGoalRoomReserved())
			return true;

		ApplyEndpointRoomSizes();
		ResolveLayoutCollisionsResult collisionResult;
		size_t subPhase = 0;
		do
		{
			collisionResult = ResolveLayoutCollisions(phase, subPhase++);
			if (collisionResult == ResolveLayoutCollisionsResult::Failed)
				return false;
		} while (collisionResult != ResolveLayoutCollisionsResult::Completed);

		return OptimizeAisleDistance(phase);
	}

	/**
	 * Represents BuildIntentLayoutCandidates.
	 * 部屋の初期位置を決定します
	 */
	std::vector<LayoutCandidate> Generator::BuildIntentLayoutCandidates() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("BuildIntentLayoutCandidates"));
			}
		);
#endif

		const int32 candidateCount = std::max(3, mGenerateParameter.GetLayoutCandidateCount());
		std::vector<LayoutCandidate> candidates;
		candidates.reserve(candidateCount);
		for (int32 candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
		{
			LayoutCandidate candidate;
			candidate.Graph = LayoutGraphGenerator::Generate(mGenerateParameter);
			candidate.Rooms = RoomPlacer::Place(mGenerateParameter, candidate.Graph);

			// 部屋の座標が決まったので、枝の通路をより近い部屋へつなぎ替えます
			LayoutGraphGenerator::OptimizeBranchParents(candidate.Graph, candidate.Rooms);

			if (!LayoutGraphGenerator::ApplyEndpointPolicies(mGenerateParameter, candidate.Graph, candidate.Rooms))
			{
				continue;
			}
			// 通路の一覧を構築する前に、門を置ける数を超える接続を間引きます
			LayoutGraphGenerator::LimitEndpointGateCapacity(mGenerateParameter, candidate.Graph);

			if (AislePlanner::Plan(candidate.Graph, candidate.Rooms, candidate.Aisles, candidate.StartPoint, candidate.GoalPoint) == false)
			{
				continue;
			}

			LayoutEvaluator::Evaluate(mGenerateParameter, candidateIndex, candidate);
			candidates.emplace_back(std::move(candidate));
		}

		return candidates;
	}

	bool Generator::SelectDistanceAwareLayout(const size_t phase, std::vector<LayoutCandidate>& candidates) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("SelectDistanceAwareLayout"));
			}
		);
#endif

		mRooms.clear();
		mAisles.clear();
		mStartPoint.reset();
		mGoalPoint.reset();
		mStartRoom.reset();
		mGoalRoom.reset();

		if (candidates.empty())
		{
			mLastError = Error::SeparateRoomsFailed;
			return false;
		}

		auto bestCandidateIterator = candidates.begin();
		for (auto candidateIterator = std::next(candidates.begin()); candidateIterator != candidates.end(); ++candidateIterator)
		{
			if (candidateIterator->Score.TotalScore > bestCandidateIterator->Score.TotalScore)
			{
				bestCandidateIterator = candidateIterator;
			}
		}

		mRooms = std::move(bestCandidateIterator->Rooms);
		mLayoutGraph = std::move(bestCandidateIterator->Graph);
		mAisles = std::move(bestCandidateIterator->Aisles);
		mStartPoint = bestCandidateIterator->StartPoint;
		mGoalPoint = bestCandidateIterator->GoalPoint;
		mStartRoom = mStartPoint ? mStartPoint->GetOwnerRoom() : nullptr;
		mGoalRoom = mGoalPoint ? mGoalPoint->GetOwnerRoom() : nullptr;
		mLastLayoutMetrics = bestCandidateIterator->Metrics;
		mLastLayoutScore = bestCandidateIterator->Score;

		if (mStartRoom == nullptr || mGoalRoom == nullptr)
		{
			mLastError = Error::SeparateRoomsFailed;
			return false;
		}

		DUNGEON_GENERATOR_LOG(TEXT("Layout selected: Candidate=%d Score=%f Rooms=%d Aisles=%d CriticalPath=%d Branches=%d Loops=%d Vertical=%d SpecialDeadEndCoverage=%f TotalAisleDistance=%f AverageAisleDistance=%f MaxAisleDistance=%f MainPathAisleDistance=%f"),
			mLastLayoutScore.CandidateIndex,
			mLastLayoutScore.TotalScore,
			mLastLayoutMetrics.RoomCount,
			mLastLayoutMetrics.AisleCount,
			mLastLayoutMetrics.CriticalPathLength,
			mLastLayoutMetrics.BranchCount,
			mLastLayoutMetrics.LoopCount,
			mLastLayoutMetrics.VerticalTransitionCount,
			mLastLayoutMetrics.SpecialDeadEndCoverage,
			mLastLayoutMetrics.TotalAisleDistance,
			mLastLayoutMetrics.AverageAisleDistance,
			mLastLayoutMetrics.MaxAisleDistance,
			mLastLayoutMetrics.MainPathAisleDistance
		);

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_SelectDistanceAwareLayout.bmp");
#endif

		return true;
	}

	/**
	 * Represents ResolveLayoutCollisions.
	 * 部屋の重なりを解消します
	 */
	Generator::ResolveLayoutCollisionsResult Generator::ResolveLayoutCollisions(const size_t phase, const size_t subPhase) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("ResolveLayoutCollisions"));
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Separate Rooms"));
#endif

		// 部屋の交差を解消します
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			FIntVector location(room->GetX(), room->GetY(), room->GetZ());
			ApplySeparationConstraints(location, *room, mGenerateParameter);
			room->SetX(location.X);
			room->SetY(location.Y);
			room->SetZ(location.Z);
		}

		FCollisionResolutionDiagnostics diagnostics;
		const auto status = ResolveRoomCollisionsRobust(mGenerateParameter, mRooms, mAisles, diagnostics);
		if (status != ECollisionResolutionStatus::Completed)
		{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
			GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_SeparateRooms_failure.bmp");
#endif
			if (status == ECollisionResolutionStatus::ImmutableConflict)
			{
				DUNGEON_GENERATOR_ERROR(
					TEXT("Generator::ResolveLayoutCollisions: Immutable rooms overlap including configured margins. Phase=%llu SubPhase=%llu RemainingCollisions=%llu"),
					static_cast<uint64>(phase),
					static_cast<uint64>(subPhase),
					static_cast<uint64>(diagnostics.RemainingCollisionEdges.size()));
			}
			else
			{
				const TCHAR* failureReason = TEXT("Unknown");
				switch (diagnostics.FailureReason)
				{
				case ECollisionFallbackFailureReason::NoCandidate:
					failureReason = TEXT("NoCandidate");
					break;
				case ECollisionFallbackFailureReason::PostValidationCollision:
					failureReason = TEXT("PostValidationCollision");
					break;
				case ECollisionFallbackFailureReason::RetryLimit:
					failureReason = TEXT("RetryLimit");
					break;
				case ECollisionFallbackFailureReason::None:
				default:
					break;
				}

				DUNGEON_GENERATOR_ERROR(
					TEXT("Generator::ResolveLayoutCollisions: Collision-free fallback validation failed. Phase=%llu SubPhase=%llu Reason=%s FallbackRounds=%llu RemainingCollisions=%llu"),
					static_cast<uint64>(phase),
					static_cast<uint64>(subPhase),
					failureReason,
					static_cast<uint64>(diagnostics.FallbackRoundCount),
					static_cast<uint64>(diagnostics.RemainingCollisionEdges.size()));

				if (diagnostics.FailedRoom != nullptr)
				{
					const auto& room = diagnostics.FailedRoom;
					DUNGEON_GENERATOR_ERROR(
						TEXT("Generator::ResolveLayoutCollisions: No collision-free candidate for Room=%u Location=(%d,%d,%d) Size=(%d,%d,%d)"),
						static_cast<uint16>(room->GetIdentifier()),
						room->GetX(), room->GetY(), room->GetZ(),
						room->GetWidth(), room->GetDepth(), room->GetHeight());
				}
			}

			/*
			 * Limit pair details to keep a pathological layout from flooding the Unreal log.
			 * 異常なレイアウトでUnrealログが埋まらないよう、衝突ペアの詳細出力数を制限します。
			 */
			constexpr size_t MaximumLoggedCollisionCount = 8;
			const size_t loggedCollisionCount = std::min(MaximumLoggedCollisionCount, diagnostics.RemainingCollisionEdges.size());
			for (size_t collisionIndex = 0; collisionIndex < loggedCollisionCount; ++collisionIndex)
			{
				const auto& edge = diagnostics.RemainingCollisionEdges[collisionIndex];
				const auto margins = GetEffectiveRoomSeparationMargins(mGenerateParameter, *edge.Room0, *edge.Room1);
				DUNGEON_GENERATOR_ERROR(
					TEXT("Generator::ResolveLayoutCollisions: Collision[%llu] Room0=%u Location=(%d,%d,%d) Size=(%d,%d,%d) Room1=%u Location=(%d,%d,%d) Size=(%d,%d,%d) Margin=(%u,%u)"),
					static_cast<uint64>(collisionIndex),
					static_cast<uint16>(edge.Room0->GetIdentifier()),
					edge.Room0->GetX(), edge.Room0->GetY(), edge.Room0->GetZ(),
					edge.Room0->GetWidth(), edge.Room0->GetDepth(), edge.Room0->GetHeight(),
					static_cast<uint16>(edge.Room1->GetIdentifier()),
					edge.Room1->GetX(), edge.Room1->GetY(), edge.Room1->GetZ(),
					edge.Room1->GetWidth(), edge.Room1->GetDepth(), edge.Room1->GetHeight(),
					margins.Horizontal, margins.Vertical);
			}
			mLastError = Error::SeparateRoomsFailed;
			return ResolveLayoutCollisionsResult::Failed;
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		if (diagnostics.Moved)
		{
			GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_ResolveLayoutCollisions.bmp");
		}
#endif
#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(
			TEXT("ResolveLayoutCollisions: local moves=%llu, repacked rooms=%llu, maximum collision group=%llu"),
			static_cast<uint64>(diagnostics.LocalMoveCount),
			static_cast<uint64>(diagnostics.RepackedRoomCount),
			static_cast<uint64>(diagnostics.MaximumCollisionGroupSize));
#endif
		return diagnostics.Moved ? ResolveLayoutCollisionsResult::Moved : ResolveLayoutCollisionsResult::Completed;

	}

	/*
	 * Locally minimizes aisle distance after collision resolution without breaking room margins.
	 * 衝突解消後に、部屋の余白を破らない範囲で通路距離を局所的に最小化します。
	 */
	bool Generator::OptimizeAisleDistance(size_t phase) const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("OptimizeAisleDistance"));
			}
		);
#endif

		if (mRooms.empty() || mAisles.empty())
			return true;

		std::vector<const Aisle*> aisles;
		aisles.reserve(mAisles.size());
		for (const auto& aisle : mAisles)
		{
			aisles.emplace_back(&aisle);
		}
		std::stable_sort(aisles.begin(), aisles.end(), [](const Aisle* left, const Aisle* right)
			{
				return static_cast<uint16_t>(left->GetIdentifier()) < static_cast<uint16_t>(right->GetIdentifier());
			}
		);

		const auto expansionPolicy = mGenerateParameter.GetExpansionPolicy();
		std::vector<ERoomSeparationAxis> axes;
		switch (expansionPolicy)
		{
		case ExpansionPolicy::Flat:
			axes = { ERoomSeparationAxis::X, ERoomSeparationAxis::Y };
			break;
		case ExpansionPolicy::ExpandVertically:
			axes = { ERoomSeparationAxis::X };
			break;
		case ExpansionPolicy::ExpandAnyDirection:
		default:
			axes = { ERoomSeparationAxis::X, ERoomSeparationAxis::Y, ERoomSeparationAxis::Z };
			break;
		}

		const auto validateCandidate = [this](const std::shared_ptr<Room>& movingRoom, const FIntVector& location) -> bool
			{
				Room candidateRoom(*movingRoom);
				candidateRoom.SetX(location.X);
				candidateRoom.SetY(location.Y);
				candidateRoom.SetZ(location.Z);

				for (const auto& otherRoom : mRooms)
				{
					if (otherRoom == movingRoom)
						continue;

					const auto margins = GetEffectiveRoomSeparationMargins(mGenerateParameter, candidateRoom, *otherRoom);
					if (candidateRoom.Intersect(*otherRoom, margins.Horizontal, margins.Vertical))
						return false;
				}
				return true;
			};

		const auto isBetterOptimizationResult = [](const double currentSpacingDeviation, const double currentMaxSpacingDeviation, const double currentDistanceCost, const double nextSpacingDeviation, const double nextMaxSpacingDeviation, const double nextDistanceCost) noexcept
			{
				constexpr double epsilon = 1.e-6;
				if (nextSpacingDeviation < currentSpacingDeviation - epsilon)
					return true;
				if (nextSpacingDeviation > currentSpacingDeviation + epsilon)
					return false;
				if (nextMaxSpacingDeviation < currentMaxSpacingDeviation - epsilon)
					return true;
				if (nextMaxSpacingDeviation > currentMaxSpacingDeviation + epsilon)
					return false;
				return nextDistanceCost < currentDistanceCost - epsilon;
			};

		const auto tryMoveRoom = [this, &validateCandidate, &isBetterOptimizationResult](const std::shared_ptr<Room>& anchorRoom, const std::shared_ptr<Room>& movingRoom, const ERoomSeparationAxis axis, const int32_t margin) -> bool
			{
				if (IsFixedRoomForLayoutOptimization(*movingRoom, mGenerateParameter))
					return false;

				FIntVector location;
				if (MakeRoomCompactionLocation(*anchorRoom, *movingRoom, mGenerateParameter, axis, margin, location) == false)
					return false;

				if (validateCandidate(movingRoom, location) == false)
					return false;

				const double currentCost = CalculateLayoutAisleDistanceCost(mAisles);
				double currentMaxSpacingDeviation = 0.;
				const double currentSpacingDeviation = CalculateLayoutMainPathSpacingDeviation(mAisles, mGenerateParameter, currentMaxSpacingDeviation);
				const FIntVector currentLocation(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
				const double movePenalty = std::sqrt(GetLocationDistanceSquared(*movingRoom, location)) * GetRoomMovePenalty(*movingRoom) * 0.05;

				movingRoom->SetX(location.X);
				movingRoom->SetY(location.Y);
				movingRoom->SetZ(location.Z);
				const double nextCost = CalculateLayoutAisleDistanceCost(mAisles) + movePenalty;
				double nextMaxSpacingDeviation = 0.;
				const double nextSpacingDeviation = CalculateLayoutMainPathSpacingDeviation(mAisles, mGenerateParameter, nextMaxSpacingDeviation);
				if (isBetterOptimizationResult(
					currentSpacingDeviation,
					currentMaxSpacingDeviation,
					currentCost,
					nextSpacingDeviation,
					nextMaxSpacingDeviation,
					nextCost))
				{
					return true;
				}

				movingRoom->SetX(currentLocation.X);
				movingRoom->SetY(currentLocation.Y);
				movingRoom->SetZ(currentLocation.Z);
				return false;
			};

		const auto tryStepRoomTowardAnchor = [this, &validateCandidate, &isBetterOptimizationResult](const std::shared_ptr<Room>& anchorRoom, const std::shared_ptr<Room>& movingRoom, const ERoomSeparationAxis axis) -> bool
			{
				if (IsFixedRoomForLayoutOptimization(*movingRoom, mGenerateParameter))
					return false;

				FIntVector location(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
				switch (axis)
				{
				case ERoomSeparationAxis::X:
					if (movingRoom->GetCenter().X < anchorRoom->GetCenter().X)
						++location.X;
					else if (movingRoom->GetCenter().X > anchorRoom->GetCenter().X)
						--location.X;
					else
						return false;
					break;
				case ERoomSeparationAxis::Y:
					if (movingRoom->GetCenter().Y < anchorRoom->GetCenter().Y)
						++location.Y;
					else if (movingRoom->GetCenter().Y > anchorRoom->GetCenter().Y)
						--location.Y;
					else
						return false;
					break;
				case ERoomSeparationAxis::Z:
					{
						const auto verticalSpacing = RoomPlacer::CalculateVerticalSpacing(mGenerateParameter);
						if (movingRoom->GetCenter().Z < anchorRoom->GetCenter().Z)
							location.Z += verticalSpacing;
						else if (movingRoom->GetCenter().Z > anchorRoom->GetCenter().Z)
							location.Z -= verticalSpacing;
						else
							return false;
					}
					break;
				default:
					checkNoEntry();
					return false;
				}
				ApplySeparationConstraints(location, *movingRoom, mGenerateParameter);

				if (validateCandidate(movingRoom, location) == false)
					return false;

				const double currentCost = CalculateLayoutAisleDistanceCost(mAisles);
				double currentMaxSpacingDeviation = 0.;
				const double currentSpacingDeviation = CalculateLayoutMainPathSpacingDeviation(mAisles, mGenerateParameter, currentMaxSpacingDeviation);
				const FIntVector currentLocation(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
				movingRoom->SetX(location.X);
				movingRoom->SetY(location.Y);
				movingRoom->SetZ(location.Z);
				const double nextCost = CalculateLayoutAisleDistanceCost(mAisles);
				double nextMaxSpacingDeviation = 0.;
				const double nextSpacingDeviation = CalculateLayoutMainPathSpacingDeviation(mAisles, mGenerateParameter, nextMaxSpacingDeviation);
				if (isBetterOptimizationResult(
					currentSpacingDeviation,
					currentMaxSpacingDeviation,
					currentCost,
					nextSpacingDeviation,
					nextMaxSpacingDeviation,
					nextCost))
				{
					return true;
				}

				movingRoom->SetX(currentLocation.X);
				movingRoom->SetY(currentLocation.Y);
				movingRoom->SetZ(currentLocation.Z);
				return false;
			};

		const size_t maxIterationCount = std::min<size_t>(12, mRooms.size() * 2);
		for (size_t iteration = 0; iteration < maxIterationCount; ++iteration)
		{
			auto moved = false;
			for (const auto* aisle : aisles)
			{
				if (aisle == nullptr || aisle->GetPoint(0) == nullptr || aisle->GetPoint(1) == nullptr)
					continue;

				const auto& room0 = aisle->GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle->GetPoint(1)->GetOwnerRoom();
				if (room0 == nullptr || room1 == nullptr || room0 == room1)
					continue;

				const auto margins = GetEffectiveRoomSeparationMargins(mGenerateParameter, *room0, *room1);
				for (const auto axis : axes)
				{
					const int32_t margin = axis == ERoomSeparationAxis::Z ? margins.Vertical : margins.Horizontal;
					const auto bPreferRoom0 = PreferFirstRoomForCompaction(*room0, *room1, mGenerateParameter);
					auto movedOnAxis = false;
					if (CanCompactAlongAxis(*room0, *room1, axis))
					{
						const auto gap = GetRoomAxisGap(*room0, *room1, axis);
						if (gap > margin)
						{
							movedOnAxis = bPreferRoom0 ?
								(tryMoveRoom(room1, room0, axis, margin) || tryMoveRoom(room0, room1, axis, margin)) :
								(tryMoveRoom(room0, room1, axis, margin) || tryMoveRoom(room1, room0, axis, margin));
						}
					}

					if (movedOnAxis == false)
					{
						movedOnAxis = bPreferRoom0 ?
							(tryStepRoomTowardAnchor(room1, room0, axis) || tryStepRoomTowardAnchor(room0, room1, axis)) :
							(tryStepRoomTowardAnchor(room0, room1, axis) || tryStepRoomTowardAnchor(room1, room0, axis));
					}

					moved = moved || movedOnAxis;
				}
			}

			if (moved == false)
				break;
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_OptimizeAisleDistance.bmp");
#endif

		return true;
	}

	bool Generator::ExpandSpace(const size_t phase, const int32_t horizontalMargin, const int32_t verticalMargin) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("ExpandSpace"));
			}
		);
#endif

		int32_t minY, minZ;
		int32_t maxY, maxZ;
		int32_t minX = minY = minZ = std::numeric_limits<int32_t>::max();
		int32_t maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (minX > room->GetLeft())
				minX = room->GetLeft();
			if (minY > room->GetTop())
				minY = room->GetTop();
			if (minZ > room->GetBackground())
				minZ = room->GetBackground();
			if (maxX < room->GetRight())
				maxX = room->GetRight();
			if (maxY < room->GetBottom())
				maxY = room->GetBottom();
			if (maxZ < room->GetForeground())
				maxZ = room->GetForeground();
		}

		// 外周に余白を作る
		{
			const auto effectiveVerticalMargin = mGenerateParameter.GetExpansionPolicy() == ExpansionPolicy::Flat ? 0 : verticalMargin;
			minX -= horizontalMargin;
			minY -= horizontalMargin;
			minZ -= effectiveVerticalMargin;
			maxX += horizontalMargin;
			maxY += horizontalMargin;
			maxZ += effectiveVerticalMargin;
		}

		// 空間のサイズを設定
		mGenerateParameter.SetWidth(maxX - minX);
		mGenerateParameter.SetDepth(maxY - minY);
		mGenerateParameter.SetHeight(maxZ - minZ);

		// 空間の原点を移動（部屋の位置を移動）
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->SetX(room->GetX() - minX);
			room->SetY(room->GetY() - minY);
			room->SetZ(room->GetZ() - minZ);
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Room: W=%d,D=%d,H=%d に空間を変更しました")
			, mGenerateParameter.GetWidth(), mGenerateParameter.GetDepth(), mGenerateParameter.GetHeight()
		);
		for (const std::shared_ptr<const Room>& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: %d,X=%d,Y=%d,Z=%d W=%d,D=%d,H=%d")
				, room->GetIdentifier().Get()
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_ExpandSpace.bmp");
#endif

		return true;
	}

	void Generator::AdjustPoints() noexcept
	{
		// 部屋の中心位置をリセット
		check(mStartPoint);
		std::const_pointer_cast<Point>(mStartPoint)->ResetByRoomGroundCenter();

		// 部屋の中心位置をリセット
		check(mGoalPoint);
		std::const_pointer_cast<Point>(mGoalPoint)->ResetByRoomGroundCenter();

		// 部屋の中心位置をリセット
		for (Aisle& aisle : mAisles)
		{
			for (uint_fast8_t i = 0; i < 2; ++i)
			{
				if (const auto& point = aisle.GetPoint(i))
				{
					std::const_pointer_cast<Point>(point)->ResetByRoomGroundCenter();
				}
			}
		}
	}

	bool Generator::DetectFloorHeightAndDepthFromStart() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("DetectFloorHeightAndDepthFromStart"));
			}
		);
#endif

		mFloorHeight.clear();

		mDeepestDepthFromStart = 0;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			const int32_t z = room->GetBackground();
			if (std::find(mFloorHeight.begin(), mFloorHeight.end(), z) == mFloorHeight.end())
			{
				mFloorHeight.emplace_back(z);
			}

			if (mDeepestDepthFromStart < room->GetDepthFromStart())
				mDeepestDepthFromStart = room->GetDepthFromStart();
		}

		std::stable_sort(mFloorHeight.begin(), mFloorHeight.end());

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeightAndDepthFromStart: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

		return true;
	}

	/*
	ドロネー三角形分割した辺を最小スパニングツリーにて抽出
	*/
	bool Generator::ExtractionAisles() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("ExtractionAisles"));
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Extract Aisles"));
		DUNGEON_GENERATOR_LOG(TEXT("%d rooms detected"), mRooms.size());
#endif

		// すべての部屋の中点を記録しながら、部屋のパーツ（役割）をリセット
		std::vector<std::shared_ptr<const Point>> points;
		points.reserve(mRooms.size());
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// 部屋のパーツ（役割）をリセットする
			room->SetParts(Room::Parts::Unidentified);
			room->ResetReservationNumber();

			// Room::GetGroundCenterリストを作成
			// cppcheck-suppress [useStlAlgorithm]
			points.emplace_back(std::make_shared<const Point>(room));
		}

		uint8_t aisleComplexity = mGenerateParameter.GetAisleComplexity();
		if (aisleComplexity > 0)
		{
			float candidateAisleComplexity = std::sqrt(mGenerateParameter.GetNumberOfCandidateRooms());
			if (candidateAisleComplexity > 255.f)
				candidateAisleComplexity = 255.f;
			aisleComplexity = std::min(aisleComplexity, static_cast<uint8_t>(candidateAisleComplexity));
		}

		if (mRooms.size() >= 4)
		{
			// 三角形分割
			DelaunayTriangulation3D delaunayTriangulation(points);

#if WITH_EDITOR
			if (!delaunayTriangulation.IsValid())
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Generator:Triangulation failed. %d rooms"), mRooms.size());
				for (const auto& room : mRooms)
				{
					DUNGEON_GENERATOR_ERROR(TEXT("X:%d Y:%d Z:%d Width:%d Depth:%d Height:%d"),
						room->GetX(), room->GetY(), room->GetZ(),
						room->GetWidth(), room->GetDepth(), room->GetHeight()
					);
				}
				mLastError = Error::TriangulationFailed;
			}
#endif

			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(
				mGenerateParameter.GetRandom(),
				delaunayTriangulation,
				aisleComplexity,
				mGenerateParameter.GetStartLocationPolicy(),
				mGenerateParameter.GetStartRoomCount()
			);
			if (GenerateAisle(minimumSpanningTree) == false)
				return false;
		}
		else
		{
			// 最小スパニングツリー
			MinimumSpanningTree minimumSpanningTree(
				mGenerateParameter.GetRandom(),
				points,
				aisleComplexity,
				mGenerateParameter.GetStartLocationPolicy(),
				mGenerateParameter.GetStartRoomCount()
			);
			if (GenerateAisle(minimumSpanningTree) == false)
				return false;
		}

		// 部屋のパーツ（役割）を設定する
		SetRoomParts();

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

		return mLastError == Error::Success;
	}

	/**
	 * ExtractionAislesから呼ばれる
	 */
	bool Generator::GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->ResetGateCount();
		}

		mAisles.clear();

		minimumSpanningTree.ForEach([this](const Aisle& edge)
			{
				const std::shared_ptr<Room>& ar = edge.GetPoint(0)->GetOwnerRoom();
				const std::shared_ptr<Room>& br = edge.GetPoint(1)->GetOwnerRoom();
				if (ar == nullptr || br == nullptr)
					return;
				ar->AddGateCount(1);
				br->AddGateCount(1);
				std::shared_ptr<Point> a = std::make_shared<Point>(ar);
				std::shared_ptr<Point> b = std::make_shared<Point>(br);
				mAisles.emplace_back(edge.IsMain(), a, b);
			}
		);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		for (const auto& aisle : mAisles)
		{
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: Aisle: %d %d-%d (%f)"),
				static_cast<uint16_t>(aisle.GetIdentifier()),
				static_cast<uint16_t>(aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier()),
				static_cast<uint16_t>(aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier()),
				aisle.GetLength()
			);
		}
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("%d minimum spanning tree edges detected"), minimumSpanningTree.Size());
#endif

		// スタート位置を記録
		mStartPoint = minimumSpanningTree.GetStartPoint();
		if (mStartPoint == nullptr)
		{
			if (mRooms.empty())
				return false;
			mStartPoint = std::make_shared<Point>(mRooms.front());
		}
		mStartRoom = mStartPoint->GetOwnerRoom();

		{
			size_t startRoomCount = 0;
			for (const auto& room : mRooms)
			{
				if (room->GetParts() == Room::Parts::Start)
					++startRoomCount;
			}

			if (mGenerateParameter.GetStartLocationPolicy() == StartLocationPolicy::UseMultiStart)
			{
				const uint8_t expectedStartRoomCount = mGenerateParameter.GetStartRoomCount();
				if (startRoomCount != expectedStartRoomCount)
				{
					DUNGEON_GENERATOR_WARNING(TEXT("UseMultiStart expected %d start rooms but found %d."), expectedStartRoomCount, startRoomCount);
				}
			}
			else
			{
				if (startRoomCount != 1)
				{
					DUNGEON_GENERATOR_WARNING(TEXT("Expected a single start room for this StartLocationPolicy, but found %d."), startRoomCount);
				}
			}
		}

		// ゴール位置を記録
		mGoalPoint = minimumSpanningTree.GetGoalPoint();
		if (mGoalPoint == nullptr)
		{
			if (mRooms.empty())
				return false;
			mGoalPoint = std::make_shared<Point>(mRooms.front());
		}
		mGoalRoom = mGoalPoint->GetOwnerRoom();

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

		return true;
	}

	/**
	 * Sets RoomParts.
	 * 部屋のパーツ（役割）を設定する
	 */
	void Generator::SetRoomParts() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("SetRoomParts"));
			}
		);
#endif

		// HACK: MinimumSpanningTreeクラスにまとめた方が良いかも
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (room->GetParts() == Room::Parts::Unidentified /* && room->GetGateCount() > 1 */)
			{
				room->SetParts(Room::Parts::Hall);
			}
		}

		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Unidentified;
			}) == mRooms.end());
		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Start;
			}) != mRooms.end());
		check(std::find_if(mRooms.begin(), mRooms.end(), [](const std::shared_ptr<Room>& room)
			{
				return room->GetParts() == Room::Parts::Goal;
			}) != mRooms.end());
	}

	/**
	 * Assigns always-loaded sublevels to ordinary rooms before final endpoint selection.
	 * 最終的な端点を選択する前に、常時ロードするサブレベルを通常部屋へ割り当てます。
	 */
	bool Generator::AdjustReservedSubLevels(const size_t phase) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("AdjustReservedSubLevels"));
			}
		);
#endif

		// 必ず生成しなければならないサブレベル
		std::list<std::pair<uint32_t, FIntVector>> alwaysLoadedSubLevels;
		if (mOnQueryParts)
		{
			mOnQueryParts(alwaysLoadedSubLevels);
		}

		if (alwaysLoadedSubLevels.empty() == false)
		{
			// サブレベルを割り当てられる部屋を集める
			// （サブレベルはサイズを変更できないため、AdjustRoomSizeによる門の数に応じた自動拡張の対象外になります）
			std::vector<std::shared_ptr<Room>> candidateRooms;
			for (const std::shared_ptr<Room>& room : mRooms)
			{
				if (room->GetParts() == Room::Parts::Hall || room->GetParts() == Room::Parts::Hanare)
					candidateRooms.emplace_back(room);
			}

			while (alwaysLoadedSubLevels.empty() == false && candidateRooms.empty() == false)
			{
				const std::pair<uint32_t, FIntVector>& subLevel = alwaysLoadedSubLevels.front();
				const uint32_t subLevelArea = static_cast<uint32_t>(subLevel.second.X) * static_cast<uint32_t>(subLevel.second.Y);

				// 既に同じサイズの部屋があれば優先的に割り当てる
				auto roomIterator = std::find_if(candidateRooms.begin(), candidateRooms.end(), [&subLevel](const std::shared_ptr<Room>& room)
				{
					return
						room->GetWidth() == subLevel.second.X &&
						room->GetDepth() == subLevel.second.Y &&
						room->GetHeight() == subLevel.second.Z;
				});

				// 一致する部屋が無ければ、つながる通路の数（門の数）が最も少ない部屋へ割り当てる
				// （門の数が多い部屋にサブレベルの固定サイズを強制すると、GenerateVoxelで門を置く場所が
				//   足りなくなり生成に失敗するため）
				if (roomIterator == candidateRooms.end())
				{
					roomIterator = std::min_element(candidateRooms.begin(), candidateRooms.end(), [](const std::shared_ptr<Room>& lhs, const std::shared_ptr<Room>& rhs)
					{
						return lhs->GetGateCount() < rhs->GetGateCount();
					});
				}

				const std::shared_ptr<Room>& room = *roomIterator;
				room->SetReservationNumber(subLevel.first);
				room->SetWidth(subLevel.second.X);
				room->SetDepth(subLevel.second.Y);
				room->SetHeight(subLevel.second.Z);

				// それでも門の数に対してサブレベルが狭すぎる場合、回り道のためだけに追加された
				// ループ／ショートカット通路を間引いて門の数を減らします
				// （本流やブランチの通路は接続を維持するため間引きません）
				// 門の数がぴったり床面積に収まる場合でも、周囲を他の通路の斜面や空洞に囲まれていると
				// 門を置く余地が無くなることがあるため、少し余裕を持たせて判定します
				while (dungeon::math::Square<uint32_t>(room->GetGateCount()) >= subLevelArea)
				{
					const auto aisleIterator = std::find_if(mAisles.begin(), mAisles.end(), [&room](const Aisle& aisle)
					{
						if (aisle.GetPurpose() != EDungeonAislePurpose::Loop && aisle.GetPurpose() != EDungeonAislePurpose::Shortcut)
							return false;
						return aisle.GetPoint(0)->GetOwnerRoom() == room || aisle.GetPoint(1)->GetOwnerRoom() == room;
					});
					if (aisleIterator == mAisles.end())
					{
						DUNGEON_GENERATOR_ERROR(TEXT("AdjustReservedSubLevels: The reserved sub-level (%d,%d,%d) is too small for room ID=%d (%d Gate) even after removing every loop/shortcut aisle; dungeon generation may fail."),
							subLevel.second.X, subLevel.second.Y, subLevel.second.Z,
							static_cast<uint16_t>(room->GetIdentifier()), room->GetGateCount());
						break;
					}

					aisleIterator->GetPoint(0)->GetOwnerRoom()->RemoveGateCount(1);
					aisleIterator->GetPoint(1)->GetOwnerRoom()->RemoveGateCount(1);
					mAisles.erase(aisleIterator);
				}

				candidateRooms.erase(roomIterator);
				alwaysLoadedSubLevels.pop_front();
			}
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("AdjustReservedSubLevels: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_AdjustReservedSubLevels.bmp");
#endif

		return true;
	}

	/**
	 * Applies registered sizes to the currently selected Start and Goal rooms.
	 * 現在選択されている開始部屋とゴール部屋へ登録済みサイズを適用します。
	 */
	void Generator::ApplyEndpointRoomSizes() const noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			const FIntVector* roomSize = nullptr;
			if (room->GetParts() == Room::Parts::Start && mGenerateParameter.IsGenerateStartRoomReserved())
				roomSize = &mGenerateParameter.GetStartRoomSize();
			else if (room->GetParts() == Room::Parts::Goal && mGenerateParameter.IsGenerateGoalRoomReserved())
				roomSize = &mGenerateParameter.GetGoalRoomSize();

			if (roomSize != nullptr)
				SetRoomSizePreservingGroundCenter(*room, *roomSize);
		}
	}

	/**
	 * Represents AdjustRoomSize.
	 * 部屋の大きさを調整する
	 */
	void Generator::AdjustRoomSize(const size_t phase) const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("AdjustRoomSize"));
			}
		);
#endif

		// 狭すぎる・大きすぎる部屋を調整する
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// 予約済みの部屋はサイズを変更できない
			if (room->IsValidReservationNumber() == true)
				continue;
			// 部屋の最大サイズにあわせる
			uint32_t width = room->GetWidth();
			if (width > mGenerateParameter.GetMaxRoomWidth())
			{
				width = mGenerateParameter.GetMaxRoomWidth();
				room->SetWidth(width);
			}

			uint32_t depth = room->GetDepth();
			if (depth > mGenerateParameter.GetMaxRoomDepth())
			{
				depth = mGenerateParameter.GetMaxRoomDepth();
				room->SetDepth(depth);
			}

			uint32_t height = room->GetHeight();
			if (height > mGenerateParameter.GetMaxRoomHeight())
			{
				height = mGenerateParameter.GetMaxRoomHeight();
				room->SetHeight(height);
			}

			// ドアに対して部屋が小さい場合は広げる
			const uint32_t minimumArea = dungeon::math::Square<uint32_t>(room->GetGateCount());
			const uint32_t requiredArea = std::max(2u, minimumArea);
			if (requiredArea > width * depth)
			{
				do {
					if (width < depth)
						++width;
					else
						++depth;
				} while (requiredArea > width * depth);

				room->SetWidth(width);
				room->SetDepth(depth);

				DUNGEON_GENERATOR_LOG(TEXT("Expanded the room because it was too small for the connecting gate : ID=%d"), static_cast<uint16_t>(room->GetIdentifier()));
			}
		}

		// 水平方向の余白を追加できるか？
		if (mGenerateParameter.GetHorizontalRoomMargin() > 0)
		{
			// 部屋の余白を調整する
			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					if (room0 == room1)
						continue;

					room0->SetMarginIfRoomIntersect(room1, 2, 1);
				}
			}
		}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_AdjustRoomSize.bmp");
#endif
	}

	bool Generator::MarkBranchIdAndDepthFromStart() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("Branch"));
			}
		);
#endif

		if (mStartRoom)
		{
			uint8_t branchId = 0;
			MarkBranchIdAndDepthFromStartRecursive(mStartRoom, branchId, 0);
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("Branch: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

		return true;
	}

	void Generator::MarkBranchIdAndDepthFromStartRecursive(const std::shared_ptr<Room>& room, uint8_t& branchId, const uint8_t depth) noexcept
	{
		if (room->IsValidBranchId() == false)
			room->SetBranchId(branchId);

		if (room->GetDepthFromStart() > depth)
			room->SetDepthFromStart(depth);

#if WITH_EDITOR
		size_t aisleCount = 0;
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
				++aisleCount;
		}
		check(aisleCount == room->GetGateCount());
#endif

		for (auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				const auto newDepth = depth + 1;
				if (room != room0)
				{
					if (room->GetGateCount() >= 3)
						++branchId;
					if (room0->GetDepthFromStart() > newDepth)
						MarkBranchIdAndDepthFromStartRecursive(room0, branchId, newDepth);
				}
				if (room != room1)
				{
					if (room->GetGateCount() >= 3)
						++branchId;
					if (room1->GetDepthFromStart() > newDepth)
						MarkBranchIdAndDepthFromStartRecursive(room1, branchId, newDepth);
				}
			}
		}
	}

	/**
	 * Rebuilds locked-route room flags from the final aisle lock states.
	 * 最終的な通路のロック状態から、鍵付き経路に接する部屋のフラグを再構築します。
	 */
	void Generator::RefreshLockedRouteRoomFlags() noexcept
	{
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			room->SetLockedRouteRoom(false);
		}

		for (const Aisle& aisle : mAisles)
		{
			if (!aisle.IsAnyLocked())
			{
				continue;
			}

			aisle.GetPoint(0)->GetOwnerRoom()->SetLockedRouteRoom(true);
			aisle.GetPoint(1)->GetOwnerRoom()->SetLockedRouteRoom(true);
		}
	}

	/*
	 * MissionGraph生成後に呼び出す必要があります
	 */
	void Generator::InvokeRoomCallbacks() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("InvokeRoomCallbacks"));
			}
		);
#endif

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			switch (room->GetParts())
			{
			case Room::Parts::Start:
				if (mOnLoadStartParts)
					mOnLoadStartParts(room);
				break;

			case Room::Parts::Goal:
				if (mOnLoadGoalParts)
					mOnLoadGoalParts(room);
				break;

			case Room::Parts::Hall:
			case Room::Parts::Hanare:
				if (mOnLoadParts)
					mOnLoadParts(room);
				break;

			case Room::Parts::Unidentified:
				break;
			}
		}
	}

	bool Generator::GenerateVoxel(const size_t phase) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		MEASURE_TIME_START(stopwatch);
		Finalizer finalizer([&stopwatch]()
			{
				MEASURE_TIME_LAP(stopwatch, TEXT("GenerateVoxel"));
			}
		);
#endif

		mVoxel = std::make_shared<Voxel>(mGenerateParameter);

		// Generate room
		for (const auto& room : mRooms)
		{
			// 部屋の深さを256段階の比率にする
			uint8_t depthRatioFromStart = 0;
			if (GetDeepestDepthFromStart() > 0)
			{
				// TODO: 良く使われる処理なので、ダンジョンの深さを0～255に正規化する関数の実装を検討してください
				float depthFromStart = static_cast<float>(room->GetDepthFromStart());
				depthFromStart /= static_cast<float>(GetDeepestDepthFromStart());
				depthRatioFromStart = static_cast<uint8_t>(depthFromStart * 255.f);
			}

			const FIntVector min(room->GetLeft(), room->GetTop(), room->GetBackground());
			const FIntVector max(room->GetRight(), room->GetBottom(), room->GetForeground());
			mVoxel->Rectangle(min, max
				, Grid::CreateFloor(mGenerateParameter.GetRandom(), room->GetIdentifier(), depthRatioFromStart, room->GetStructuralRole(), room->GetGameplayRole(), room->GetZoneIndex())
				, Grid::CreateDeck(mGenerateParameter.GetRandom(), room->GetIdentifier(), depthRatioFromStart, room->GetStructuralRole(), room->GetGameplayRole(), room->GetZoneIndex())
			);

			// 部屋に吹き抜けを生成する
			GenerateRoomSkylightVoxel(room, depthRatioFromStart);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			// 通信同期用に現在の乱数の種を出力する
			{
				uint32_t x, y, z, w;
				GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: room %x generated : RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), static_cast<uint16_t>(room->GetIdentifier()), x, y, z, w, CalculateCRC32());
			}
#endif
		}

		if (mOnPreGenerateVoxel)
		{
			mOnPreGenerateVoxel(mVoxel);
		}

		/*
		 * サイズを変更できない部屋が門を置けるように、門の外側のグリッドを確保します
		 * サブレベルの壁の情報はmOnPreGenerateVoxelで反映されるため、その後に呼び出す必要があります
		 */
		ReserveGateApproachVoxel();

		/*
		 * 施錠される通路の識別子を登録します
		 * 門や通路を共有できるかの判定に使用します
		 */
		{
			std::unordered_set<Identifier::IdentifierType> lockedAisleIdentifiers;
			for (const Aisle& aisle : mAisles)
			{
				if (aisle.IsAnyLocked())
					lockedAisleIdentifiers.emplace(static_cast<Identifier::IdentifierType>(aisle.GetIdentifier()));
			}
			mVoxel->SetLockedAisleIdentifiers(std::move(lockedAisleIdentifiers));
		}

		// 通路の距離が短い順に並べ替える
		std::stable_sort(mAisles.begin(), mAisles.end(), [](const Aisle& l, const Aisle& r)
			{
				/*
				 * 施錠される通路は門を共有できないため、先に生成して専用の門を確保させます
				 * 後回しにすると、共有可能な通路が先にDeckを門へ変えてしまい門を置けなくなります
				 */
				if (l.IsAnyLocked() != r.IsAnyLocked())
					return l.IsAnyLocked();

				// メインルート以外のソートキー（優先させない）
				static constexpr double AlternativeRouteCost = 10000. * 100.;
				double lLength = l.GetLength();
				if (l.IsMain() == false)
					lLength += AlternativeRouteCost;

				double rLength = r.GetLength();
				if (r.IsMain() == false)
					rLength += AlternativeRouteCost;

				return lLength < rLength;
			}
		);

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION) || defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		for (const auto& aisle : mAisles)
		{
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Aisle: %d %d-%d (%f) %c"),
				static_cast<uint16_t>(aisle.GetIdentifier()),
				static_cast<uint16_t>(aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier()),
				static_cast<uint16_t>(aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier()),
				aisle.GetLength(),
				aisle.IsMain() ? TCHAR('M') : TCHAR(' ')
			);
		}
#endif

		// Generate pathways
		// 通路の生成
		for (Aisle& aisle : mAisles)
		{
			// 通路の天井の高さを決定
			uint8_t aisleHeight;
			switch (mGenerateParameter.GetAisleCeilingHeightPolicy())
			{
			case AisleCeilingHeightPolicy::TwoGrids:
				aisleHeight = 2;
				break;
			case AisleCeilingHeightPolicy::OneGrid:
				aisleHeight = 1;
				break;
			case AisleCeilingHeightPolicy::Random:
			default:
				aisleHeight = mGenerateParameter.GetRandom()->Get<uint8_t>(2) + 1;
				break;
			}
			aisle.SetHeight(aisleHeight);
		}

		/*
		 * 各部屋があと何本の通路を受け入れる必要があるかを数えます
		 * 通路を生成するたびに減らし、門の余裕が少ない部屋ほど壁際を通りにくくします
		 */
		std::unordered_map<Identifier::IdentifierType, uint8_t> remainingGates;
		remainingGates.reserve(mRooms.size());
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (room->GetGateCount() > 0)
				remainingGates.emplace(static_cast<Identifier::IdentifierType>(room->GetIdentifier()), room->GetGateCount());
		}

		for (size_t i = 0; i < mAisles.size(); ++i)
		{
			const Aisle& aisle = mAisles[i];
			std::shared_ptr<const Point> startPoint = aisle.GetPoint(0);
			std::shared_ptr<const Point> goalPoint = aisle.GetPoint(1);

			// 経路探索は並列に実行されるため、通路の生成を始める前に確定させます
			UpdateRoomGateScarcity(remainingGates);

			// Select the deeper endpoint independently from pathfinding direction and height-based endpoint swaps.
			// 経路探索方向や高さによる端点の入れ替えとは独立して、深度の大きい接続先を選択します。
			const int32 aisleZoneIndex = aisle.GetZoneIndex();

			// Use the back room as a starting point
			if (startPoint->GetOwnerRoom()->GetDepthFromStart() < goalPoint->GetOwnerRoom()->GetDepthFromStart())
			{
				std::swap(startPoint, goalPoint);
			}

			// 通路は奥の部屋の深さにあわせ、256段階の比率にする
			uint8_t depthRatioFromStart = 0;
			if (GetDeepestDepthFromStart() > 0)
			{
				float depthFromStart = static_cast<float>(goalPoint->GetOwnerRoom()->GetDepthFromStart());
				depthFromStart /= static_cast<float>(GetDeepestDepthFromStart());
				depthRatioFromStart = static_cast<uint8_t>(depthFromStart * 255.f);
			}

			// Check if the start and end points are included in the room
			check(startPoint->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*startPoint)));
			check(goalPoint->GetOwnerRoom()->GetRect().Contains(ToIntPoint(*goalPoint)));

			const int32 startPointZ = startPoint->Z;
			const int32 goalPointZ = goalPoint->Z;
			AisleVoxelResult aisleVoxelResult = AisleVoxelResult::Failed;
			if (startPointZ == goalPointZ)
				// 開始門と終了門が同じ高さにある？
				aisleVoxelResult = GenerateAisleVoxel(i, aisle, startPoint, goalPoint, depthRatioFromStart, aisleZoneIndex, false);
			else if (startPointZ < goalPointZ)
				// 開始門が終了門よりも低い高さにある？
				aisleVoxelResult = GenerateAisleVoxel(i, aisle, startPoint, goalPoint, depthRatioFromStart, aisleZoneIndex, mGenerateParameter.IsGenerateSlopeInRoom());
			else
				// 終了門が開始門よりも低い高さにある？
				aisleVoxelResult = GenerateAisleVoxel(i, aisle, goalPoint, startPoint, depthRatioFromStart, aisleZoneIndex, mGenerateParameter.IsGenerateSlopeInRoom());

			// Abort here because generating the remaining aisles is pointless once the dungeon cannot be completed.
			// ダンジョンとして成立しないならば、残りの通路を生成しても無意味なので生成を中止します
			if (aisleVoxelResult == AisleVoxelResult::Failed)
			{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
				/*
				 * 失敗した状況を確認できるよう、成功時と同じ画像を出力します
				 * 生成できなかった通路の両端はFailedAisleColorで描かれます
				 */
				mVoxel->GenerateImageForDebug("/debug/" + std::to_string(phase) + "_GenerateVoxel.bmp", mFloorHeight);
#endif
				return false;
			}

			// この通路が消費した分の門を減らします
			for (uint_fast8_t pointIndex = 0; pointIndex < 2; ++pointIndex)
			{
				const std::shared_ptr<Room>& room = aisle.GetPoint(pointIndex)->GetOwnerRoom();
				if (room == nullptr)
					continue;
				const auto remaining = remainingGates.find(static_cast<Identifier::IdentifierType>(room->GetIdentifier()));
				if (remaining != remainingGates.end() && remaining->second > 0)
					--remaining->second;
			}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			// 通信同期用に現在の乱数の種を出力する
			{
				uint32_t x, y, z, w;
				GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: aisle generated: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
			}
#endif

			// Skipped aisles own no voxels, so there is nothing to raise.
			// 生成を諦めた通路はボクセルを持たないため、天井を拡張する必要はありません
			if (aisleVoxelResult == AisleVoxelResult::Succeeded && aisle.GetHeight() > 1)
			{
				ExpandAisleHeightVoxel(aisle);
			}
		}

		// 全ての通路を生成したので、門のために確保したグリッドを解放します
		mVoxel->ReleaseGateApproachLocations();
		mVoxel->ClearLockedAisleIdentifiers();


		if (mGenerateParameter.IsGenerateStructuralColumn())
		{
			for (const auto& room : mRooms)
			{
				GenerateStructuralColumnVoxel(room);
			}
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: finish: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x: CRC32=%x"), x, y, z, w, CalculateCRC32());
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		mVoxel->GenerateImageForDebug("/debug/" + std::to_string(phase) + "_GenerateVoxel.bmp", mFloorHeight);
#endif

		return true;
	}

	void Generator::UpdateMeshAttributes() const noexcept
	{
		if (!mVoxel)
			return;

		constexpr bool mergeRooms = false;
		mVoxel->Each([this, mergeRooms](const FIntVector& location, Grid& grid)
			{
				const Grid& northGrid = mVoxel->Get(location.X, location.Y - 1, location.Z);
				const Grid& southGrid = mVoxel->Get(location.X, location.Y + 1, location.Z);
				const Grid& eastGrid = mVoxel->Get(location.X + 1, location.Y, location.Z);
				const Grid& westGrid = mVoxel->Get(location.X - 1, location.Y, location.Z);
				const Grid& upperGrid = mVoxel->Get(location.X, location.Y, location.Z + 1);

				grid.SetNorthWall(grid.CanBuildWall(northGrid, Direction::North, mergeRooms, false));
				grid.SetSouthWall(grid.CanBuildWall(southGrid, Direction::South, mergeRooms, false));
				grid.SetEastWall(grid.CanBuildWall(eastGrid, Direction::East, mergeRooms, false));
				grid.SetWestWall(grid.CanBuildWall(westGrid, Direction::West, mergeRooms, false));
				grid.SetFloor(grid.CanBuildSlope() || grid.CanBuildFloor(true));
				grid.SetCeiling(grid.CanBuildRoof(upperGrid, true));
				return true;
			}
		);
	}

	void Generator::ExpandAisleHeightVoxel(const Aisle& aisle) const noexcept
	{
		if (!mVoxel)
			return;

		std::vector<std::pair<FIntVector, Grid>> upSpaceGrids;
		mVoxel->Each([this, &aisle, &upSpaceGrids](const FIntVector& location, const Grid& grid)
			{
				if ((!grid.IsKindOfAisleType() && !grid.IsKindOfGateType()) || grid.GetIdentifier() != aisle.GetIdentifier())
					return true;

				const FIntVector upperLocation = location + FIntVector(0, 0, 1);
				if (!mVoxel->Contain(upperLocation))
					return true;

				const Grid& upperGrid = mVoxel->Get(upperLocation);
				if (!upperGrid.Is(Grid::Type::Empty))
					return true;

				Grid upSpace = grid;
				upSpace.SetType(Grid::Type::UpSpace);
				upSpace.MergeAisle(grid.CanMergeAisle());
				upSpace.SetProps(Grid::Props::None);
				upSpaceGrids.emplace_back(upperLocation, upSpace);
				return true;
			}
		);

		for (const auto& upSpaceGrid : upSpaceGrids)
		{
			mVoxel->Set(upSpaceGrid.first, upSpaceGrid.second);
		}
	}

	void Generator::GenerateRoomSkylightVoxel(const std::shared_ptr<Room>& room, const uint8_t depthRatioFromStart) noexcept
	{
		if (!mVoxel || !room)
			return;

		if (room->GetWidth() < 3 || room->GetDepth() < 3)
			return;

		if (mGenerateParameter.GetRandom()->Get<uint8_t>(0, 99) >= mGenerateParameter.GetSkylightChancePercent())
			return;

		const int32 x = mGenerateParameter.GetRandom()->Get<int32>(room->GetLeft() + 1, room->GetRight() - 1);
		const int32 y = mGenerateParameter.GetRandom()->Get<int32>(room->GetTop() + 1, room->GetBottom() - 1);
		const int32 z = room->GetForeground();
		const FIntVector skylightLocation(x, y, z);
		if (!mVoxel->Contain(skylightLocation))
			return;

		if (!mVoxel->Get(skylightLocation).Is(Grid::Type::Empty))
			return;

		Grid upSpace(Grid::Type::UpSpace);
		upSpace.SetIdentifier(room->GetIdentifier());
		upSpace.SetDepthRatioFromStart(depthRatioFromStart);
		upSpace.SetRoomStructuralRole(room->GetStructuralRole());
		upSpace.SetRoomGameplayRole(room->GetGameplayRole());
		upSpace.SetZoneIndex(room->GetZoneIndex());
		upSpace.SetProps(Grid::Props::None);
		mVoxel->Set(skylightLocation, upSpace);
	}

	/**
	 * Keeps the grids in front of a size-fixed room's openings free from other aisles.
	 * サイズを変更できない部屋の門の外側のグリッドが、他の通路に塞がれないように確保します。
	 */
	void Generator::ReserveGateApproachVoxel() const noexcept
	{
		if (!mVoxel)
			return;

		/*
		 * 門を置ける面に十分な余裕がある部屋まで確保すると、他の通路の経路探索を妨げてしまいます
		 * 門の数に対して余裕がわずかしかない部屋だけを確保の対象にします
		 */
		static constexpr size_t GateApproachReservationMargin = 1;

		std::vector<FIntVector> approachLocations;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (IsRoomSizeFixed(*room) == false)
				continue;

			mVoxel->CollectGateApproachLocations(approachLocations, room->GetRect(), room->GetBackground(), room->GetIdentifier());
			if (approachLocations.size() > static_cast<size_t>(room->GetGateCount()) + GateApproachReservationMargin)
				continue;

			mVoxel->ReserveGateApproachLocations(approachLocations, room->GetIdentifier());

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: Reserved %d gate approach grids for room ID=%d (%d Gate)")
				, approachLocations.size(), static_cast<uint16_t>(room->GetIdentifier()), room->GetGateCount());
#endif
		}
	}

	/**
	 * Rebuilds the extra path cost charged for running along each room's wall.
	 * A grid next to a room can become that room's gate, so an aisle that hugs a room it does not
	 * connect to takes gate candidates away from it. Rooms that still need many gates but have few
	 * candidates left are the ones that must be protected, and a room whose openings are fixed by a
	 * sub-level naturally lands there because it can never gain a new candidate.
	 * 部屋の壁際を通る事に対する追加コストを作り直します。
	 * 部屋に隣接するグリッドはその部屋の門になり得るため、接続しない部屋に貼り付いて進む通路は
	 * その部屋から門の候補を奪います。まだ多くの門を必要とするのに候補が残り少ない部屋ほど
	 * 守る必要があり、サブレベルで開口部が決まっている部屋は候補を増やせないため自然とそこに入ります。
	 */
	void Generator::UpdateRoomGateScarcity(const std::unordered_map<Identifier::IdentifierType, uint8_t>& remainingGates) const noexcept
	{
		if (!mVoxel)
			return;

		/*
		 * 部屋を1周する経路が選ばれないよう、壁際のグリッドには常に基本コストを課します
		 * 通常の移動コストと同じ値にして、壁際を通る歩数あたりの費用を倍にします
		 */
		static constexpr uint32_t BaseRoomProximityCost = 2;

		// 門の候補が枯れかけている部屋へ上乗せするコストの上限です
		static constexpr uint32_t MaxRoomScarcityCost = 14;

		std::unordered_map<Identifier::IdentifierType, uint32_t> scarcity;
		scarcity.reserve(mRooms.size());

		std::vector<FIntVector> approachLocations;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			const Identifier::IdentifierType identifier = static_cast<Identifier::IdentifierType>(room->GetIdentifier());
			const auto remaining = remainingGates.find(identifier);
			if (remaining == remainingGates.end() || remaining->second == 0)
				continue;

			mVoxel->CollectGateApproachLocations(approachLocations, room->GetRect(), room->GetBackground(), room->GetIdentifier());

			const uint32_t required = remaining->second;
			const uint32_t available = static_cast<uint32_t>(approachLocations.size());
			const uint32_t scarcityCost = available <= required
				? MaxRoomScarcityCost
				: (MaxRoomScarcityCost * required) / available;
			scarcity.emplace(identifier, BaseRoomProximityCost + scarcityCost);
		}

		mVoxel->SetRoomGateScarcity(std::move(scarcity));
	}

	/**
	 * Returns whether every room can be reached from the start room through the aisles that were built.
	 * An aisle that was given up during voxel generation was never dug, so it does not connect anything.
	 * The room that only that aisle reached is still placed and still shows on the minimap, which is
	 * how a player notices it.
	 * 生成した通路で、開始部屋から全ての部屋へ到達できるかを返します。
	 * ボクセル生成で諦めた通路は実際には掘られていないため、何も接続しません。
	 * その通路でしかつながっていなかった部屋は配置されたまま残り、ミニマップにも表示されるため、
	 * プレイヤーからは入れない部屋として見えます。
	 */
	bool Generator::VerifyRoomReachability() noexcept
	{
		if (mRooms.empty() || mStartRoom == nullptr)
			return true;

		const auto ownerRoomIdentifier = [](const Aisle& aisle, const uint_fast8_t pointIndex) -> Identifier::IdentifierType
			{
				const std::shared_ptr<const Point>& point = aisle.GetPoint(pointIndex);
				const std::shared_ptr<Room>& room = point != nullptr ? point->GetOwnerRoom() : nullptr;
				return room != nullptr
					? static_cast<Identifier::IdentifierType>(room->GetIdentifier())
					: std::numeric_limits<Identifier::IdentifierType>::max();
			};

		constexpr Identifier::IdentifierType InvalidRoom = std::numeric_limits<Identifier::IdentifierType>::max();

		// 実際に掘られた通路だけで接続を作ります
		std::unordered_map<Identifier::IdentifierType, std::vector<Identifier::IdentifierType>> adjacency;
		for (const Aisle& aisle : mAisles)
		{
			if (mAbandonedAisleIdentifiers.find(static_cast<Identifier::IdentifierType>(aisle.GetIdentifier())) != mAbandonedAisleIdentifiers.end())
				continue;

			const Identifier::IdentifierType room0 = ownerRoomIdentifier(aisle, 0);
			const Identifier::IdentifierType room1 = ownerRoomIdentifier(aisle, 1);
			if (room0 == InvalidRoom || room1 == InvalidRoom || room0 == room1)
				continue;

			adjacency[room0].emplace_back(room1);
			adjacency[room1].emplace_back(room0);
		}

		std::unordered_set<Identifier::IdentifierType> visited;
		std::vector<Identifier::IdentifierType> pending;
		const Identifier::IdentifierType startRoom = static_cast<Identifier::IdentifierType>(mStartRoom->GetIdentifier());
		visited.emplace(startRoom);
		pending.emplace_back(startRoom);
		while (pending.empty() == false)
		{
			const Identifier::IdentifierType current = pending.back();
			pending.pop_back();
			const auto neighbors = adjacency.find(current);
			if (neighbors == adjacency.end())
				continue;
			for (const Identifier::IdentifierType next : neighbors->second)
			{
				if (visited.emplace(next).second)
					pending.emplace_back(next);
			}
		}

		size_t isolatedRoomCount = 0;
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			if (visited.find(static_cast<Identifier::IdentifierType>(room->GetIdentifier())) != visited.end())
				continue;

			++isolatedRoomCount;
#if WITH_EDITOR
			DUNGEON_GENERATOR_ERROR(TEXT("The start room cannot reach room ID=%d (%d,%d,%d) %s")
				, static_cast<uint16_t>(room->GetIdentifier())
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetItem() != Room::Item::Empty ? TEXT("holding an item") : TEXT(""));
#endif
		}

		if (isolatedRoomCount == 0)
			return true;

		DUNGEON_GENERATOR_ERROR(TEXT("%d rooms cannot be reached from the start room."), static_cast<int32>(isolatedRoomCount));
		mLastError = Error::RoomIsolated;
		return false;
	}

	bool Generator::IsRoomSizeFixed(const Room& room) const noexcept
	{
		// サブレベルを割り当てた部屋はサイズを変更できません
		if (room.IsValidReservationNumber())
			return true;

		// サイズを予約した開始部屋とゴール部屋はサイズを変更できません
		if (room.GetParts() == Room::Parts::Start && mGenerateParameter.IsGenerateStartRoomReserved())
			return true;
		if (room.GetParts() == Room::Parts::Goal && mGenerateParameter.IsGenerateGoalRoomReserved())
			return true;

		return false;
	}

	Generator::AisleVoxelResult Generator::GenerateAisleVoxel(const size_t aisleIndex, const Aisle& aisle, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint, const uint8_t depthRatioFromStart, const int32 aisleZoneIndex, const bool generateIndoorSlope) noexcept
	{
		constexpr size_t MaxResultCount = 8;

		// Change to voxel coordinates
		FIntVector start = ToIntVector(*startPoint);
		FIntVector goal = ToIntVector(*goalPoint);

		// Conditions for reaching the passage. The endpoint can be anywhere in the goal room.
		const std::shared_ptr<Room>& goalRoom = goalPoint->GetOwnerRoom();
		check(goalRoom);
		const PathGoalCondition pathGoalCondition(goalRoom->GetRect());

		// ゴール地点周辺でゲートを生成できるボクセルを探す
		std::vector<Voxel::CandidateLocation> goalToStart;
		// 施錠される通路は鍵を書き込む専用の門を必要とするため、門を共有しません
		const bool shareGate = aisle.IsAnyLocked() == false;
		if (!mVoxel->SearchGateLocation(goalToStart, MaxResultCount, goal, goalPoint->GetOwnerRoom()->GetIdentifier(), start, shareGate))
		{
			return HandleAisleVoxelFailure(aisleIndex, aisle, AisleVoxelFailure::GoalGateNotFound, startPoint, goalPoint);
		}

		/*
		 * 室内にスロープを生成します
		 * 通路のスタート位置が通路のゴール位置よりも高い必要があります
		 */
		if (generateIndoorSlope)
		{
			RoomStructureGenerator roomStructureGenerator;
			if (roomStructureGenerator.CheckSlopePlacement(mVoxel, startPoint->GetOwnerRoom(), goal, mGenerateParameter.GetRandom()))
			{
				std::vector<Voxel::CandidateLocation> startToGoal;
				startToGoal.reserve(1);
				startToGoal.emplace_back(0, roomStructureGenerator.GetGateLocation());

				Voxel::AisleParameter aisleParameter;
				aisleParameter.mGoalCondition = pathGoalCondition;
				aisleParameter.mIdentifier = aisle.GetIdentifier();
				aisleParameter.mZoneIndex = aisleZoneIndex;
				// 施錠される通路は鍵を書き込む専用の門と廊下を必要とするため、交差点を生成しません
				aisleParameter.mGenerateIntersections = aisle.IsAnyLocked() == false && mGenerateParameter.IsAisleComplexity();
				aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
				aisleParameter.mLocked = aisle.IsLocked();
				aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
				aisleParameter.mStartRoomIdentifier = static_cast<Identifier::IdentifierType>(startPoint->GetOwnerRoom()->GetIdentifier());
				aisleParameter.mGoalRoomIdentifier = static_cast<Identifier::IdentifierType>(goalPoint->GetOwnerRoom()->GetIdentifier());
				if (mVoxel->Aisle(startToGoal, goalToStart, aisleParameter))
				{
					roomStructureGenerator.GenerateSlope(mVoxel);
					return AisleVoxelResult::Succeeded;
				}
			}
		}

		// スタート地点周辺でゲートを生成できるボクセルを探す
		std::vector<Voxel::CandidateLocation> startToGoal;
		if (!mVoxel->SearchGateLocation(startToGoal, MaxResultCount, start, startPoint->GetOwnerRoom()->GetIdentifier(), goal, shareGate))
		{
			return HandleAisleVoxelFailure(aisleIndex, aisle, AisleVoxelFailure::StartGateNotFound, startPoint, goalPoint);
		}

		Voxel::AisleParameter aisleParameter;
		aisleParameter.mGoalCondition = pathGoalCondition;
		aisleParameter.mIdentifier = aisle.GetIdentifier();
		aisleParameter.mZoneIndex = aisleZoneIndex;
		// 施錠される通路は鍵を書き込む専用の門と廊下を必要とするため、交差点を生成しません
		aisleParameter.mGenerateIntersections = aisle.IsAnyLocked() == false && mGenerateParameter.IsAisleComplexity();
		aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
		aisleParameter.mLocked = aisle.IsLocked();
		aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
		aisleParameter.mStartRoomIdentifier = static_cast<Identifier::IdentifierType>(startPoint->GetOwnerRoom()->GetIdentifier());
		aisleParameter.mGoalRoomIdentifier = static_cast<Identifier::IdentifierType>(goalPoint->GetOwnerRoom()->GetIdentifier());
		if (mVoxel->Aisle(startToGoal, goalToStart, aisleParameter) == false)
		{
			/*
			 * 交差点を生成しない設定では、先に引かれた通路が細い隙間を塞いだまま譲りません。
			 * 部屋の余白が狭い配置では通路同士が塞ぎ合って大量の部屋が孤立するため、
			 * 諦める前に交差点を許可して一度だけ引き直します。
			 * 施錠された通路は迂回路を与えないよう、ここでも交差点を許可しません。
			 */
			const bool canRetryWithIntersections = aisleParameter.mGenerateIntersections == false && aisle.IsAnyLocked() == false;
			if (canRetryWithIntersections == false)
			{
				return HandleAisleVoxelFailure(aisleIndex, aisle, AisleVoxelFailure::RouteNotFound, startPoint, goalPoint);
			}

			aisleParameter.mGenerateIntersections = true;
			if (mVoxel->Aisle(startToGoal, goalToStart, aisleParameter) == false)
			{
				return HandleAisleVoxelFailure(aisleIndex, aisle, AisleVoxelFailure::RouteNotFound, startPoint, goalPoint);
			}
		}

		return AisleVoxelResult::Succeeded;
	}

	/**
	 * Reports an aisle voxel generation failure and decides whether the dungeon can still be completed.
	 * 通路のボクセル生成の失敗を報告し、ダンジョンとして成立させられるかを判定します。
	 */
	Generator::AisleVoxelResult Generator::HandleAisleVoxelFailure(const size_t aisleIndex, const Aisle& aisle, const AisleVoxelFailure failure, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint) noexcept
	{
		/*
		 * 経路の検索に失敗した場合のみ、幹線通路以外であれば生成を諦めて続行します
		 * 門の検索に失敗した場合は生成を諦めません
		 * 幹線通路以外でも、その通路が部屋への唯一の接続ならば部屋が到達不能になるためです
		 * （IsMainは開始部屋からゴール部屋への最短経路上かどうかしか表さないので、
		 *   到達可能性の判定には使用できません）
		 */
		if (failure == AisleVoxelFailure::RouteNotFound && aisle.IsMain() == false)
		{
#if WITH_EDITOR
			DUNGEON_GENERATOR_WARNING(TEXT("Abandoned a non-main aisle because it could not be generated (%s). %d: ID=%d (%d,%d,%d)-(%d,%d,%d)")
				, GetAisleVoxelFailureName(failure), aisleIndex, static_cast<uint16_t>(aisle.GetIdentifier())
				, static_cast<int32>(startPoint->X), static_cast<int32>(startPoint->Y), static_cast<int32>(startPoint->Z)
				, static_cast<int32>(goalPoint->X), static_cast<int32>(goalPoint->Y), static_cast<int32>(goalPoint->Z));
#endif
			mAbandonedAisleIdentifiers.emplace(static_cast<Identifier::IdentifierType>(aisle.GetIdentifier()));
			return AisleVoxelResult::Skipped;
		}

#if WITH_EDITOR
		ReportAisleVoxelFailure(aisleIndex, aisle, failure, startPoint, goalPoint);
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		// デバッグ画像へ、生成できなかった通路の両端を描かせます
		if (mVoxel && startPoint != nullptr && goalPoint != nullptr)
			mVoxel->AddFailedAisleEndpoints(ToIntVector(*startPoint), ToIntVector(*goalPoint));
#endif

		mLastError = failure == AisleVoxelFailure::RouteNotFound ? Error::RouteSearchFailed : Error::GateSearchFailed;

		/*
		 * 門が足りなくなった部屋を覚えます
		 * 利用者へどのサブレベルのアセットを直せば良いか伝えるためです
		 */
		if (mLastError == Error::GateSearchFailed)
		{
			const std::shared_ptr<const Point>& point = failure == AisleVoxelFailure::GoalGateNotFound ? goalPoint : startPoint;
			const std::shared_ptr<Room>& room = point != nullptr ? point->GetOwnerRoom() : nullptr;
			mLastErrorRoomParts = room != nullptr ? room->GetParts() : Room::Parts::Unidentified;
		}

		return AisleVoxelResult::Failed;
	}

	void Generator::GenerateStructuralColumnVoxel(const std::shared_ptr<Room>& room) const
	{
		// サブレベル生成済みの部屋には構造柱を生成できない
		if (room->IsValidReservationNumber())
			return;

		const int32 minZ = room->GetBackground();
		const int32 maxZ = room->GetForeground();

		int32 count = static_cast<int32>(std::sqrt(static_cast<float>(room->GetRect().Area())));
		if (count >= 5)
		{
			for (int32 i = 0; i < count; ++i)
			{
				const int32 x = mGenerateParameter.GetRandom()->Get(room->GetLeft(), room->GetRight());
				const int32 y = mGenerateParameter.GetRandom()->Get(room->GetTop(), room->GetBottom());
				if (CanFillStructuralColumnVoxel(x, y, minZ, maxZ))
					FillStructuralColumnVoxel(x, y, minZ, maxZ);
			}
		}

		if (mGenerateParameter.GetRandom()->Get(3) == 0)
		{
			const int32 x = room->GetLeft();
			const int32 y = room->GetTop();
			if (CanFillStructuralColumnVoxel(x, y, minZ, maxZ))
				FillStructuralColumnVoxel(x, y, minZ, maxZ);
		}
		if (mGenerateParameter.GetRandom()->Get(3) == 0)
		{
			const int32 x = room->GetRight() - 1;
			const int32 y = room->GetTop();
			if (CanFillStructuralColumnVoxel(x, y, minZ, maxZ))
				FillStructuralColumnVoxel(x, y, minZ, maxZ);
		}
		if (mGenerateParameter.GetRandom()->Get(3) == 0)
		{
			const int32 x = room->GetLeft();
			const int32 y = room->GetBottom() - 1;
			if (CanFillStructuralColumnVoxel(x, y, minZ, maxZ))
				FillStructuralColumnVoxel(x, y, minZ, maxZ);
		}
		if (mGenerateParameter.GetRandom()->Get(3) == 0)
		{
			const int32 x = room->GetRight() - 1;
			const int32 y = room->GetBottom() - 1;
			if (CanFillStructuralColumnVoxel(x, y, minZ, maxZ))
				FillStructuralColumnVoxel(x, y, minZ, maxZ);
		}
	}

	bool Generator::CanFillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const
	{
		check(minZ <= maxZ);
		static const std::array<FIntVector2, 8> offsets = {
			{
				FIntVector2(-1, -1), FIntVector2(0, -1), FIntVector2(1, -1),
				FIntVector2(-1,  0),                             FIntVector2(1,  0),
				FIntVector2(-1,  1), FIntVector2(0,  1), FIntVector2(1,  1)
			}
		};
		int32 z = minZ;
		while (z < maxZ)
		{
			// 中心（構造柱の中）の確認
			{
				const FIntVector location(x, y, z);
				const Grid& grid = mVoxel->Get(location);
				if (grid.IsReserved())
					return false;
				if (grid.IsCatwalk())
					return false;
				switch (grid.GetType())
				{
				case Grid::Type::Gate:
				case Grid::Type::Slope:
				case Grid::Type::Stairwell:
				case Grid::Type::DownSpace:
				case Grid::Type::UpSpace:
				case Grid::Type::StructuralColumn:
					return false;

				//case Grid::Type::Aisle: 部屋の中に通路があったら異常な状態
				default:
					break;
				}
			}

			// 周辺の確認
			for (const auto& offset : offsets)
			{
				const FIntVector location(
					x + offset.X,
					y + offset.Y,
					z
				);
				switch (mVoxel->Get(location).GetType())
				{
				case Grid::Type::Slope:
				case Grid::Type::DownSpace:
				case Grid::Type::StructuralColumn:
					return false;

				default:
					break;
				}
			}
			++z;
		}
		return true;
	}

	void Generator::FillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const
	{
		check(minZ <= maxZ);
		FIntVector location(x, y, minZ);
		while (location.Z < maxZ)
		{
			auto grid = mVoxel->Get(location);
			grid.SetType(Grid::Type::StructuralColumn);
			grid.ResetIdentifier();
			mVoxel->Set(location, grid);
			++location.Z;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下は生成完了後に操作する関数です。
	////////////////////////////////////////////////////////////////////////////////////////////////
	const std::vector<int32_t>& Generator::GetFloorHeight() const
	{
		return mFloorHeight;
	}

	size_t Generator::FindFloor(const int32_t height) const
	{
		const std::vector<int32_t>& floorHeight = GetFloorHeight();
		if (floorHeight.empty())
			return 0;

		const auto upperFloor = std::upper_bound(floorHeight.begin(), floorHeight.end(), height);
		if (upperFloor == floorHeight.begin())
			return 0;

		return static_cast<size_t>(std::distance(floorHeight.begin(), upperFloor) - 1);
	}

	std::shared_ptr<Room> Generator::Find(const Point& point) const noexcept
	{
		const auto i = std::find_if(mRooms.begin(), mRooms.end(), [&point](const std::shared_ptr<Room>& room)
			{
				return room->Contain(point);
			}
		);
		return i != mRooms.end() ? *i : nullptr;
	}

	const GenerateParameter& Generator::GetGenerateParameter() const noexcept
	{
		return mGenerateParameter;
	}

	const std::shared_ptr<Voxel>& Generator::GetVoxel() const noexcept
	{
		return mVoxel;
	}

	size_t Generator::GetRoomCount() const noexcept
	{
		return mRooms.size();
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByRoute(const std::shared_ptr<Room>& room) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());

		if (IsRoutePassable(room) == true)
			result.emplace_back(room);

		std::unordered_set<const Aisle*> passableAisles;
		FindByRoute(result, passableAisles, room);

		result.shrink_to_fit();
		return result;
	}

	/**
	 * 入力された部屋から経路検索上通過可能な部屋を検索します
	 * @param[inout]	passableRooms	通過可能な部屋の一覧
	 * @param[inout]	passableAisles	通過可能な通路の一覧
	 * @param[in]		room			検索する部屋
	 */
	void Generator::FindByRoute(std::vector<std::shared_ptr<Room>>& passableRooms, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			// 鍵付き扉がある通路なら何もしない
			if (aisle.IsLocked())
				continue;

			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (passableAisles.contains(&aisle) == true)
					continue;
				passableAisles.emplace(&aisle);

				if (room == room0)
				{
					if (IsRoutePassable(room1) == true)
						passableRooms.emplace_back(room1);
					FindByRoute(passableRooms, passableAisles, room1);
				}
				else
				{
					if (IsRoutePassable(room0) == true)
						passableRooms.emplace_back(room0);
					FindByRoute(passableRooms, passableAisles, room0);
				}
			}
		}
	}

	/**
	 * Returns whether RoutePassable.
	 * 入力された部屋が経路検索上通過可能か判定します
	 */
	bool Generator::IsRoutePassable(const std::shared_ptr<Room>& room) noexcept
	{
		return
			// 予約済みの部屋は対象外
			room->IsValidReservationNumber() == false &&
			// アイテムがあると対象外
			room->GetItem() == Room::Item::Empty &&
			// ホールとはなれだけ対象
			(room->GetParts() == Room::Parts::Hall || room->GetParts() == Room::Parts::Hanare);
	}


	void Generator::SetFloor(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetFloor(position, enable);
	}

	void Generator::SetCeiling(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetCeiling(position, enable);
	}

	void Generator::SetNorthWall(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetNorthWall(position, enable);
	}

	void Generator::SetSouthWall(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetSouthWall(position, enable);
	}

	void Generator::SetEastWall(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetEastWall(position, enable);
	}

	void Generator::SetWestWall(const FIntVector& position, const bool enable) const noexcept
	{
		mVoxel->SetWestWall(position, enable);
	}


	bool Generator::HasFloor(const FIntVector& position) const noexcept
	{
		return mVoxel->HasFloor(position);
	}

	bool Generator::HasCeiling(const FIntVector& position) const noexcept
	{
		return mVoxel->HasCeiling(position);
	}

	bool Generator::HasNorthWall(const FIntVector& position) const noexcept
	{
		return mVoxel->HasNorthWall(position);
	}

	bool Generator::HasSouthWall(const FIntVector& position) const noexcept
	{
		return mVoxel->HasSouthWall(position);
	}

	bool Generator::HasEastWall(const FIntVector& position) const noexcept
	{
		return mVoxel->HasEastWall(position);
	}

	bool Generator::HasWestWall(const FIntVector& position) const noexcept
	{
		return mVoxel->HasWestWall(position);
	}

	uint32_t Generator::CalculateCRC32(const uint32_t hash) const noexcept
	{
		return mVoxel ? mVoxel->CalculateCRC32(hash) : hash;
	}

	const Grid& Generator::GetGrid(const FIntVector& location) const noexcept
	{
		return mVoxel->Get(location.X, location.Y, location.Z);
	}

	const Grid& Generator::GetGrid(const int32 x, const int32 y, const int32 z) const noexcept
	{
		return mVoxel->Get(x, y, z);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	// 以下はデバッグに関する関数です。
	////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITOR
	/*
	markdown + mermaidによるフローチャートを出力します
	*/
	void Generator::DumpRoomDiagram(const std::string& path) const noexcept
	{
		std::ofstream stream(path);
		if (stream.is_open())
		{
			stream << "```mermaid" << std::endl;
			stream << "graph TD;" << std::endl;

			for (const auto& room : mRooms)
			{
				stream << room->GetName() << "(" << std::endl;
				stream << room->GetPartsName() << std::endl;
				stream << "Identifier:" << std::to_string(room->GetIdentifier()) << std::endl;
				stream << "Branch:" << std::to_string(room->GetBranchId()) << std::endl;
				stream << "Depth:" << std::to_string(room->GetDepthFromStart()) << std::endl;
				if (Room::Item::Empty != room->GetItem())
					stream << "Item:" << room->GetItemName() << std::endl;
				if (room->IsValidReservationNumber())
					stream << "Sublevel:" << room->GetReservationNumber() << std::endl;
				stream << ")" << std::endl;
			}

			if (mStartRoom)
			{
				std::unordered_set<const Aisle*> edges;
				DumpRoomDiagram(stream, edges, mStartRoom);
			}

			stream << "```" << std::endl;
		}
	}

	void Generator::DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (passableAisles.contains(&aisle) == true)
					continue;
				passableAisles.emplace(&aisle);

				std::string label;
				label = "Identifier:" + std::to_string(aisle.GetIdentifier());
				if (aisle.IsUniqueLocked())
					label += "\nUnique lock";
				else if (aisle.IsLocked())
					label += "\nLock";

				if (room == room0)
					stream << room0->GetName() << "<-->|" + label + "|" << room1->GetName();
				else
					stream << room1->GetName() << "<-->|" + label + "|" << room0->GetName();
				stream << std::endl;

				if (room != room0)
					DumpRoomDiagram(stream, passableAisles, room0);
				if (room != room1)
					DumpRoomDiagram(stream, passableAisles, room1);
			}
		}
	}

	void Generator::GenerateRoomImageForDebug(const std::string& filename) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		int32_t minY, minZ;
		int32_t maxY, maxZ;
		int32_t minX = minY = minZ = 0;
		int32_t maxX = maxY = maxZ = std::numeric_limits<int32_t>::lowest();

		// 空間の必要な大きさを求める
		for (const auto& room : mRooms)
		{
			minX = std::min(minX, room->GetLeft());
			minY = std::min(minY, room->GetTop());
			minZ = std::min(minZ, room->GetBackground());
			maxX = std::max(maxX, room->GetRight());
			maxY = std::max(maxY, room->GetBottom());
			maxZ = std::max(maxZ, room->GetForeground());
		}

		const int32_t width = maxX - minX + 2;
		const int32_t depth = maxY - minY + 2;
		const int32_t height = maxZ - minZ + 2;
		const int32_t offsetX = -minX + 1;
		const int32_t offsetY = -minY + 1;
		const int32_t offsetZ = depth + 1;

		// 空間のサイズを設定
		const bmp::Canvas canvas(Scale(width), Scale(depth + 1 + height));

		for (const auto& room : mRooms)
		{
			bmp::RGBCOLOR color;
			if (room->GetParts() == Room::Parts::Start)
			{
				color = StartColor;
			}
			else if (room->GetParts() == Room::Parts::Goal)
			{
				color = GoalColor;
			}
			else if (room->GetParts() == Room::Parts::Hanare)
			{
				color = LeafColor;
			}
			else
			{
				float ratio = static_cast<float>(room->GetZ() - minZ) / static_cast<float>(maxZ - minZ);
				if (ratio <= std::numeric_limits<float>::epsilon())
					ratio = std::numeric_limits<float>::epsilon();
				color.rgbRed = BaseDarkColor.rgbRed + (BaseLightColor.rgbRed - BaseDarkColor.rgbRed) * ratio;
				color.rgbGreen = BaseDarkColor.rgbGreen + (BaseLightColor.rgbGreen - BaseDarkColor.rgbGreen) * ratio;
				color.rgbBlue = BaseDarkColor.rgbBlue + (BaseLightColor.rgbBlue - BaseDarkColor.rgbBlue) * ratio;
			}

			// XY平面を描画
			canvas.Rectangle(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetY + room->GetTop()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetY + room->GetBottom()),
				color
			);

			// XZ平面を描画
			canvas.Rectangle(
				Scale(offsetX + room->GetLeft()),
				Scale(offsetZ + height - room->GetForeground()),
				Scale(offsetX + room->GetRight()),
				Scale(offsetZ + height - room->GetBackground()),
				color
			);
		}

		// グリッドを描画
		{
			for (int32_t x = minX; x <= maxX; ++x)
			{
				canvas.VerticalLine(
					Scale(offsetX + x),
					Scale(offsetY + minY),
					Scale(offsetY + maxY),
					x == 0 ? OriginYColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);

				canvas.VerticalLine(
					Scale(offsetX + x),
					Scale(offsetZ + height - minZ),
					Scale(offsetZ + height - maxZ),
					x == 0 ? OriginZColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (int32_t y = minY; y <= maxY; ++y)
			{
				canvas.HorizontalLine(
					Scale(offsetX + minX),
					Scale(offsetX + maxX),
					Scale(offsetY + y),
					y == 0 ? OriginXColor : y % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (int32_t z = minZ; z <= maxZ; ++z)
			{
				canvas.HorizontalLine(
					Scale(offsetX + minX),
					Scale(offsetX + maxX),
					Scale(offsetZ + height - z),
					z == 0 ? OriginXColor : z % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}
		}

		canvas.Write(dungeon::GetDebugDirectoryString() + filename);
#endif
	}

	const TCHAR* Generator::GetAisleVoxelFailureName(const AisleVoxelFailure failure) noexcept
	{
		switch (failure)
		{
		case AisleVoxelFailure::StartGateNotFound:
			return TEXT("StartGateNotFound");
		case AisleVoxelFailure::GoalGateNotFound:
			return TEXT("GoalGateNotFound");
		case AisleVoxelFailure::RouteNotFound:
			return TEXT("RouteNotFound");
		default:
			return TEXT("Unknown");
		}
	}

	void Generator::ReportAisleVoxelFailure(const size_t aisleIndex, const Aisle& aisle, const AisleVoxelFailure failure, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint) const noexcept
	{
		const FIntVector start = ToIntVector(*startPoint);
		const FIntVector goal = ToIntVector(*goalPoint);

		switch (failure)
		{
		case AisleVoxelFailure::StartGateNotFound:
			DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a start gate that can be generated. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), aisleIndex, static_cast<uint16_t>(aisle.GetIdentifier()), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
			break;
		case AisleVoxelFailure::GoalGateNotFound:
			DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a goal gate that can be generated. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), aisleIndex, static_cast<uint16_t>(aisle.GetIdentifier()), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
			break;
		case AisleVoxelFailure::RouteNotFound:
		default:
			DUNGEON_GENERATOR_ERROR(TEXT("Generator: Route search failed. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), aisleIndex, static_cast<uint16_t>(aisle.GetIdentifier()), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
			break;
		}

		/*
		 * 失敗した側の部屋の状態をダンプします
		 * 経路の検索に失敗した場合は開始部屋とゴール部屋の両方をダンプします
		 */
		if (failure != AisleVoxelFailure::GoalGateNotFound)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("State of the grid in the starting room %d: ID=%d (%d,%d,%d) %d Gate"), aisleIndex
				, static_cast<uint16_t>(startPoint->GetOwnerRoom()->GetIdentifier())
				, startPoint->GetOwnerRoom()->GetX(), startPoint->GetOwnerRoom()->GetY(), startPoint->GetOwnerRoom()->GetZ()
				, startPoint->GetOwnerRoom()->GetGateCount());
			DumpVoxel(startPoint);
		}
		if (failure != AisleVoxelFailure::StartGateNotFound)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("State of the grid in the goal room %d: ID=%d (%d,%d,%d) %d Gate"), aisleIndex
				, static_cast<uint16_t>(goalPoint->GetOwnerRoom()->GetIdentifier())
				, goalPoint->GetOwnerRoom()->GetX(), goalPoint->GetOwnerRoom()->GetY(), goalPoint->GetOwnerRoom()->GetZ()
				, goalPoint->GetOwnerRoom()->GetGateCount());
			DumpVoxel(goalPoint);
		}

		DumpAisleAndRoomInformation(aisleIndex);
	}

	void Generator::DumpAisleAndRoomInformation(const size_t index) const noexcept
	{
		for (size_t j = 0; j < index; ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_VERBOSE(TEXT("OK .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		{
			const Aisle& a = mAisles[index];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_VERBOSE(TEXT("NG .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), index, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		for (size_t j = index + 1; j < mAisles.size(); ++j)
		{
			const Aisle& a = mAisles[j];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_VERBOSE(TEXT("-- .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}

		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d) center(%f, %f, %f)")
				, static_cast<uint16_t>(room->GetIdentifier())
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
				, room->GetCenter().X, room->GetCenter().Y, room->GetCenter().Z
			);
		}
	}


	void Generator::DumpVoxel(const std::shared_ptr<const Point>& point) const noexcept
	{
		if (point)
		{
			DumpVoxel(point->GetOwnerRoom());
		}
	}

	void Generator::DumpVoxel(const std::shared_ptr<Room>& room) const noexcept
	{
#if JENKINS_FOR_DEVELOP
		for (int32 y = room->GetY() - 2; y < room->GetY() + room->GetDepth() + 2; ++y)
		{
			FString text;
			for (int32 x = room->GetX() - 2; x < room->GetX() + room->GetWidth() + 2; ++x)
			{
				const Grid& grid = mVoxel->Get(x, y, room->GetZ());
				FString typeName;
				if (room->GetY() <= y && y < room->GetY() + room->GetDepth() &&
					room->GetX() <= x && x < room->GetX() + room->GetWidth())
					typeName = TEXT("*");
				typeName += grid.GetTypeName();
				text += FString::Printf(TEXT("%10s (%5d),"), *typeName, static_cast<uint16_t>(grid.GetIdentifier()));
			}
			DUNGEON_GENERATOR_VERBOSE(TEXT("%s"), *text);
		}
#endif
	}
#endif
}
