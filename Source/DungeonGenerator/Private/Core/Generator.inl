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

	inline void Generator::ForEach(std::function<void(const std::shared_ptr<Room>&)> func) noexcept
	{
		for (const auto& room : mRooms)
		{
			func(room);
		}
	}

	/**
	生成された部屋を参照します
	*/
	inline void Generator::ForEach(std::function<void(const std::shared_ptr<const Room>&)> func) const noexcept
	{
		for (const auto& room : mRooms)
		{
			func(room);
		}
	}


	inline void Generator::EachAisle(std::function<bool(const Aisle& edge)> func) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			if (!func(aisle))
				break;
		}
	}

	inline void Generator::FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(Aisle& edge)> func) noexcept
	{
		for (auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (func(aisle))
					break;
			}
		}
	}

	inline void Generator::FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(const Aisle& edge)> func) const noexcept
	{
		for (const auto& aisle : mAisles)
		{
			const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
			const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
			if (room == room0 || room == room1)
			{
				if (func(aisle))
					break;
			}
		}
	}

	inline void Generator::OnQueryParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept
	{
		mOnQueryParts = func;
	}

	inline void Generator::OnStartParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept
	{
		mOnStartParts = func;
	}

	inline void Generator::OnGoalParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept
	{
		mOnGoalParts = func;
	}


	inline const std::shared_ptr<const Point>& Generator::GetStartPoint() const noexcept
	{
		return mStartPoint;
	}

	inline const std::shared_ptr<const Point>& Generator::GetGoalPoint() const noexcept
	{
		return mGoalPoint;
	}

	inline void Generator::EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const noexcept
	{
		for (auto& point : mLeafPoints)
		{
			func(point);
		}
	}

	inline uint8_t Generator::GetDeepestDepthFromStart() const noexcept
	{
		return mDistance;
	}

	inline void Generator::PreGenerateVoxel(std::function<void(const std::shared_ptr<Voxel>&)> func) noexcept
	{
		mOnPreGenerateVoxel = func;
	}

	inline void Generator::PostGenerateVoxel(std::function<void(const std::shared_ptr<Voxel>&)> func) noexcept
	{
		mOnPostGenerateVoxel = func;
	}
}
