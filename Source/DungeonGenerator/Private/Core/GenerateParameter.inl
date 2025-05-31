/**
ダンジョン生成パラメータに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Math/Random.h"
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

	inline void GenerateParameter::SetNumberOfCandidateFloors(const uint8_t count) noexcept
	{
		mNumberOfCandidateFloors = count;
	}

	inline uint8_t GenerateParameter::GetNumberOfCandidateRooms() const noexcept
	{
		return mNumberOfCandidateRooms;
	}

	inline void GenerateParameter::SetNumberOfCandidateRooms(const uint8_t count) noexcept
	{
		mNumberOfCandidateRooms = count;
	}

	inline uint32_t GenerateParameter::GetMinRoomWidth() const noexcept
	{
		return mMinRoomWidth;
	}

	inline void GenerateParameter::SetMinRoomWidth(const uint32_t width) noexcept
	{
		mMinRoomWidth = width;
	}

	inline uint32_t GenerateParameter::GetMaxRoomWidth() const noexcept
	{
		return mMaxRoomWidth;
	}

	inline void GenerateParameter::SetMaxRoomWidth(const uint32_t width) noexcept
	{
		mMaxRoomWidth = width;
	}

	inline uint32_t GenerateParameter::GetMinRoomDepth() const noexcept
	{
		return mMinRoomDepth;
	}

	inline void GenerateParameter::SetMinRoomDepth(const uint32_t depth) noexcept
	{
		mMinRoomDepth = depth;
	}

	inline uint32_t GenerateParameter::GetMaxRoomDepth() const noexcept
	{
		return mMaxRoomDepth;
	}

	inline void GenerateParameter::SetMaxRoomDepth(const uint32_t depth) noexcept
	{
		mMaxRoomDepth = depth;
	}

	inline uint32_t GenerateParameter::GetMinRoomHeight() const noexcept
	{
		return mMinRoomHeight;
	}

	inline void GenerateParameter::SetMinRoomHeight(const uint32_t height) noexcept
	{
		mMinRoomHeight = height;
	}

	inline uint32_t GenerateParameter::GetMaxRoomHeight() const noexcept
	{
		return mMaxRoomHeight;
	}

	inline void GenerateParameter::SetMaxRoomHeight(const uint32_t height) noexcept
	{
		mMaxRoomHeight = height;
	}

	inline uint32_t GenerateParameter::GetHorizontalRoomMargin() const noexcept
	{
		return mHorizontalRoomMargin;
	}

	inline void GenerateParameter::SetHorizontalRoomMargin(const uint32_t margin) noexcept
	{
		mHorizontalRoomMargin = margin;
	}

	inline uint32_t GenerateParameter::GetVerticalRoomMargin() const noexcept
	{
		return mVerticalRoomMargin;
	}

	inline void GenerateParameter::SetVerticalRoomMargin(const uint32_t margin) noexcept
	{
		mVerticalRoomMargin = margin;
	}

	inline bool GenerateParameter::IsMergeRooms() const noexcept
	{
		return mMergeRooms;
	}

	inline void GenerateParameter::SetMergeRooms(const bool mergeRooms) noexcept
	{
		mMergeRooms = mergeRooms;
	}

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

	inline void GenerateParameter::SetWidth(const uint32_t width) noexcept
	{
		mWidth = width;
	}

	inline uint32_t GenerateParameter::GetDepth() const noexcept
	{
		return mDepth;
	}

	inline void GenerateParameter::SetDepth(const uint32_t depth) noexcept
	{
		mDepth = depth;
	}

	inline uint32_t GenerateParameter::GetHeight() const noexcept
	{
		return mHeight;
	}

	inline void GenerateParameter::SetHeight(const uint32_t height) noexcept
	{
		mHeight = height;
	}

	inline bool GenerateParameter::IsGenerateStartRoomReserved() const noexcept
	{
		return mStartRoomSize.IsZero() == false;
	}

	inline bool GenerateParameter::IsGenerateGoalRoomReserved() const noexcept
	{
		return mGoalRoomSize.IsZero() == false;
	}

	inline bool GenerateParameter::UseMissionGraph() const noexcept
	{
		return GetAisleComplexity() <= 0;
	}

	inline void GenerateParameter::SetMissionGraph(const bool use) noexcept
	{
		mUseMissionGraph = use;
	}

	inline uint8_t GenerateParameter::GetAisleComplexity() const noexcept
	{
		return mUseMissionGraph == false ? mAisleComplexity : 0;
	}

	inline void GenerateParameter::SetAisleComplexity(const uint8_t complexity) noexcept
	{
		mAisleComplexity = complexity;
	}

	inline bool GenerateParameter::IsAisleComplexity() const noexcept
	{
		return GetAisleComplexity() > 0;
	}

	inline bool GenerateParameter::IsGenerateSlopeInRoom() const noexcept
	{
		return mGenerateSlopeInRoom;
	}

	inline void GenerateParameter::SetGenerateSlopeInRoom(const bool generateSlopeInRoom) noexcept
	{
		mGenerateSlopeInRoom = generateSlopeInRoom;
	}

	inline bool GenerateParameter::IsGenerateStructuralColumn() const noexcept
	{
		return mGenerateStructuralColumn;
	}

	inline void GenerateParameter::SetGenerateStructuralColumn(const bool generateStructuralColumn) noexcept
	{
		mGenerateStructuralColumn = generateStructuralColumn;
	}

	inline const FIntVector& GenerateParameter::GetStartRoomSize() const noexcept
	{
		return mStartRoomSize;
	}

	inline void GenerateParameter::SetStartRoomSize(const FIntVector& size) noexcept
	{
		mStartRoomSize = size;
	}

	inline const FIntVector& GenerateParameter::GetGoalRoomSize() const noexcept
	{
		return mGoalRoomSize;
	}

	inline void GenerateParameter::SetGoalRoomSize(const FIntVector& size) noexcept
	{
		mGoalRoomSize = size;
	}
}
