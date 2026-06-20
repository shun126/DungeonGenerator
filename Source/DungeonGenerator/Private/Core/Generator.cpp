/**
 * ダンジョン生成ソースファイル
 *
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Generator.h"
#include "GenerateParameter.h"
#include "Debug/Config.h"
#include "Debug/Debug.h"
#include "Helper/Finalizer.h"
#include "Helper/Stopwatch.h"
#include "Math/Math.h"
#include "Math/Vector.h"
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
	 * Returns the integer Y location whose room center is closest to zero.
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
		}
	}

	/*
	 * Returns the squared grid distance between a room's current location and a candidate location.
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
	 * Returns true when the right candidate should replace the current best candidate.
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
		if (next.LayoutSpreadCost < current.LayoutSpreadCost - epsilon)
		{
			return true;
		}
		if (next.LayoutSpreadCost > current.LayoutSpreadCost + epsilon)
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
	 * Creates a separation candidate by placing the movable room just outside the fixed room on one axis.
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
	 * Returns the effective room margin used between two rooms.
	 * 2つの部屋の間で使用する実効余白を返します。
	 */
	FRoomSpacingMargins GetEffectiveRoomSpacingMargins(const dungeon::GenerateParameter& parameter, const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		FRoomSpacingMargins margins;
		margins.Horizontal = static_cast<uint8>(parameter.GetHorizontalRoomMargin());
		if (margins.Horizontal < room0.GetHorizontalRoomMargin())
			margins.Horizontal = room0.GetHorizontalRoomMargin();
		if (margins.Horizontal < room1.GetHorizontalRoomMargin())
			margins.Horizontal = room1.GetHorizontalRoomMargin();

		margins.Vertical = parameter.GetExpansionPolicy() == dungeon::ExpansionPolicy::Flat ? 0 : static_cast<uint8>(parameter.GetVerticalRoomMargin());
		if (parameter.GetExpansionPolicy() != dungeon::ExpansionPolicy::Flat)
		{
			if (margins.Vertical < room0.GetVerticalRoomMargin())
				margins.Vertical = room0.GetVerticalRoomMargin();
			if (margins.Vertical < room1.GetVerticalRoomMargin())
				margins.Vertical = room1.GetVerticalRoomMargin();
		}
		return margins;
	}

	/*
	 * Returns the effective room margin used by room separation.
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
	 * Returns true when two one-dimensional room intervals overlap.
	 * 1次元の部屋範囲が重なっている場合にtrueを返します。
	 */
	bool RoomIntervalsOverlap(const int32_t min0, const int32_t max0, const int32_t min1, const int32_t max1) noexcept
	{
		return max0 > min1 && min0 < max1;
	}

	/*
	 * Returns the overlap length between one-dimensional intervals.
	 * 1次元範囲同士の重なり長さを返します。
	 */
	uint64 GetIntervalOverlapDepth(const int32_t min0, const int32_t max0, const int32_t min1, const int32_t max1) noexcept
	{
		const auto overlap = std::min(max0, max1) - std::max(min0, min1);
		return overlap > 0 ? static_cast<uint64>(overlap) : 0;
	}

	/*
	 * Returns the empty gap between two one-dimensional room ranges.
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
	 * Returns a purpose weight used when minimizing aisle length.
	 * 通路距離を最小化するときに使う通路目的ごとの重みを返します。
	 */
	double GetAisleDistanceWeight(const dungeon::Aisle& aisle) noexcept
	{
		if (aisle.IsMain())
		{
			return 1.25;
		}

		switch (aisle.GetPurpose())
		{
		case EDungeonAislePurpose::Locked:
		case EDungeonAislePurpose::VerticalTransition:
			return 1.10;
		case EDungeonAislePurpose::Branch:
			return 0.85;
		case EDungeonAislePurpose::Loop:
		case EDungeonAislePurpose::Shortcut:
			return 0.65;
		case EDungeonAislePurpose::MainPath:
		default:
			return 1.00;
		}
	}

	/*
	 * Calculates aisle distance as the sum of room shell gaps on each axis.
	 * 各軸の部屋外周間ギャップ合計として通路距離を計算します。
	 */
	int32_t CalculateRoomPairAisleDistance(const dungeon::Room& room0, const dungeon::Room& room1) noexcept
	{
		return
			CalculateIntervalGap(room0.GetLeft(), room0.GetRight(), room1.GetLeft(), room1.GetRight()) +
			CalculateIntervalGap(room0.GetTop(), room0.GetBottom(), room1.GetTop(), room1.GetBottom()) +
			CalculateIntervalGap(room0.GetBackground(), room0.GetForeground(), room1.GetBackground(), room1.GetForeground());
	}

	/*
	 * Returns true when the room must not be moved by layout optimization.
	 * レイアウト最適化で部屋を動かしてはいけない場合にtrueを返します。
	 */
	bool IsFixedRoomForLayoutOptimization(const dungeon::Room& room, const dungeon::GenerateParameter& parameter) noexcept
	{
		return room.GetParts() == dungeon::Room::Parts::Start && parameter.IsGenerateStartRoomReserved();
	}

	/*
	 * Returns a soft move penalty for rooms that may move but should avoid needless drift.
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
	 * Returns the room connected to movingRoom through an aisle.
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
	 * Calculates the weighted distance cost for all aisles in a layout.
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
	 * Calculates connected aisle distance cost for one room at a candidate location.
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
	 * Calculates the maximum connected aisle distance for one room at a candidate location.
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
	 * Returns the overlap volume between a margin-expanded candidate room and another room.
	 * 余白で拡張した候補部屋と別の部屋の重なり体積を返します。
	 */
	uint64 GetRoomCollisionDepth(const dungeon::Room& candidateRoom, const dungeon::Room& otherRoom, const FRoomSpacingMargins margins) noexcept
	{
		const auto overlapX = GetIntervalOverlapDepth(candidateRoom.GetLeft() - margins.Horizontal, candidateRoom.GetRight() + margins.Horizontal, otherRoom.GetLeft(), otherRoom.GetRight());
		const auto overlapY = GetIntervalOverlapDepth(candidateRoom.GetTop() - margins.Horizontal, candidateRoom.GetBottom() + margins.Horizontal, otherRoom.GetTop(), otherRoom.GetBottom());
		const auto overlapZ = GetIntervalOverlapDepth(candidateRoom.GetBackground() - margins.Vertical, candidateRoom.GetForeground() + margins.Vertical, otherRoom.GetBackground(), otherRoom.GetForeground());
		return overlapX * overlapY * overlapZ;
	}

	/*
	 * Scores candidate collisions against rooms other than the fixed and moving room.
	 * 固定部屋と移動部屋以外に対する候補位置の交差を採点します。
	 */
	void ScoreSeparationCandidateCollisions(
		FRoomSeparationCandidate& candidate,
		const dungeon::GenerateParameter& parameter,
		const std::list<std::shared_ptr<dungeon::Room>>& rooms,
		const std::shared_ptr<dungeon::Room>& fixedRoom,
		const std::shared_ptr<dungeon::Room>& movableRoom) noexcept
	{
		dungeon::Room candidateRoom(*movableRoom);
		candidateRoom.SetX(candidate.Location.X);
		candidateRoom.SetY(candidate.Location.Y);
		candidateRoom.SetZ(candidate.Location.Z);

		candidate.CollisionCount = 0;
		candidate.CollisionDepth = 0;
		for (const auto& otherRoom : rooms)
		{
			if (otherRoom == nullptr || otherRoom == fixedRoom || otherRoom == movableRoom)
				continue;

			const auto margins = GetEffectiveRoomSeparationMargins(parameter, candidateRoom, *otherRoom);
			if (candidateRoom.Intersect(*otherRoom, margins.Horizontal, margins.Vertical))
			{
				++candidate.CollisionCount;
				candidate.CollisionDepth += GetRoomCollisionDepth(candidateRoom, *otherRoom, margins);
			}
		}
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
	 * Returns true when the rooms overlap on the axes not being compacted.
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
	 * Returns the current gap between two rooms on a single axis.
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
	 * Returns the target location that places a moving room at the requested margin from an anchor room.
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
	 * Returns true when the first room should be tried as the moving room before the second one.
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
		mGenerateParameter = parameter;

		// 生成
		// TODO:リトライする仕組みの検討をして下さい。部屋の間隔を広げると成功する可能性が上がるかもしれません。
		size_t retryCount = 1;
		while (!GenerateImpl())
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
			check(mLastError == Error::Success);
#endif

			if (--retryCount == 0)
				break;

			mGenerateParameter.SetHorizontalRoomMargin(mGenerateParameter.GetHorizontalRoomMargin() + 2);
			mLastError = Error::Success;

			Reset();
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

		// スタート部屋とゴール部屋のサブレベルを調整します
		if (AdjustedStartAndGoalSubLevel(3) == false)
			return false;

		// 部屋のサイズを調整します
		AdjustRoomSize(4);

		// 改めて部屋の重なりを解消します
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

		// 部屋が全て収まるように空間を拡張します
		if (ExpandSpace(7) == false)
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
			Stopwatch stopwatch;
#endif
			MissionGraph missionGraph(shared_from_this(), mStartRoom, mGoalRoom, maxKeyCount);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			DUNGEON_GENERATOR_LOG(TEXT("MissionGraph: %lf seconds"), stopwatch.Lap());
#endif

#if defined(DEBUG_GENERATE_MISSION_GRAPH_FILE)
			// デバッグ情報を出力
			DumpRoomDiagram(dungeon::GetDebugDirectoryString() + "/debug/DungeonStructureDiagram.md");
#endif

			// クリアできるミッションかテストします
			const MissionGraphTester missionGraphTester(mRooms, mAisles);
			if (missionGraphTester.Success() == false)
			{
				DUNGEON_GENERATOR_ERROR(TEXT("MissionGraph validation failed. The generated key-lock route is not solvable."));
				mLastError = Error::MissionGraphValidationFailed;
				return false;
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
		if (GenerateVoxel(8) == false)
			return false;

		return true;
	}

	/**
	 * 部屋の初期位置を決定します
	 */
	std::vector<LayoutCandidate> Generator::BuildIntentLayoutCandidates() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("BuildIntentLayoutCandidates: %lf seconds"), stopwatch.Lap());
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("SelectDistanceAwareLayout: %lf seconds"), stopwatch.Lap());
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
	 * 部屋の重なりを解消します
	 */
	Generator::ResolveLayoutCollisionsResult Generator::ResolveLayoutCollisions(const size_t phase, const size_t subPhase) noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("ResolveLayoutCollisions: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Separate Rooms"));
