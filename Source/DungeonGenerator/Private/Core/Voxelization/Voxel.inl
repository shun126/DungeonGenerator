/**
グリッドに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline void Voxel::NoRoofMeshGeneration(const FIntVector& location, const bool noRoofMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoRoofMeshGeneration(noRoofMeshGeneration);
		}
	}

	inline void Voxel::NoFloorMeshGeneration(const FIntVector& location, const bool noFloorMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoFloorMeshGeneration(noFloorMeshGeneration);
		}
	}

	inline void Voxel::NoNorthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoNorthWallMeshGeneration(noWallMeshGeneration);
		}
	}

	inline void Voxel::NoSouthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoSouthWallMeshGeneration(noWallMeshGeneration);
		}
	}

	inline void Voxel::NoEastWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoEastWallMeshGeneration(noWallMeshGeneration);
		}
	}

	inline void Voxel::NoWestWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoWestWallMeshGeneration(noWallMeshGeneration);
		}
	}

	inline Voxel::Error Voxel::GetLastError() const noexcept
	{
		return mLastError;
	}

	inline bool Voxel::Contain(const FIntVector& location) const noexcept
	{
		return
			(0 <= location.X && location.X < static_cast<int32_t>(mWidth)) &&
			(0 <= location.Y && location.Y < static_cast<int32_t>(mDepth)) &&
			(0 <= location.Z && location.Z < static_cast<int32_t>(mHeight));
	}

	inline size_t Voxel::Index(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept
	{
		const size_t index
			= static_cast<size_t>(z) * mWidth * mDepth
			+ static_cast<size_t>(y) * mWidth
			+ static_cast<size_t>(x);
		return index;
	}

	inline size_t Voxel::Index(const FIntVector& location) const noexcept
	{
		const size_t index
			= static_cast<size_t>(location.Z) * mWidth * mDepth
			+ static_cast<size_t>(location.Y) * mWidth
			+ static_cast<size_t>(location.X);
		return index;
	}

	inline const Grid& Voxel::Get(const FIntVector& location) const noexcept
	{
		return Get(location.X, location.Y, location.Z);
	}

	inline const Grid& Voxel::Get(const size_t index) const noexcept
	{
		check(index < static_cast<size_t>(mWidth) * mDepth * mHeight);
		return mGrids.get()[index];
	}

	inline Grid& Voxel::operator[](const size_t index) noexcept
	{
		check(index < static_cast<size_t>(mWidth) * mDepth * mHeight);
		return mGrids.get()[index];
	}

	inline const Grid& Voxel::operator[](const size_t index) const noexcept
	{
		check(index < static_cast<size_t>(mWidth) * mDepth * mHeight);
		return mGrids.get()[index];
	}

	inline uint32_t Voxel::GetWidth() const noexcept
	{
		return mWidth;
	}

	inline uint32_t Voxel::GetDepth() const noexcept
	{
		return mDepth;
	}

	inline uint32_t Voxel::GetHeight() const noexcept
	{
		return mHeight;
	}


	inline Voxel::Route::Route(const FIntVector& start, const FIntVector& idealGoal)
		: mStart(start)
		, mIdealGoal(idealGoal)
	{
	}
}
