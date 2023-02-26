/**
識別子クラスヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	inline Identifier::Identifier() noexcept
		: Identifier(Type::Unknown)
	{
	}

	inline Identifier::Identifier(const Identifier& other) noexcept
		: mIdentifier(other.mIdentifier)
	{
	}

	inline Identifier::Identifier(Identifier&& other) noexcept
		: mIdentifier(std::move(other.mIdentifier))
	{
	}

	inline Identifier& Identifier::operator=(const Identifier& other) noexcept
	{
		mIdentifier = other.mIdentifier;
		return *this;
	}

	inline Identifier& Identifier::operator=(Identifier&& other) noexcept
	{
		mIdentifier = std::move(other.mIdentifier);
		return *this;
	}

	inline bool Identifier::operator==(const Identifier& other) const noexcept
	{
		return mIdentifier == other.mIdentifier;
	}

	inline bool Identifier::operator!=(const Identifier& other) const noexcept
	{
		return mIdentifier != other.mIdentifier;
	}

	inline Identifier::Type Identifier::GetType() const noexcept
	{
		const uint8_t type = mIdentifier >> shift;
		return static_cast<Type>(type);
	}

	inline bool Identifier::IsType(const Type type) const noexcept
	{
		return type == GetType();
	}
}