#endif

		// 部屋の交差を解消します
		const auto expansionPolicy = mGenerateParameter.GetExpansionPolicy();
		if (expansionPolicy == ExpansionPolicy::Flat || expansionPolicy == ExpansionPolicy::ExpandVertically)
		{
			for (const std::shared_ptr<Room>& room : mRooms)
			{
				FIntVector location(room->GetX(), room->GetY(), room->GetZ());
				ApplySeparationConstraints(location, *room, mGenerateParameter);
				room->SetX(location.X);
				room->SetY(location.Y);
				room->SetZ(location.Z);
			}
		}

		uint8_t imageNo = 0;
		constexpr uint8_t maxImageNo = 20;
		ResolveLayoutCollisionsResult resolveLayoutCollisionsResult = ResolveLayoutCollisionsResult::Completed;
		bool retry;
		do {
			retry = false;

			// 中心から近い順に並べ替える
			mRooms.sort([](const std::shared_ptr<const Room>& l, const std::shared_ptr<const Room>& r)
				{
					const double lsd = l->GetCenter().SizeSquared();
					const double rsd = r->GetCenter().SizeSquared();
					return lsd < rsd;
				}
			);

			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				std::vector<std::shared_ptr<Room>> intersectedRooms;

				// 他の部屋と交差している？
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room0->GetHorizontalRoomMargin())
						horizontalRoomMargin = room0->GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room1->GetHorizontalRoomMargin())
						horizontalRoomMargin = room1->GetHorizontalRoomMargin();

					uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
					if (verticalRoomMargin < room0->GetVerticalRoomMargin())
						verticalRoomMargin = room0->GetVerticalRoomMargin();
					if (verticalRoomMargin < room1->GetVerticalRoomMargin())
						verticalRoomMargin = room1->GetVerticalRoomMargin();

					if (room0 != room1 && room0->Intersect(*room1, horizontalRoomMargin, verticalRoomMargin))
					{
						// 交差した部屋を記録
						// cppcheck-suppress [useStlAlgorithm]
						intersectedRooms.emplace_back(room1);
						// 動いた先で交差している可能性があるので再チェック
						retry = true;
						// 一度でも部屋を動かしてしまったので結果を記録
						resolveLayoutCollisionsResult = ResolveLayoutCollisionsResult::Moved;
					}
				}

				if (intersectedRooms.empty() == false)
				{
					// 一番原点に近い部屋を探す
					intersectedRooms.emplace_back(room0);
					std::stable_sort(intersectedRooms.begin(), intersectedRooms.end(), [](const std::shared_ptr<Room>& l, const std::shared_ptr<Room>& r)
						{
							return l->GetCenter().SizeSquared2D() < r->GetCenter().SizeSquared2D();
						});

					auto nearestRoomToOrigin = intersectedRooms[0];
					intersectedRooms.erase(intersectedRooms.begin());

					// 交差した部屋が重ならないように移動
					ResolveRoomCollisionGroup(nearestRoomToOrigin, intersectedRooms, imageNo > 10);
				}
			}

