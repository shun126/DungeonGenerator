/**
ダンジョン生成ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once 

namespace dungeon
{
	inline Generator::Error Generator::GetLastError() const noexcept
	{
		return mLastError;
	}

	inline void Generator::OnQueryParts(const std::function<void(QueryPartsType&)>& function) noexcept
	{
		mOnQueryParts = function;
	}

	inline void Generator::OnLoadParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept
	{
		mOnLoadParts = function;
	}

	inline void Generator::OnLoadStartParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept
	{
		mOnLoadStartParts = function;
	}

	inline void Generator::OnLoadGoalParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept
	{
		mOnLoadGoalParts = function;
	}

	inline const std::shared_ptr<const Point>& Generator::GetStartPoint() const noexcept
	{
		return mStartPoint;
	}

	inline const std::shared_ptr<const Point>& Generator::GetGoalPoint() const noexcept
	{
		return mGoalPoint;
	}

	inline uint8_t Generator::GetDeepestDepthFromStart() const noexcept
	{
		return mDeepestDepthFromStart;
	}

	inline void Generator::PreGenerateVoxel(const std::function<void(const std::shared_ptr<Voxel>&)>& function) noexcept
	{
		mOnPreGenerateVoxel = function;
	}

	inline void Generator::PostGenerateVoxel(const std::function<void(const std::shared_ptr<Voxel>&)>& function) noexcept
	{
		mOnPostGenerateVoxel = function;
	}
}
