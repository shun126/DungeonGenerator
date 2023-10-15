/**
ダンジョン生成パラメータに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Core/Math/Random.h"
#include <algorithm>
#include <limits>

namespace dungeon
{
	inline GenerateParameter::GenerateParameter()
		: mRandom(std::make_shared<Random>())
	{
	}

	inline uint8_t GenerateParameter::GetNumberOfCandidateFloors() const noexcept
	{
		return mNumberOfCandidateFloors;
	}

	inline uint8_t GenerateParameter::GetNumberOfCandidateRooms() const noexcept
	{
		return mNumberOfCandidateRooms;
	}

	inline uint32_t GenerateParameter::GetMinRoomWidth() const noexcept
	{
		return mMinRoomWidth;
	}

	inline uint32_t GenerateParameter::GetMaxRoomWidth() const noexcept
	{
		return mMaxRoomWidth;
	}

	inline uint32_t GenerateParameter::GetMinRoomDepth() const noexcept
	{
		return mMinRoomDepth;
	}

	inline uint32_t GenerateParameter::GetMaxRoomDepth() const noexcept
	{
		return mMaxRoomDepth;
	}

	inline uint32_t GenerateParameter::GetMinRoomHeight() const noexcept
	{
		return mMinRoomHeight;
	}

	inline uint32_t GenerateParameter::GetMaxRoomHeight() const noexcept
	{
		return mMaxRoomHeight;
	}

	inline uint32_t GenerateParameter::GetHorizontalRoomMargin() const noexcept
	{
		return mHorizontalRoomMargin;
	};

	inline uint32_t GenerateParameter::GetVerticalRoomMargin() const noexcept
	{
		return mVerticalRoomMargin;
	};

	inline std::shared_ptr<Random> GenerateParameter::GetRandom() noexcept
	{
		return mRandom;
	}

	inline std::shared_ptr<Random> GenerateParameter::GetRandom() const noexcept
	{
		return const_cast<GenerateParameter*>(this)->mRandom;
	}

	inline uint32_t GenerateParameter::GetWidth() const noexcept
	{
		return mWidth;
	}

	inline uint32_t GenerateParameter::GetDepth() const noexcept
	{
		return mDepth;
	}

	inline uint32_t GenerateParameter::GetHeight() const noexcept
	{
		return mHeight;
	}

	inline bool GenerateParameter::IsGenerateStartRoomReserved() const noexcept
	{
		return mStartRoomSize.IsZero() == false;
	}

	inline bool GenerateParameter::IsGenerateGoalRoomReserved() const noexcept
	{
		return mGoalRoomSize.IsZero() == false;
	}
}
