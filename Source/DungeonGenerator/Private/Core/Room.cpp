/*!
部屋に関するソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Room.h"
#include "Core/Math/Random.h"
#include "GenerateParameter.h"
#include <array>
#include <cassert>
#include <string>
#include <string_view>

namespace
{
	/*!
	サイズを乱数で選択
	\param[in]	minimum		最小値
	\param[in]	maximum		最大値
	\return		乱数で決めたサイズ
	*/
	static uint32_t randSize(dungeon::Random& random, const uint32_t minimum, const uint32_t maximum)
	{
		return random.Get<uint32_t>(minimum, maximum);
	}
}

namespace dungeon
{
	Room::Room(const GenerateParameter& parameter)
	{
		mWidth = randSize(parameter.GetRandom(), parameter.GetMinRoomWidth(), parameter.GetMaxRoomWidth());
		mDepth = randSize(parameter.GetRandom(), parameter.GetMinRoomDepth(), parameter.GetMaxRoomDepth());
		mHeight = randSize(parameter.GetRandom(), parameter.GetMinRoomHeight(), parameter.GetMaxRoomHeight());

		const float range = std::max(1.f, std::sqrtf(parameter.GetNumberOfCandidateRooms()));
		const float radian = parameter.GetRandom().Get<float>() * (3.14159265359f * 2.f);
		const float width = std::sin(radian) * range;
		const float depth = std::cos(radian) * range;
		mX = static_cast<int32_t>(std::round(width));
		mY = static_cast<int32_t>(std::round(depth));
		if (parameter.GetRoomMargin() > 0)
		{
			const float height = 3.f * parameter.GetRandom().Get<float>();
			mZ = static_cast<int32_t>(std::round(height));
		}
		else
		{
			/*
			RoomMarginが0の場合、部屋と部屋の間にスロープを作る隙間が無いので、
			部屋の高さを必ず同じにする必要がある。
			*/
			mZ = 0;
		}
	}

	Room::Room(const Room& other)
		: mX(other.mX)
		, mY(other.mY)
		, mZ(other.mZ)
		, mWidth(other.mWidth)
		, mDepth(other.mDepth)
		, mHeight(other.mHeight)
		, mUnderfloorHeight(other.mUnderfloorHeight)
		, mIdentifier(other.mIdentifier)
		, mParts(other.mParts)
		, mItem(other.mItem)
		, mDepthFromStart(other.mDepthFromStart)
		, mBranchId(other.mBranchId)
	{
	}

	Room::Room(Room&& other) noexcept
		: mX(std::move(other.mX))
		, mY(std::move(other.mY))
		, mZ(std::move(other.mZ))
		, mWidth(std::move(other.mWidth))
		, mDepth(std::move(other.mDepth))
		, mHeight(std::move(other.mHeight))
		, mUnderfloorHeight(std::move(other.mUnderfloorHeight))
		, mIdentifier(std::move(other.mIdentifier))
		, mParts(std::move(other.mParts))
		, mItem(std::move(other.mItem))
		, mDepthFromStart(std::move(other.mDepthFromStart))
		, mBranchId(std::move(other.mBranchId))
	{
	}

	const Identifier& Room::GetIdentifier() const
	{
		return mIdentifier;
	}

	int32_t Room::GetX() const
	{
		return mX;
	}

	int32_t Room::GetY() const
	{
		return mY;
	}

	int32_t Room::GetZ() const
	{
		return mZ;
	}

	void Room::SetX(const int32_t x)
	{
		mX = x;
	}

	void Room::SetY(const int32_t y)
	{
		mY = y;
	}

	void Room::SetZ(const int32_t z)
	{
		mZ = z;
	}

	int32_t Room::GetWidth() const
	{
		return mWidth;
	}

	int32_t Room::GetDepth() const
	{
		return mDepth;
	}

	int32_t Room::GetHeight() const
	{
		return mHeight;
	}

	uint16_t Room::GetUnderfloorHeight() const
	{
		return mUnderfloorHeight;
	}

	void Room::SetWidth(const int32_t width)
	{
		mWidth = width;
	}

	void Room::SetDepth(const int32_t depth)
	{
		mDepth = depth;
	}

	void Room::SetHeight(const int32_t height)
	{
		mHeight = height;
	}

