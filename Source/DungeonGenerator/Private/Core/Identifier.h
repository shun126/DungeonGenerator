/**
識別子クラスヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once

namespace dungeon
{
	/**
	識別子クラス
	*/
	class Identifier final
	{
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

		explicit Identifier(const Identifier& other) noexcept;
		Identifier(Identifier&& other) noexcept;

		~Identifier() = default;

		Identifier& operator=(const Identifier& other) noexcept;
		Identifier& operator=(Identifier&& other) noexcept;

		bool operator==(const Identifier& other) const noexcept;
		bool operator!=(const Identifier& other) const noexcept;

		Type GetType() const noexcept;
		bool IsType(const Type type) const noexcept;

		// TODO:移行が完了したら削除して下さい
		uint16_t Get() const noexcept { return mIdentifier; }

	private:
		IdentifierType mIdentifier;

		static constexpr uint8_t bitCount = 2;
		static constexpr uint8_t shift = sizeof(mIdentifier) * 8 - bitCount;
		static constexpr IdentifierType maskCounter = static_cast<IdentifierType>(~0) >> bitCount;
		static IdentifierType mCounter;
	};
}

#include "Identifier.inl"
