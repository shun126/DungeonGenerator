/**
部屋に関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline const Identifier& Room::GetIdentifier() const noexcept
	{
		return mIdentifier;
	}

	inline int32_t Room::GetX() const noexcept
	{
		return mX;
	}

	inline int32_t Room::GetY() const noexcept
	{
		return mY;
	}

	inline int32_t Room::GetZ() const noexcept
	{
		return mZ;
	}

	inline void Room::SetX(const int32_t x) noexcept
	{
		mX = x;
	}

	inline void Room::SetY(const int32_t y) noexcept
	{
		mY = y;
	}

	inline void Room::SetZ(const int32_t z) noexcept
	{
		mZ = z;
	}

	inline int32_t Room::GetWidth() const noexcept
	{
		return mWidth;
	}

	inline int32_t Room::GetDepth() const noexcept
	{
		return mDepth;
	}

	inline int32_t Room::GetHeight() const noexcept
	{
		return mHeight;
	}

	inline void Room::SetWidth(const int32_t width) noexcept
	{
		mWidth = width;
	}

	inline void Room::SetDepth(const int32_t depth) noexcept
	{
		mDepth = depth;
	}

	inline void Room::SetHeight(const int32_t height) noexcept
	{
		mHeight = height;
	}

	inline int32_t Room::GetLeft() const noexcept
	{
		return mX;
	}

	inline int32_t Room::GetRight() const noexcept
	{
		return mX + mWidth;
	}

	inline int32_t Room::GetTop() const noexcept
	{
		return mY;
	}

	inline int32_t Room::GetBottom() const noexcept
	{
		return mY + mDepth;
	}

	inline FIntRect Room::GetRect() const noexcept
	{
		const int32_t l = GetLeft();
		const int32_t t = GetTop();
		const int32_t r = GetRight();
		const int32_t b = GetBottom();
		return FIntRect(l, t, r, b);
	}

	inline int32_t Room::GetForeground() const noexcept
	{
		return mZ + mHeight;
	}

	inline int32_t Room::GetBackground() const noexcept
	{
		return mZ;
	}

	inline Point Room::GetCenter() const noexcept
	{
		const float x = static_cast<float>(GetX()) + static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetY()) + static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetZ()) + static_cast<float>(GetHeight()) * 0.5f;
		return Point(x, y, z);
	}

	inline FVector Room::GetExtent() const noexcept
	{
		const float x = static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetHeight()) * 0.5f;
		return FVector(x, y, z);
	}

	inline Point Room::GetGroundCenter() const noexcept
	{
		const float x = static_cast<float>(GetX()) + static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetY()) + static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetZ());
		return Point(x, y, z);
	}

	inline bool Room::Intersect(const Room& other) const noexcept
	{
		return Intersect(other, 0, 0);
	}

	inline bool Room::Contain(const Point& point) const noexcept
	{
		return Contain(point.X, point.Y, point.Z);
	}

	inline bool Room::Contain(const FIntVector& location) const noexcept
	{
		return Contain(location.X, location.Y, location.Z);
	}

	inline Room::Parts Room::GetParts() const noexcept
	{
		return mParts;
	}

	inline void Room::SetParts(const Parts parts) noexcept
	{
		mParts = parts;
	}

	inline Room::Item Room::GetItem() const noexcept
	{
		return mItem;
	}

	inline void Room::SetItem(const Item item) noexcept
	{
		mItem = item;
	}

	inline uint8_t Room::GetDepthFromStart() const noexcept
	{
		return mDepthFromStart;
	}

	inline void Room::SetDepthFromStart(const uint8_t depthFromStart) noexcept
	{
		mDepthFromStart = depthFromStart;
	}

	inline uint8_t Room::GetBranchId() const noexcept
	{
		return mBranchId;
	}

	inline void Room::SetBranchId(const uint8_t branchId) noexcept
	{
		mBranchId = branchId;
	}

	inline uint8_t Room::GetGateCount() const noexcept
	{
		return mNumberOfGates;
	}

	inline void Room::ResetGateCount() noexcept
	{
		mNumberOfGates = 0;
	}

	inline void Room::AddGateCount(const uint8_t count) noexcept
	{
		mNumberOfGates += count;
	}

	inline uint8_t Room::GetVerticalRoomMargin() const noexcept
	{
		return mVerticalRoomMargin;
	}

	inline FIntVector Room::GetDataSize() const noexcept
	{
		return FIntVector(mDataWidth, mDataDepth, mDataHeight);
	}

	inline void Room::SetDataSize(const uint32_t dataWidth, const uint32_t dataDepth, const uint32_t dataHeight)
	{
		mDataWidth = dataWidth;
		mDataDepth = dataDepth;
		mDataHeight = dataHeight;
	}

	inline bool Room::IsValidReservationNumber() const noexcept
	{
		return GetReservationNumber() != 0;
	}

	inline uint32_t Room::GetReservationNumber() const noexcept
	{
		return mReservationNumber;
	}

	inline void Room::SetReservationNumber(const uint32_t reservationNumber) noexcept
	{
		mReservationNumber = reservationNumber;
	}

	inline void Room::ResetReservationNumber() noexcept
	{
		SetReservationNumber(0);
	}
}
