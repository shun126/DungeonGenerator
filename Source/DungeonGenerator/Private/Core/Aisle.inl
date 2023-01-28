/**
通路に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Math/Point.h"

namespace dungeon
{
	inline Aisle::Aisle(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1) noexcept
	{
		mPoints[0] = p0;
		mPoints[1] = p1;
		mLength = Point::Dist(*p0, *p1);
	}

	inline Aisle::Aisle(const Aisle& other) noexcept
		: mPoints(other.mPoints)
		, mLength(other.mLength)
		, mIdentifier(other.mIdentifier)
		, mLocked(other.mLocked)
		, mUniqueLocked(other.mUniqueLocked)
	{
	}

	inline Aisle::Aisle(Aisle&& other) noexcept
		: mPoints(std::move(other.mPoints))
		, mLength(std::move(other.mLength))
		, mIdentifier(std::move(other.mIdentifier))
		, mLocked(std::move(other.mLocked))
		, mUniqueLocked(std::move(other.mUniqueLocked))
	{
	}

	inline Aisle& Aisle::operator=(const Aisle& other) noexcept
	{
		mPoints = other.mPoints;
		mLength = other.mLength;
		mIdentifier = other.mIdentifier;
		mLocked = other.mLocked;
		mUniqueLocked = other.mUniqueLocked;
		return *this;
	}

	inline Aisle& Aisle::operator=(Aisle&& other) noexcept
	{
		mPoints = std::move(other.mPoints);
		mLength = std::move(other.mLength);
		mIdentifier = std::move(other.mIdentifier);
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