	void Room::SetUnderfloorHeight(const uint16_t underfloorHeight)
	{
		mUnderfloorHeight = underfloorHeight;
	}

	int32_t Room::GetLeft() const
	{
		return mX;
	}

	int32_t Room::GetRight() const
	{
		return mX + mWidth;
	}

	int32_t Room::GetTop() const
	{
		return mY;
	}

	int32_t Room::GetBottom() const
	{
		return mY + mDepth;
	}

	FIntRect Room::GetRect() const
	{
		const int32_t l = GetLeft();
		const int32_t t = GetTop();
		const int32_t r = GetRight();
		const int32_t b = GetBottom();
		return FIntRect(l, t, r, b);
	}

	int32_t Room::GetForeground() const
	{
		return mZ + mHeight;
	}

	int32_t Room::GetBackground() const
	{
		return mZ;
	}

	Point Room::GetCenter() const
	{
		const float x = static_cast<float>(GetX()) + static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetY()) + static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetZ()) + static_cast<float>(GetHeight()) * 0.5f;
		return Point(x, y, z);
	}

	FVector Room::GetExtent() const
	{
		const float x = static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetHeight()) * 0.5f;
		return FVector(x, y, z);
	}

	Point Room::GetFloorCenter() const
	{
		const float x = static_cast<float>(GetX()) + static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetY()) + static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetZ() + GetUnderfloorHeight());
		return Point(x, y, z);
	}

	Point Room::GetGroundCenter() const
	{
		const float x = static_cast<float>(GetX()) + static_cast<float>(GetWidth()) * 0.5f;
		const float y = static_cast<float>(GetY()) + static_cast<float>(GetDepth()) * 0.5f;
		const float z = static_cast<float>(GetZ());
		return Point(x, y, z);
	}

	bool Room::Intersect(const Room& other) const
	{
		return
			(GetLeft() < other.GetRight() && GetRight() > other.GetLeft()) &&
			(GetTop() < other.GetBottom() && GetBottom() > other.GetTop()) &&
			(GetBackground() < other.GetForeground() && GetForeground() > other.GetBackground());
	}

	bool Room::Intersect(const Room& other, const uint32_t margin) const
	{
		const int32_t left = GetLeft() - margin;
		const int32_t right = GetRight() + margin;
		const int32_t top = GetTop() - margin;
		const int32_t bottom = GetBottom() + margin;
		const int32_t foreground = GetForeground() + 0;
		const int32_t background = GetBackground() - 1;
		return
			(left < other.GetRight() && right > other.GetLeft()) &&
			(top < other.GetBottom() && bottom > other.GetTop()) &&
			(background < other.GetForeground() && foreground > other.GetBackground());
	}

	bool Room::Contain(const Point& point) const
	{
		return Contain(point.X, point.Y, point.Z);
	}

	bool Room::Contain(const int32_t x, const int32_t y, const int32_t z) const
	{
		return
			(GetLeft() <= x && x <= GetRight()) &&
			(GetTop() <= y && y <= GetBottom()) &&
			(GetBackground() <= z && z <= GetForeground());
	}

	Room::Parts Room::GetParts() const
	{
		return mParts;
	}

	void Room::SetParts(const Parts parts)
	{
		mParts = parts;
	}

	Room::Item Room::GetItem() const
	{
		return mItem;
	}

	void Room::SetItem(const Item item)
	{
		mItem = item;
	}

	uint8_t Room::GetDepthFromStart() const
	{
		return mDepthFromStart;
	}

	void Room::SetDepthFromStart(const uint8_t depthFrcomStart)
	{
		mDepthFromStart = depthFrcomStart;
	}

	uint8_t Room::GetBranchId() const
	{
		return mBranchId;
	}

	void Room::SetBranchId(const uint8_t branchId)
	{
		mBranchId = branchId;
	}


	std::string Room::GetName() const
	{
		return std::to_string(mX) + "_" + std::to_string(mY) + "_" + std::to_string(mZ);
	}

	const std::string_view& Room::GetPartsName() const noexcept
	{
		static const std::array<std::string_view, PartsSize> names = {
			"Unidentified",
			"Start",
			"Goal",
			"Hall",
			"Hanare",
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
}
