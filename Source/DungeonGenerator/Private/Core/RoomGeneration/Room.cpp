/**
部屋に関するソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Room.h"
#include "../GenerateParameter.h"
#include "../Math/Random.h"
#include <array>
#include <string>
#include <string_view>

namespace
{
	/**
	サイズを乱数で選択
	@param[in]	minimum		最小値
	@param[in]	maximum		最大値
	@return		乱数で決めたサイズ
	*/
	static uint32_t randSize(const std::shared_ptr<dungeon::Random>& random, const uint32_t minimum, const uint32_t maximum)
	{
		return random->Get<uint32_t>(minimum, maximum + 1);
	}
}

namespace dungeon
{
	Room::Room(const GenerateParameter& parameter, const FIntVector& location) noexcept
		: mX(location.X)
		, mY(location.Y)
		, mZ(location.Z)
		, mIdentifier(Identifier::Type::Room)
	{
		mHeight = randSize(parameter.GetRandom(), parameter.GetMinRoomHeight(), parameter.GetMaxRoomHeight());
		mWidth = randSize(parameter.GetRandom(), parameter.GetMinRoomWidth(), parameter.GetMaxRoomWidth());
		mDepth = randSize(parameter.GetRandom(), parameter.GetMinRoomDepth(), parameter.GetMaxRoomDepth());
	}

	Room::Room(const FIntVector& location, const FIntVector& size, const bool undeletable) noexcept
		: mX(location.X)
		, mY(location.Y)
		, mZ(location.Z)
		, mWidth(size.X)
		, mDepth(size.Y)
		, mHeight(size.Z)
		, mIdentifier(Identifier::Type::Room)
	{
	}

	Room::Room(const Room& other) noexcept
		: mX(other.mX)
		, mY(other.mY)
		, mZ(other.mZ)
		, mWidth(other.mWidth)
		, mDepth(other.mDepth)
		, mHeight(other.mHeight)
		, mDataWidth(other.mDataWidth)
		, mDataDepth(other.mDataDepth)
		, mDataHeight(other.mDataHeight)
		, mIdentifier(other.mIdentifier)
		, mParts(other.mParts)
		, mItem(other.mItem)
		, mDepthFromStart(other.mDepthFromStart)
		, mBranchId(other.mBranchId)
	{
	}

	Room& Room::operator=(const Room& other) noexcept
	{
		mX = other.mX;
		mY = other.mY;
		mZ = other.mZ;
		mWidth = other.mWidth;
		mDepth = other.mDepth;
		mHeight = other.mHeight;
		mDataWidth = other.mDataWidth;
		mDataDepth = other.mDataDepth;
		mDataHeight = other.mDataHeight;
		mIdentifier = other.mIdentifier;
		mParts = other.mParts;
		mItem = other.mItem;
		mDepthFromStart = other.mDepthFromStart;
		mBranchId = other.mBranchId;
		return *this;
	}

	bool Room::Intersect(const Room& other, const uint32_t horizontalMargin, const uint32_t verticalMargin) const noexcept
	{
		const int32_t selfMinX = GetLeft() - horizontalMargin;
		const int32_t selfMaxX = GetRight() + horizontalMargin;
		const int32_t selfMinY = GetTop() - horizontalMargin;
		const int32_t selfMaxY = GetBottom() + horizontalMargin;
		const int32_t selfMinZ = GetBackground() - verticalMargin;
		const int32_t selfMaxZ = GetForeground() + verticalMargin;
		const int32_t otherMinX = other.GetLeft();
		const int32_t otherMaxX = other.GetRight();
		const int32_t otherMinY = other.GetTop();
		const int32_t otherMaxY = other.GetBottom();
		const int32_t otherMinZ = other.GetBackground();
		const int32_t otherMaxZ = other.GetForeground();
#if 1
		if (selfMaxX <= otherMinX || selfMinX >= otherMaxX)
			return false;
		if (selfMaxY <= otherMinY || selfMinY >= otherMaxY)
			return false;
		if (selfMaxZ <= otherMinZ || selfMinZ >= otherMaxZ)
			return false;
		return true;
#else
		return
			selfMaxX >= otherMinX &&
			selfMinX <= otherMaxX &&
			selfMaxY >= otherMinY &&
			selfMinY <= otherMaxY &&
			selfMinZ <= otherMaxZ &&
			selfMaxZ >= otherMinZ;
#endif
	}

	bool Room::Contain(const int32_t x, const int32_t y, const int32_t z) const noexcept
	{
		const int32_t minX = GetLeft();
		const int32_t maxX = GetRight();
		const int32_t minY = GetTop();
		const int32_t maxY = GetBottom();
		const int32_t minZ = GetBackground();
		const int32_t maxZ = GetForeground();
		return
			(minX <= x && x < maxX) &&
			(minY <= y && y < maxY) &&
			(minZ <= z && z < maxZ);
	}

	std::string Room::GetName() const noexcept
	{
		return std::to_string(mX) + "_" + std::to_string(mY) + "_" + std::to_string(mZ);
	}

	const std::string_view& Room::GetPartsName() const noexcept
	{
		static const std::array<std::string_view, PartsSize> names = {
			"unidentified",
			"hall",
			"hanare",
			"start",
			"goal",
		};
		return names[static_cast<size_t>(mParts)];
	}

	const std::string_view& Room::GetItemName() const noexcept
	{
		static const std::array<std::string_view, ItemSize> names = {
			"Empty",
			"Key",
			"Unique key",
		};
		return names[static_cast<size_t>(mItem)];
	}

	void Room::GetDataBounds(FIntVector& min, FIntVector& max) const noexcept
	{
#if 1
		min = FIntVector(mX, mY, mZ);
		max = min + FIntVector(mWidth, mDepth, mHeight);
#else
		const FIntVector center(GetGroundCenter());
		const FIntVector size = GetDataSize();
		const FIntVector offset(size.X / 2, size.Y / 2, 0);
		min = center - offset;
		max = min + size;
#endif
	}
}
