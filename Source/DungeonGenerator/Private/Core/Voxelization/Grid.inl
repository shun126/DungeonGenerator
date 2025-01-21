/**
ボクセルなどに利用するグリッド情報のヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Helper/Direction.h"
#include "../Math/Random.h"

namespace dungeon
{
	inline Grid::Grid() noexcept
	{
		SetType(Type::Empty);
		SetProps(Props::None);
		SetDirection(Direction(Direction::North));
	}

	inline Grid::Grid(const Type type) noexcept
	{
		SetType(type);
		SetProps(Props::None);
		SetDirection(Direction(Direction::North));
	}

	inline Grid::Grid(const Type type, const Direction& direction) noexcept
	{
		SetType(type);
		SetProps(Props::None);
		SetDirection(direction);
	}

	inline Grid::Grid(const Type type, const Direction& direction, const uint16_t identifier) noexcept
		: mIdentifier(identifier)
	{
		SetType(type);
		SetProps(Props::None);
		SetDirection(direction);
	}

	inline Grid Grid::CreateFloor(const std::shared_ptr<Random>& random, const uint16_t identifier) noexcept
	{
		return Grid(Type::Floor, Direction::CreateFromRandom(random), identifier);
	}

	inline Grid Grid::CreateDeck(const std::shared_ptr<Random>& random, const uint16_t identifier) noexcept
	{
		return Grid(Type::Deck, Direction::CreateFromRandom(random), identifier);
	}

	inline Grid::Type Grid::GetType() const noexcept
	{
		return mPack.GetType();
	}

	inline void Grid::SetType(const Type type) noexcept
	{
		mPack.SetType(type);
	}

	inline bool Grid::Is(const Grid::Type type) const noexcept
	{
		return GetType() == type;
	}

	inline bool Grid::IsKindOfRoomType() const noexcept
	{
		return IsKindOfRoomTypeWithoutGate() || IsKindOfGateType();
	}

	inline bool Grid::IsKindOfRoomTypeWithoutGate() const noexcept
	{
		return Is(Type::Floor) || Is(Type::Deck);
	}

	inline bool Grid::IsKindOfGateType() const noexcept
	{
		return Is(Type::Gate);
	}

	inline bool Grid::IsKindOfAisleType() const noexcept
	{
		return Is(Type::Aisle);
	}

	inline bool Grid::IsKindOfSlopeType() const noexcept
	{
		return
			Is(Type::Slope) ||
			Is(Type::Stairwell) ||
			Is(Type::DownSpace) ||
			Is(Type::UpSpace);
	}

	inline bool Grid::IsKindOfSpatialType() const noexcept
	{
		return
			Is(Type::Empty) ||
			Is(Type::OutOfBounds);
	}

	inline bool Grid::IsHorizontallyPassable() const noexcept
	{
		return IsKindOfRoomType() || IsKindOfAisleType() || Is(Type::Slope) || Is(Type::UpSpace);
	}

	inline Direction Grid::GetDirection() const noexcept
	{
		return mPack.GetDirection();
	}

	inline void Grid::SetDirection(const Direction direction) noexcept
	{
		mPack.SetDirection(direction);
	}

	inline Direction Grid::GetCatwalkDirection() const noexcept
	{
		return mPack.GetCatwalkDirection();
	}

	inline void Grid::SetCatwalkDirection(const Direction direction) noexcept
	{
		mPack.SetCatwalkDirection(direction);
	}

	inline Grid::Props Grid::GetProps() const noexcept
	{
		return mPack.GetProps();
	}

	inline void Grid::SetProps(const Props props) noexcept
	{
		mPack.SetProps(props);
	}

	inline Identifier Grid::GetIdentifier() const noexcept
	{
		return Identifier(mIdentifier);
	}

	inline void Grid::SetIdentifier(Identifier identifier) noexcept
	{
		mIdentifier = static_cast<uint16_t>(identifier);
	}

	inline bool Grid::IsInvalidIdentifier() const noexcept
	{
		return mIdentifier == InvalidIdentifier;
	}

	inline void Grid::NoRoofMeshGeneration(const bool noRoofMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoRoofMeshGeneration, noRoofMeshGeneration);
	}

	inline void Grid::NoFloorMeshGeneration(const bool noFloorMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoFloorMeshGeneration, noFloorMeshGeneration);
	}

	inline bool Grid::IsNoFloorMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoFloorMeshGeneration);
	}

	inline bool Grid::IsNoRoofMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoRoofMeshGeneration);
	}

	inline void Grid::NoNorthWallMeshGeneration(const bool noWallMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoNorthWallMeshGeneration, noWallMeshGeneration);
	}

	inline void Grid::NoSouthWallMeshGeneration(const bool noWallMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoSouthWallMeshGeneration, noWallMeshGeneration);
	}

	inline void Grid::NoEastWallMeshGeneration(const bool noWallMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoEastWallMeshGeneration, noWallMeshGeneration);
	}

	inline void Grid::NoWestWallMeshGeneration(const bool noWallMeshGeneration) noexcept
	{
		mPack.SetAttribute(Attribute::NoWestWallMeshGeneration, noWallMeshGeneration);
	}

	inline bool Grid::IsNoWallMeshGeneration() const noexcept
	{
		return IsNoNorthWallMeshGeneration() || IsNoSouthWallMeshGeneration() || IsNoEastWallMeshGeneration() || IsNoWestWallMeshGeneration();
	}

	inline bool Grid::IsNoWallMeshGeneration(const Direction direction) const noexcept
	{
		return IsNoWallMeshGeneration(direction.Get());
	}

	inline bool Grid::IsNoWallMeshGeneration(const Direction::Index direction) const noexcept
	{
		const auto attribute = static_cast<uint8_t>(Attribute::NoNorthWallMeshGeneration) + direction;
		return mPack.IsAttributeEnabled(static_cast<Attribute>(attribute));
	}

	inline bool Grid::IsNoNorthWallMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoNorthWallMeshGeneration);
	}

	inline bool Grid::IsNoSouthWallMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoSouthWallMeshGeneration);
	}

	inline bool Grid::IsNoEastWallMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoEastWallMeshGeneration);
	}

	inline bool Grid::IsNoWestWallMeshGeneration() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::NoWestWallMeshGeneration);
	}

	inline void Grid::MergeAisle(const bool enable) noexcept
	{
		mPack.SetAttribute(Attribute::MergeAisle, enable);
	}

	inline bool Grid::CanMergeAisle() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::MergeAisle);
	}

	inline void Grid::Catwalk(const bool enable) noexcept
	{
		mPack.SetAttribute(Attribute::Catwalk, enable);
	}

	inline bool Grid::IsCatwalk() const noexcept
	{
		return mPack.IsAttributeEnabled(Attribute::Catwalk);
	}



	inline Grid::Pack::Pack()
	{
		constexpr size_t DataBitSize = BitCount(DirectionSize) + TypeSize + PropsSize + AttributeSize;
		constexpr size_t PackBitSize = sizeof(ValueType) * 8;
		static_assert(DataBitSize <= PackBitSize);
	}

	inline Direction Grid::Pack::GetDirection() const noexcept
	{
		constexpr ValueType shift = 0;
		constexpr ValueType count = BitCount(DirectionSize);
		constexpr ValueType mask = Mask(count, shift);
		return Direction(static_cast<Direction::Index>(GetValue<int16_t>(shift, mask)));
	}
	inline void Grid::Pack::SetDirection(const Direction direction) noexcept
	{
		constexpr ValueType shift = 0;
		constexpr ValueType count = BitCount(DirectionSize);
		constexpr ValueType mask = ~Mask(count, shift);
		SetValue<int16_t>(shift, mask, static_cast<ValueType>(direction.Get()));
	}

	inline Direction Grid::Pack::GetCatwalkDirection() const noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize);
		constexpr ValueType count = BitCount(DirectionSize);
		constexpr ValueType mask = Mask(count, shift);
		return Direction(static_cast<Direction::Index>(GetValue<int16_t>(shift, mask)));
	}
	inline void Grid::Pack::SetCatwalkDirection(const Direction direction) noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize);
		constexpr ValueType count = BitCount(DirectionSize);
		constexpr ValueType mask = ~Mask(count, shift);
		SetValue<int16_t>(shift, mask, static_cast<ValueType>(direction.Get()));
	}

	inline Grid::Type Grid::Pack::GetType() const noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2;
		constexpr ValueType count = BitCount(TypeSize);
		constexpr ValueType mask = Mask(count, shift);
		return GetValue<Type>(shift, mask);
	}
	inline void Grid::Pack::SetType(const Type type) noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2;
		constexpr ValueType count = BitCount(TypeSize);
		constexpr ValueType mask = ~Mask(count, shift);
		SetValue<Type>(shift, mask, type);
	}

	inline Grid::Props Grid::Pack::GetProps() const noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2 + BitCount(TypeSize);
		constexpr ValueType count = BitCount(PropsSize);
		constexpr ValueType mask = Mask(count, shift);
		return GetValue<Props>(shift, mask);
	}
	inline void Grid::Pack::SetProps(const Props props) noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2 + BitCount(TypeSize);
		constexpr ValueType count = BitCount(PropsSize);
		constexpr ValueType mask = ~Mask(count, shift);
		SetValue<Props>(shift, mask, props);
	}

	inline bool Grid::Pack::IsAttributeEnabled(const Attribute attribute) const noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2 + BitCount(TypeSize) + BitCount(PropsSize);
		return Test(shift + static_cast<ValueType>(attribute));
	}
	inline void Grid::Pack::SetAttribute(const Attribute attribute, const bool enable) noexcept
	{
		constexpr ValueType shift = BitCount(DirectionSize) * 2 + BitCount(TypeSize) + BitCount(PropsSize);
		Set(shift + static_cast<ValueType>(attribute), enable);
	}
}
