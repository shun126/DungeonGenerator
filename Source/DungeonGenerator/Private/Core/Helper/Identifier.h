/**
識別子クラスヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <cstdint>

namespace dungeon
{
	/**
	識別子クラス
	*/
	class Identifier final
	{
	public:
		//! 識別子の型
		using IdentifierType = uint16_t;

	public:
		/**
		識別子のタイプ
		*/
		enum class Type : uint8_t
		{
			Unknown = 0,
			Room,
			Aisle
		};

	public:
		Identifier() noexcept;
		explicit Identifier(const Type type) noexcept;
		explicit Identifier(const IdentifierType other) noexcept;

		Identifier(const Identifier& other) noexcept;
		Identifier(Identifier&& other) noexcept;

		~Identifier() = default;

		Identifier& operator=(const Identifier& other) noexcept;
		Identifier& operator=(Identifier&& other) noexcept;

		bool operator==(const Identifier& other) const noexcept;
		bool operator!=(const Identifier& other) const noexcept;

		operator IdentifierType() const noexcept;

		Type GetType() const noexcept;
		bool IsType(const Type type) const noexcept;
		static bool IsType(const IdentifierType identifier, const Type type) noexcept;

		/*
		Reset counter to identify
		*/
		static void ResetCounter();

	private:
		IdentifierType mIdentifier;

		static constexpr uint8_t BitCount = 2;
		static constexpr uint8_t Shift = sizeof(mIdentifier) * 8 - BitCount;
		static constexpr IdentifierType MaskCounter = static_cast<IdentifierType>(~0) >> BitCount;
		static IdentifierType mCounter;
	};
}

#include "Identifier.inl"