#if defined(DEBUG_GENERATE_BITMAP_FILE)
			if (retry)
			{
				GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_ResolveLayoutCollisions_" + std::to_string(imageNo) + ".bmp");
			}
#endif

			++imageNo;
		} while (imageNo < maxImageNo && retry);

		// 部屋の重複が解決できなかった場合
		if (imageNo >= maxImageNo && retry)
		{
			for (const std::shared_ptr<Room>& room0 : mRooms)
			{
				for (const std::shared_ptr<Room>& room1 : mRooms)
				{
					uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room0->GetHorizontalRoomMargin())
						horizontalRoomMargin = room0->GetHorizontalRoomMargin();
					if (horizontalRoomMargin < room1->GetHorizontalRoomMargin())
						horizontalRoomMargin = room1->GetHorizontalRoomMargin();

					uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
					if (verticalRoomMargin < room0->GetVerticalRoomMargin())
						verticalRoomMargin = room0->GetVerticalRoomMargin();
					if (verticalRoomMargin < room1->GetVerticalRoomMargin())
						verticalRoomMargin = room1->GetVerticalRoomMargin();

					if (room0 != room1 && room0->Intersect(*room1, horizontalRoomMargin, verticalRoomMargin))
					{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
						GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_" + std::to_string(subPhase) + "_SeparateRooms_failure.bmp");
#endif
						DUNGEON_GENERATOR_ERROR(TEXT("Generator::ResolveLayoutCollisions: The room crossing was not resolved."));
						mLastError = Error::SeparateRoomsFailed;
						return ResolveLayoutCollisionsResult::Failed;
					}
				}
			}
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		for (const std::shared_ptr<Room>& room : mRooms)
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
			DUNGEON_GENERATOR_LOG(TEXT("ResolveLayoutCollisions: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d)")
				, static_cast<uint16_t>(room->GetIdentifier())
				, room->GetX(), room->GetY(), room->GetZ()
				, room->GetWidth(), room->GetDepth(), room->GetHeight()
			);
		}
