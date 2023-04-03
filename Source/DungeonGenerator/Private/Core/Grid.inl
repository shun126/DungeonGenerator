/**
ボクセルなどに利用するグリッド情報のヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Core/Math/Random.h"
#include "Direction.h"

namespace dungeon
{
	inline Grid::Grid() noexcept
		: mType(Type::Empty)
		, mProps(Props::None)
		, mDirection(Direction(Direction::North))
	{
	}

	inline Grid::Grid(const Type type) noexcept
		: mType(type)
		, mProps(Props::None)
		, mDirection(Direction(Direction::North))
	{
	}

	inline Grid::Grid(const Type type, const Direction direction) noexcept
		: mType(type)
		, mProps(Props::None)
		, mDirection(direction)
	{
	}

	inline Grid::Grid(const Type type, const Direction direction, const uint16_t identifier) noexcept
		: mType(type)
		, mProps(Props::None)
		, mDirection(direction)
		, mIdentifier(identifier)
	{
	}

	inline Grid Grid::CreateFloor(Random& random, const uint16_t identifier) noexcept
	{
		return Grid(Type::Floor, Direction::CreateFromRandom(random), identifier);
	}

	inline Grid Grid::CreateDeck(Random& random, const uint16_t identifier) noexcept
	{
		return Grid(Type::Deck, Direction::CreateFromRandom(random), identifier);
	}

	inline Grid::Type Grid::GetType() const noexcept
	{
		return mType;
	}

	inline void Grid::SetType(const Type type) noexcept
	{
		mType = type;
	}

	inline Direction Grid::GetDirection() const noexcept
	{
		return mDirection;
	}

	inline void Grid::SetDirection(const Direction direction) noexcept
	{
		mDirection = direction;
	}

	inline uint16_t Grid::GetIdentifier() const noexcept
	{
		return mIdentifier;
	}

	inline void Grid::SetIdentifier(const uint16_t identifier) noexcept
	{
		mIdentifier = identifier;
	}

	inline bool Grid::IsInvalidIdentifier() const noexcept
	{
		return mIdentifier == InvalidIdentifier;
	}

	inline Grid::Props Grid::GetProps() const noexcept
	{
		return mProps;
	}

	inline void Grid::SetProps(const Props props) noexcept
	{
		mProps = props;
	}

	inline void Grid::SetNoMeshGeneration(const bool noRoofMeshGeneration, const bool noFloorMeshGeneration)
	{
		mNoMeshGeneration.set(static_cast<uint8_t>(NoMeshGeneration::Roof), noRoofMeshGeneration);
		mNoMeshGeneration.set(static_cast<uint8_t>(NoMeshGeneration::Floor), noFloorMeshGeneration);
	}

	inline bool Grid::IsNoFloorMeshGeneration() const noexcept
	{
		return mNoMeshGeneration.test(static_cast<uint8_t>(NoMeshGeneration::Floor));
	}

	inline bool Grid::IsNoRoofMeshGeneration() const noexcept
	{
		return mNoMeshGeneration.test(static_cast<uint8_t>(NoMeshGeneration::Roof));
	}
}
