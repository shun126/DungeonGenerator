/**
ベクターに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>

namespace dungeon
{
	/**
	Converts an integer vector to a real vector
	*/
	inline FVector ToVector(const FIntVector& vector) noexcept
	{
		return FVector(static_cast<double>(vector.X), static_cast<double>(vector.Y), static_cast<double>(vector.Z));
	}

	/**
	Converts an real vector to a integer vector
	*/
	inline FIntVector ToIntVector(const FVector& vector) noexcept
	{
		return FIntVector(static_cast<int32>(vector.X), static_cast<int32>(vector.Y), static_cast<int32>(vector.Z));
	}

	/**
	Converts an integer vector to a integer point
	(Note that the Z value is discarded.)
	*/
	inline FIntPoint ToIntPoint(const FIntVector& vector) noexcept
	{
		return FIntPoint(vector.X, vector.Y);
	}

	/**
	Converts an real vector to a integer point
	(Note that the Z value is discarded.)
	*/
	inline FIntPoint ToIntPoint(const FVector& vector) noexcept
	{
		return FIntPoint(static_cast<int32>(vector.X), static_cast<int32>(vector.Y));
	}

	/**
	 * FIntVectorの長さの二乗
	 * @param vector ベクトル
	 * @return 長さの二乗
	 */
	inline int32 SquaredLength(const FIntVector& vector) noexcept
	{
		return vector.X * vector.X + vector.Y * vector.Y + vector.Z * vector.Z;
	}

	/**
	 * FIntVectorの距離の二乗
	 * @param a 位置ベクトル
	 * @param b 位置ベクトル
	 * @return 距離の二乗
	 */
	inline int32 SquaredDistance(const FIntVector& a, const FIntVector& b) noexcept
	{
		const FIntVector delta = a - b;
		return SquaredLength(delta);
	}
}