#endif

		return resolveLayoutCollisionsResult;
	}

	void Generator::ResolveRoomCollisionGroup(const std::shared_ptr<Room>& fixedRoom, const std::vector<std::shared_ptr<Room>>& intersectedRooms, const bool activateOuterMovement) const noexcept
	{
		// 交差した部屋が重ならないように移動
		for (const std::shared_ptr<Room>& movableRoom : intersectedRooms)
		{
			if (IsFixedRoomForLayoutOptimization(*movableRoom, mGenerateParameter))
				continue;

			check(fixedRoom->GetCenter().SizeSquared2D() <= movableRoom->GetCenter().SizeSquared2D());

			uint8 horizontalRoomMargin = mGenerateParameter.GetHorizontalRoomMargin();
			if (horizontalRoomMargin < fixedRoom->GetHorizontalRoomMargin())
				horizontalRoomMargin = fixedRoom->GetHorizontalRoomMargin();
			if (horizontalRoomMargin < movableRoom->GetHorizontalRoomMargin())
				horizontalRoomMargin = movableRoom->GetHorizontalRoomMargin();

			uint8 verticalRoomMargin = mGenerateParameter.GetVerticalRoomMargin();
			if (verticalRoomMargin < fixedRoom->GetVerticalRoomMargin())
				verticalRoomMargin = fixedRoom->GetVerticalRoomMargin();
			if (verticalRoomMargin < movableRoom->GetVerticalRoomMargin())
				verticalRoomMargin = movableRoom->GetVerticalRoomMargin();

			// 二つの部屋を合わせた空間の大きさ
			const auto expansionPolicy = mGenerateParameter.GetExpansionPolicy();
			const auto direction = MakeSeparationTieBreakDirection(*fixedRoom, *movableRoom, activateOuterMovement);
			auto hasBestCandidate = false;
			FRoomSeparationCandidate bestCandidate;
			const auto addCandidate = [&](const ERoomSeparationAxis axis)
				{
					for (const bool positiveSide : { true, false })
					{
						auto candidate = MakeSeparationCandidate(
							*fixedRoom,
							*movableRoom,
							mGenerateParameter,
							axis,
							positiveSide,
							horizontalRoomMargin,
							verticalRoomMargin,
							direction,
							activateOuterMovement,
							mGenerateParameter.GetRandom()->Get<double>()
						);
						ScoreSeparationCandidateCollisions(candidate, mGenerateParameter, mRooms, fixedRoom, movableRoom);
						ScoreSeparationCandidateAisleDistance(candidate, mGenerateParameter, mAisles, movableRoom);
						if (hasBestCandidate == false || IsBetterSeparationCandidate(bestCandidate, candidate))
						{
							bestCandidate = candidate;
							hasBestCandidate = true;
						}
					}
				};

			switch (expansionPolicy)
			{
			case ExpansionPolicy::Flat:
				addCandidate(ERoomSeparationAxis::X);
				addCandidate(ERoomSeparationAxis::Y);
				break;
			case ExpansionPolicy::ExpandVertically:
				addCandidate(ERoomSeparationAxis::X);
				addCandidate(ERoomSeparationAxis::Z);
				break;
			case ExpansionPolicy::ExpandAnyDirection:
			default:
				addCandidate(ERoomSeparationAxis::X);
				addCandidate(ERoomSeparationAxis::Y);
				addCandidate(ERoomSeparationAxis::Z);
				break;
			}
			check(hasBestCandidate);

			movableRoom->SetX(bestCandidate.Location.X);
			movableRoom->SetY(bestCandidate.Location.Y);
			movableRoom->SetZ(bestCandidate.Location.Z);

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			// 交差していないか再確認
			if (fixedRoom->Intersect(*movableRoom, horizontalRoomMargin, verticalRoomMargin))
			{
				DUNGEON_GENERATOR_LOG(TEXT("direction %f,%f,%f"), direction.X, direction.Y, direction.Z);
				DUNGEON_GENERATOR_LOG(TEXT("Room0: X=%d~%d,Y=%d~%d,Z=%d~%d"), fixedRoom->GetLeft(), fixedRoom->GetRight(), fixedRoom->GetTop(), fixedRoom->GetBottom(), fixedRoom->GetBackground(), fixedRoom->GetForeground());
				DUNGEON_GENERATOR_LOG(TEXT("Room1: X=%d~%d,Y=%d~%d,Z=%d~%d"), movableRoom->GetLeft(), movableRoom->GetRight(), movableRoom->GetTop(), movableRoom->GetBottom(), movableRoom->GetBackground(), movableRoom->GetForeground());
				check(false);
				check(fixedRoom->Intersect(*movableRoom, horizontalRoomMargin, verticalRoomMargin) == true);
			}
#endif
		}
	}

	/*
	 * Locally minimizes aisle distance after collision resolution without breaking room margins.
	 * 衝突解消後に、部屋の余白を破らない範囲で通路距離を局所的に最小化します。
	 */
	bool Generator::OptimizeAisleDistance(size_t phase) const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("OptimizeAisleDistance: %lf seconds"), stopwatch.Lap());
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
			axes = { ERoomSeparationAxis::X, ERoomSeparationAxis::Z };
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

					const auto margins = GetEffectiveRoomSpacingMargins(mGenerateParameter, candidateRoom, *otherRoom);
					if (candidateRoom.Intersect(*otherRoom, margins.Horizontal, margins.Vertical))
						return false;
				}
				return true;
			};

		const auto tryMoveRoom = [this, &validateCandidate](const std::shared_ptr<Room>& anchorRoom, const std::shared_ptr<Room>& movingRoom, const ERoomSeparationAxis axis, const int32_t margin) -> bool
			{
				if (IsFixedRoomForLayoutOptimization(*movingRoom, mGenerateParameter))
					return false;

				FIntVector location;
				if (MakeRoomCompactionLocation(*anchorRoom, *movingRoom, mGenerateParameter, axis, margin, location) == false)
					return false;

				if (validateCandidate(movingRoom, location) == false)
					return false;

				const double currentCost = CalculateLayoutAisleDistanceCost(mAisles);
				const FIntVector currentLocation(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
				const double movePenalty = std::sqrt(GetLocationDistanceSquared(*movingRoom, location)) * GetRoomMovePenalty(*movingRoom) * 0.05;

				movingRoom->SetX(location.X);
				movingRoom->SetY(location.Y);
				movingRoom->SetZ(location.Z);
				const double nextCost = CalculateLayoutAisleDistanceCost(mAisles) + movePenalty;
				if (nextCost < currentCost)
				{
					return true;
				}

				movingRoom->SetX(currentLocation.X);
				movingRoom->SetY(currentLocation.Y);
				movingRoom->SetZ(currentLocation.Z);
				return false;
			};

		const auto tryStepRoomTowardAnchor = [this, &validateCandidate](const std::shared_ptr<Room>& anchorRoom, const std::shared_ptr<Room>& movingRoom, const ERoomSeparationAxis axis) -> bool
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
					if (movingRoom->GetCenter().Z < anchorRoom->GetCenter().Z)
						++location.Z;
					else if (movingRoom->GetCenter().Z > anchorRoom->GetCenter().Z)
						--location.Z;
					else
						return false;
					break;
				default:
					checkNoEntry();
					return false;
				}
				ApplySeparationConstraints(location, *movingRoom, mGenerateParameter);

				if (validateCandidate(movingRoom, location) == false)
					return false;

				const double currentCost = CalculateLayoutAisleDistanceCost(mAisles);
				const FIntVector currentLocation(movingRoom->GetX(), movingRoom->GetY(), movingRoom->GetZ());
				movingRoom->SetX(location.X);
				movingRoom->SetY(location.Y);
				movingRoom->SetZ(location.Z);
				if (CalculateLayoutAisleDistanceCost(mAisles) < currentCost)
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

				const auto margins = GetEffectiveRoomSpacingMargins(mGenerateParameter, *room0, *room1);
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: %lf seconds"), stopwatch.Lap());
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
		mGenerateParameter.SetWidth(maxX - minX + 1);
		mGenerateParameter.SetDepth(maxY - minY + 1);
		mGenerateParameter.SetHeight(maxZ - minZ + 1);

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
			DUNGEON_GENERATOR_LOG(TEXT("ExpandSpace: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeightAndDepthFromStart: %lf seconds"), stopwatch.Lap());
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
			DUNGEON_GENERATOR_LOG(TEXT("DetectFloorHeightAndDepthFromStart: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: %lf seconds"), stopwatch.Lap());
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
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("ExtractionAisles: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
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
				aisle.GetIdentifier().Get(),
				aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get(),
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
			DUNGEON_GENERATOR_LOG(TEXT("GenerateAisle: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

		return true;
	}

	/**
	 * 部屋のパーツ（役割）を設定する
	 */
	void Generator::SetRoomParts() noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("SetRoomParts: %lf seconds"), stopwatch.Lap());
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
	 * スタート部屋およびゴール部屋のサブレベルが指定されていた場合に
	 * サブレベルが入る空間の範囲を空ける
	 */
	bool Generator::AdjustedStartAndGoalSubLevel(const size_t phase) const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSubLevel: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		// 必ず生成しなければならないサブレベル
		std::list<std::pair<uint32_t, FIntVector>> alwaysLoadedSubLevels;
		if (mOnQueryParts)
		{
			mOnQueryParts(alwaysLoadedSubLevels);
		}

		for (const std::shared_ptr<Room>& room : mRooms)
		{
			switch (room->GetParts())
			{
			case Room::Parts::Start:
				// スタート部屋の大きさを調整する
				if (mGenerateParameter.IsGenerateStartRoomReserved())
				{
					room->SetWidth(mGenerateParameter.GetStartRoomSize().X);
					room->SetDepth(mGenerateParameter.GetStartRoomSize().Y);
					room->SetHeight(mGenerateParameter.GetStartRoomSize().Z);
				}
				break;

			case Room::Parts::Goal:
				// ゴール部屋の大きさを調整する
				if (mGenerateParameter.IsGenerateGoalRoomReserved())
				{
					room->SetWidth(mGenerateParameter.GetGoalRoomSize().X);
					room->SetDepth(mGenerateParameter.GetGoalRoomSize().Y);
					room->SetHeight(mGenerateParameter.GetGoalRoomSize().Z);
				}
				break;

			case Room::Parts::Hall:
			case Room::Parts::Hanare:
				// サブレベルを部屋に関連付ける
				if (alwaysLoadedSubLevels.empty() == false)
				{
					auto i = std::find_if(alwaysLoadedSubLevels.begin(), alwaysLoadedSubLevels.end(), [room](const std::pair<uint32_t, FIntVector>& sublevel)
					{
						return
							room->GetWidth() == sublevel.second.X &&
							room->GetDepth() == sublevel.second.Y &&
							room->GetHeight() == sublevel.second.Z;
					});
					if (i != alwaysLoadedSubLevels.end())
					{
						room->SetReservationNumber(i->first);
						alwaysLoadedSubLevels.erase(i);
					}
					else
					{
						const std::pair<uint32_t, FIntVector>& alwaysLoadedSubLevel = alwaysLoadedSubLevels.front();
						room->SetReservationNumber(alwaysLoadedSubLevel.first);
						room->SetWidth(alwaysLoadedSubLevel.second.X);
						room->SetDepth(alwaysLoadedSubLevel.second.Y);
						room->SetHeight(alwaysLoadedSubLevel.second.Z);
						alwaysLoadedSubLevels.pop_front();
					}
				}
				break;

			case Room::Parts::Unidentified:
				break;
			}
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			DUNGEON_GENERATOR_LOG(TEXT("AdjustedStartAndGoalSubLevel: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		GenerateRoomImageForDebug("/debug/" + std::to_string(phase) + "_AdjustedStartAndGoalSubLevel.bmp");
#endif

		return true;
	}

	/**
	 * 部屋の大きさを調整する
	 */
	void Generator::AdjustRoomSize(const size_t phase) const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("AdjustRoomSize: %lf seconds"), stopwatch.Lap());
			}
		);
