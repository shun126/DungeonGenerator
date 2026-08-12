/**
 * 通路に関するヘッダーファイル
 *
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "../Math/Point.h"
#include "Room.h"
#include <fstream>

namespace dungeon
{
	inline Aisle::Aisle(const bool main, const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const EDungeonAislePurpose purpose) noexcept
		: mIdentifier(Identifier::Type::Aisle)
		, mMain(main)
		, mPurpose(purpose)
	{
		mPoints[0] = p0;
		mPoints[1] = p1;
		mLength = Point::Dist(*p0, *p1);
	}

	inline Aisle::Aisle(const Aisle& other) noexcept
		: mPoints(other.mPoints)
		, mLength(other.mLength)
		, mIdentifier(other.mIdentifier)
		, mMain(other.mMain)
		, mPurpose(other.mPurpose)
		, mHeight(other.mHeight)
		, mLocked(other.mLocked)
		, mUniqueLocked(other.mUniqueLocked)
	{
	}

	inline Aisle::Aisle(Aisle&& other) noexcept
		: mPoints(std::move(other.mPoints))
		, mLength(std::move(other.mLength))
		, mIdentifier(std::move(other.mIdentifier))
		, mMain(std::move(other.mMain))
		, mPurpose(std::move(other.mPurpose))
		, mHeight(std::move(other.mHeight))
		, mLocked(std::move(other.mLocked))
		, mUniqueLocked(std::move(other.mUniqueLocked))
	{
	}

	inline Aisle& Aisle::operator=(const Aisle& other) noexcept
	{
		mPoints = other.mPoints;
		mLength = other.mLength;
		mIdentifier = other.mIdentifier;
		mMain = other.mMain;
		mPurpose = other.mPurpose;
		mHeight = other.mHeight;
		mLocked = other.mLocked;
		mUniqueLocked = other.mUniqueLocked;
		return *this;
	}

	inline Aisle& Aisle::operator=(Aisle&& other) noexcept
	{
		mPoints = std::move(other.mPoints);
		mLength = std::move(other.mLength);
		mIdentifier = std::move(other.mIdentifier);
		mMain = std::move(other.mMain);
		mPurpose = std::move(other.mPurpose);
		mHeight = std::move(other.mHeight);
		mLocked = std::move(other.mLocked);
		mUniqueLocked = std::move(other.mUniqueLocked);
		return *this;
	}

	inline bool Aisle::Contain(const std::shared_ptr<const Point>& point) const noexcept
	{
		return mPoints[0] == point || mPoints[1] == point;
	}

	inline const std::shared_ptr<const Point>& Aisle::GetPoint(const size_t index) const noexcept
	{
		return mPoints.at(index);
	}

	inline double Aisle::GetLength() const noexcept
	{
		return mLength;
	}

	inline const Identifier& Aisle::GetIdentifier() const noexcept
	{
		return mIdentifier;
	}

	inline int32 Aisle::GetZoneIndex() const noexcept
	{
		const std::shared_ptr<Room>& firstRoom = mPoints[0]->GetOwnerRoom();
		const std::shared_ptr<Room>& secondRoom = mPoints[1]->GetOwnerRoom();
		const bool useSecondRoomZone = firstRoom->GetDepthFromStart() < secondRoom->GetDepthFromStart()
			|| (firstRoom->GetDepthFromStart() == secondRoom->GetDepthFromStart()
				&& static_cast<uint16_t>(firstRoom->GetIdentifier()) < static_cast<uint16_t>(secondRoom->GetIdentifier()));
		return useSecondRoomZone ? secondRoom->GetZoneIndex() : firstRoom->GetZoneIndex();
	}

	inline bool Aisle::IsMain() const noexcept
	{
		return mMain;
	}

	inline void Aisle::SetMain(const bool main) noexcept
	{
		mMain = main;
	}

	inline EDungeonAislePurpose Aisle::GetPurpose() const noexcept
	{
		return mPurpose;
	}

	inline bool Aisle::IsVerticalTransition() const noexcept
	{
		const std::shared_ptr<const Point>& point0 = GetPoint(0);
		const std::shared_ptr<const Point>& point1 = GetPoint(1);
		if (point0 == nullptr || point1 == nullptr)
			return false;

		const std::shared_ptr<Room>& room0 = point0->GetOwnerRoom();
		const std::shared_ptr<Room>& room1 = point1->GetOwnerRoom();
		if (room0 == nullptr || room1 == nullptr)
			return false;

		return room0->GetZ() != room1->GetZ();
	}

	inline void Aisle::SetPurpose(const EDungeonAislePurpose purpose) noexcept
	{
		mPurpose = purpose;
	}

	inline bool Aisle::IsLocked() const noexcept
	{
		return mLocked;
	}

	inline void Aisle::SetLock(const bool lock) noexcept
	{
		mLocked = lock;

		if (lock == false)
			mUniqueLocked = false;
	}

	inline bool Aisle::IsUniqueLocked() const noexcept
	{
		return mUniqueLocked;
	}

	inline void Aisle::SetUniqueLock(const bool lock) noexcept
	{
		mLocked = mUniqueLocked = lock;
	}

	inline bool Aisle::IsAnyLocked() const noexcept
	{
		return IsLocked() || IsUniqueLocked();
	}


	inline uint8_t Aisle::GetHeight() const noexcept
	{
		return mHeight;
	}

	inline void Aisle::SetHeight(const uint8_t height) noexcept
	{
		check(1 <= height && height <= 2);
		mHeight = height;
	}

	inline bool Aisle::operator==(const Aisle& other) const noexcept
	{
		return
			(mPoints[0] == other.mPoints[0] && mPoints[1] == other.mPoints[1]) ||
			(mPoints[0] == other.mPoints[1] && mPoints[1] == other.mPoints[0]);
		// mLockedを考慮していない
	}

	inline bool Aisle::operator!=(const Aisle& other) const noexcept
	{
		return !(*this == other);
	}

	inline void Aisle::Dump(std::ofstream& stream) const noexcept
	{
		mPoints[0]->Dump(stream);
		mPoints[1]->Dump(stream);
		stream << "Lock: " << (mLocked ? "Locked" : "Unlocked") << std::endl;
	}
}
