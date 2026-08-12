/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <Math/IntVector.h>
#include <Math/Vector.h>

namespace dungeon::spatial_mesh_group
{
	constexpr int32 HorizontalGridCount = 8;
	constexpr int32 VerticalGridCount = 2;

	/**
	 * Calculates the stable XYZ group coordinate shared by generated instanced meshes and vegetation.
	 * 生成インスタンスメッシュと植生で共有する安定したXYZグループ座標を計算します。
	 */
	inline FIntVector CalculateCoordinate(const FVector& worldPosition, const FVector& actorLocation, const FVector& gridSize)
	{
		const FVector safeGridSize(
			FMath::Max(gridSize.X, 1.0),
			FMath::Max(gridSize.Y, 1.0),
			FMath::Max(gridSize.Z, 1.0)
		);
		const FVector groupWorldSize(
			safeGridSize.X * HorizontalGridCount,
			safeGridSize.Y * HorizontalGridCount,
			safeGridSize.Z * VerticalGridCount
		);
		const FVector localPosition = worldPosition - actorLocation;
		return FIntVector(
			FMath::FloorToInt(localPosition.X / groupWorldSize.X),
			FMath::FloorToInt(localPosition.Y / groupWorldSize.Y),
			FMath::FloorToInt(localPosition.Z / groupWorldSize.Z)
		);
	}

	/**
	 * Orders spatial coordinates deterministically for deferred work scheduling.
	 * 遅延処理の順序を安定させるため空間座標を決定的に並べます。
	 */
	inline bool Less(const FIntVector& left, const FIntVector& right) noexcept
	{
		if (left.Z != right.Z)
			return left.Z < right.Z;
		if (left.Y != right.Y)
			return left.Y < right.Y;
		return left.X < right.X;
	}
}