#endif

		// 狭すぎる・大きすぎる部屋を調整する
		for (const std::shared_ptr<Room>& room : mRooms)
		{
			// 予約済みの部屋はサイズを変更できない
			if (room->IsValidReservationNumber() == true)
				continue;
			// スタート・ゴール部屋はサイズを変更できない
			if (room->GetParts() == Room::Parts::Start && mGenerateParameter.IsGenerateStartRoomReserved())
				continue;
			if (room->GetParts() == Room::Parts::Goal && mGenerateParameter.IsGenerateGoalRoomReserved())
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("Branch: %lf seconds"), stopwatch.Lap());
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
			DUNGEON_GENERATOR_LOG(TEXT("Branch: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x"), x, y, z, w);
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

	/*
	 * MissionGraph生成後に呼び出す必要があります
	 */
	void Generator::InvokeRoomCallbacks() const noexcept
	{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("InvokeRoomCallbacks: %lf seconds"), stopwatch.Lap());
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
		Stopwatch stopwatch;
		Finalizer finalizer([&stopwatch]()
			{
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: %lf seconds"), stopwatch.Lap());
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
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: room generated : RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

		if (mOnPreGenerateVoxel)
		{
			mOnPreGenerateVoxel(mVoxel);
		}

		// 通路の距離が短い順に並べ替える
		std::stable_sort(mAisles.begin(), mAisles.end(), [](const Aisle& l, const Aisle& r)
			{
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
				aisle.GetIdentifier().Get(),
				aisle.GetPoint(0)->GetOwnerRoom()->GetIdentifier().Get(),
				aisle.GetPoint(1)->GetOwnerRoom()->GetIdentifier().Get(),
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

		for (size_t i = 0; i < mAisles.size(); ++i)
		{
			const Aisle& aisle = mAisles[i];
			std::shared_ptr<const Point> startPoint = aisle.GetPoint(0);
			std::shared_ptr<const Point> goalPoint = aisle.GetPoint(1);

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
			if (startPointZ == goalPointZ)
				// 開始門と終了門が同じ高さにある？
				GenerateAisleVoxel(i, aisle, startPoint, goalPoint, depthRatioFromStart, false);
			else if (startPointZ < goalPointZ)
				// 開始門が終了門よりも低い高さにある？
				GenerateAisleVoxel(i, aisle, startPoint, goalPoint, depthRatioFromStart, mGenerateParameter.IsGenerateSlopeInRoom());
			else
				// 終了門が開始門よりも低い高さにある？
				GenerateAisleVoxel(i, aisle, goalPoint, startPoint, depthRatioFromStart, mGenerateParameter.IsGenerateSlopeInRoom());

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
			// 通信同期用に現在の乱数の種を出力する
			{
				uint32_t x, y, z, w;
				GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
				const uint32_t crc32 = CalculateCRC32();
				DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: aisle generated: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
			}
#endif

			if (aisle.GetHeight() > 1)
			{
				ExpandAisleHeightVoxel(aisle);
			}
		}


		if (mGenerateParameter.IsGenerateStructuralColumn())
		{
			for (const auto& room : mRooms)
			{
				GenerateStructuralColumnVoxel(room);
			}
		}

		if (mOnPostGenerateVoxel)
		{
			mOnPostGenerateVoxel(mVoxel);
		}

#if defined(DEBUG_ENABLE_INFORMATION_FOR_REPLICATION)
		// 通信同期用に現在の乱数の種を出力する
		{
			uint32_t x, y, z, w;
			GetGenerateParameter().GetRandom()->GetSeeds(x, y, z, w);
			const uint32_t crc32 = CalculateCRC32();
			DUNGEON_GENERATOR_LOG(TEXT("GenerateVoxel: finish: RandomSeed x=%08x, y=%08x, z=%08x, w=%08x, CRC32=%x"), x, y, z, w, crc32);
		}
#endif

#if defined(DEBUG_GENERATE_BITMAP_FILE)
		mVoxel->GenerateImageForDebug("/debug/" + std::to_string(phase) + "_GenerateVoxel.bmp");
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

				grid.SetNorthWall(grid.CanBuildWall(northGrid, Direction::North, mergeRooms));
				grid.SetSouthWall(grid.CanBuildWall(southGrid, Direction::South, mergeRooms));
				grid.SetEastWall(grid.CanBuildWall(eastGrid, Direction::East, mergeRooms));
				grid.SetWestWall(grid.CanBuildWall(westGrid, Direction::West, mergeRooms));
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

	bool Generator::GenerateAisleVoxel(const size_t aisleIndex, const Aisle& aisle, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint, const uint8_t depthRatioFromStart, const bool generateIndoorSlope) noexcept
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
		if (!mVoxel->SearchGateLocation(goalToStart, MaxResultCount, goal, goalPoint->GetOwnerRoom()->GetIdentifier(), start, mGenerateParameter.UseMissionGraph() == false))
			return false;

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
				aisleParameter.mGenerateIntersections = /*aisle.IsAnyLocked() == false ||*/ mGenerateParameter.IsAisleComplexity();
				aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
				aisleParameter.mLocked = aisle.IsLocked();
				aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
				if (mVoxel->Aisle(startToGoal, goalToStart, aisleParameter))
				{
					roomStructureGenerator.GenerateSlope(mVoxel);
					return true;
				}
			}
		}

		// スタート地点周辺でゲートを生成できるボクセルを探す
		std::vector<Voxel::CandidateLocation> startToGoal;
		if (mVoxel->SearchGateLocation(startToGoal, MaxResultCount, start, startPoint->GetOwnerRoom()->GetIdentifier(), goal, mGenerateParameter.UseMissionGraph() == false))
		{
			Voxel::AisleParameter aisleParameter;
			aisleParameter.mGoalCondition = pathGoalCondition;
			aisleParameter.mIdentifier = aisle.GetIdentifier();
			aisleParameter.mGenerateIntersections = /*aisle.IsAnyLocked() == false ||*/ mGenerateParameter.IsAisleComplexity();
			aisleParameter.mUniqueLocked = aisle.IsUniqueLocked();
			aisleParameter.mLocked = aisle.IsLocked();
			aisleParameter.mDepthRatioFromStart = depthRatioFromStart;
			bool complete = mVoxel->Aisle(startToGoal, goalToStart, aisleParameter);

			// 幹線通路以外なら生成に失敗しても到達可能なので成功扱いにする
			if (aisle.IsMain() == false)
			{
				complete = true;
			}
			// 幹線通路でも部屋を結合しているなら成功扱いにする
			// 生成失敗？
			if (complete == false)
			{
#if WITH_EDITOR
				DUNGEON_GENERATOR_ERROR(TEXT("Generator: Route search failed. %d: ID=%d (%d,%d,%d)-(%d,%d,%d)"), aisleIndex, static_cast<uint16_t>(aisle.GetIdentifier()), start.X, start.Y, start.Z, goal.X, goal.Y, goal.Z);
				DUNGEON_GENERATOR_ERROR(TEXT("State of the grid in the starting room %d: ID=%d (%d,%d,%d) %d Gate"), aisleIndex
					, static_cast<uint16_t>(startPoint->GetOwnerRoom()->GetIdentifier())
					, startPoint->GetOwnerRoom()->GetX(), startPoint->GetOwnerRoom()->GetY(), startPoint->GetOwnerRoom()->GetZ()
					, startPoint->GetOwnerRoom()->GetGateCount());
				DumpVoxel(startPoint);
				DUNGEON_GENERATOR_ERROR(TEXT("State of the grid in the goal room %d: ID=%d (%d,%d,%d) %d Gate"), aisleIndex
					, static_cast<uint16_t>(goalPoint->GetOwnerRoom()->GetIdentifier())
					, goalPoint->GetOwnerRoom()->GetX(), goalPoint->GetOwnerRoom()->GetY(), goalPoint->GetOwnerRoom()->GetZ()
					, goalPoint->GetOwnerRoom()->GetGateCount());
				DumpVoxel(goalPoint);
				DumpAisleAndRoomInformation(aisleIndex);
#endif
				mLastError = Error::RouteSearchFailed;
				return false;
			}

			return true;
		}
		else
		{
			// 部屋が結合されているなら通路が無くても問題ないはず…
#if WITH_EDITOR
				DUNGEON_GENERATOR_ERROR(TEXT("Cannot find a start gate that can be generated. %d: ID=%d (%d,%d,%d) %d Gate"), aisleIndex
					, static_cast<uint16_t>(startPoint->GetOwnerRoom()->GetIdentifier())
					, startPoint->GetOwnerRoom()->GetX(), startPoint->GetOwnerRoom()->GetY(), startPoint->GetOwnerRoom()->GetZ()
					, startPoint->GetOwnerRoom()->GetGateCount());
				DumpVoxel(startPoint);
				DumpAisleAndRoomInformation(aisleIndex);
#endif
			mLastError = Error::GateSearchFailed;
			return false;
		}
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

		for (size_t i = 0; i < floorHeight.size(); ++i)
		{
			if (height <= floorHeight[i])
				return i;
		}

		return 0;
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

	std::vector<std::shared_ptr<Room>> Generator::FindAll(const Point& point) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		std::copy_if(mRooms.begin(), mRooms.end(), result.begin(), [&point](const std::shared_ptr<Room>& room)
			{
				return room->Contain(point);
			}
		);
		return result;
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

	std::shared_ptr<Room> Generator::FindByIdentifier(const Identifier& identifier) const noexcept
	{
		std::shared_ptr<Room> result;
		for (auto& room : mRooms)
		{
			if (room->GetIdentifier() == identifier)
			{
				result = room;
			}
		}
		return result;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByDepth(const uint8_t depth) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		for (auto& room : mRooms)
		{
			if (room->GetDepthFromStart() == depth)
			{
				// cppcheck-suppress [useStlAlgorithm]
				result.emplace_back(room);
			}
		}
		return result;
	}

	std::vector<std::shared_ptr<Room>> Generator::FindByBranch(const uint8_t branchId) const noexcept
	{
		std::vector<std::shared_ptr<Room>> result;
		result.reserve(mRooms.size());
		for (auto& room : mRooms)
		{
			if (room->GetBranchId() == branchId)
			{
				// cppcheck-suppress [useStlAlgorithm]
				result.emplace_back(room);
			}
		}
		return result;
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

	void Generator::GenerateHeightImageForDebug(const PerlinNoise& perlinNoise, const std::size_t octaves, const float noiseBoostRatio, const std::string& filename) noexcept
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		{
			constexpr size_t width = 512;

			bmp::Canvas canvas(width, width);
			for (size_t y = 0; y < width; ++y)
			{
				for (size_t x = 0; x < width; ++x)
				{
					float noise = perlinNoise.OctaveNoise(
						octaves,
						static_cast<float>(x) / static_cast<float>(width) * 2.f - 1.f,
						static_cast<float>(y) / static_cast<float>(width) * 2.f - 1.f
					);
					noise = noise * 0.5f + 0.5f;
					noise *= noiseBoostRatio;
					noise = std::max(0.f, std::min(noise, 1.f));

					bmp::RGBCOLOR color;
					color.rgbBlue = color.rgbGreen = color.rgbRed = static_cast<uint8_t>(noise * 255.f);
					canvas.Put(x, y, color);
				}
			}
			canvas.Write(dungeon::GetDebugDirectoryString() + filename);
		}
#endif
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
			DUNGEON_GENERATOR_ERROR(TEXT("OK .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}
		{
			const Aisle& a = mAisles[index];
			const Point& s = a.GetPoint(0)->GetOwnerRoom()->GetCenter();
			const Point& g = a.GetPoint(1)->GetOwnerRoom()->GetCenter();
			const auto aid = static_cast<uint16_t>(a.GetIdentifier());
			const auto amp = a.IsMain() ? TCHAR('M') : TCHAR(' ');
			const auto sid = static_cast<uint16_t>(a.GetPoint(0)->GetOwnerRoom()->GetIdentifier());
			const auto gid = static_cast<uint16_t>(a.GetPoint(1)->GetOwnerRoom()->GetIdentifier());
			DUNGEON_GENERATOR_ERROR(TEXT("NG .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), index, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
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
			DUNGEON_GENERATOR_ERROR(TEXT("-- .. %d %c: ID=%d (%d:%f,%f,%f)-(%d:%f,%f,%f)"), j, amp, aid, sid, s.X, s.Y, s.Z, gid, g.X, g.Y, g.Z);
		}

		for (const auto& room : mRooms)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Room: ID=%d (X=%d,Y=%d,Z=%d) (W=%d,D=%d,H=%d) center(%f, %f, %f)")
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
			DUNGEON_GENERATOR_ERROR(TEXT("%s"), *text);
		}
#endif
	}
#endif
}
