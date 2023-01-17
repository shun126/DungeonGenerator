/*!
方向

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Core/Math/Random.h"
#include <array>

namespace dungeon
{
	/*!
	方向クラス
	*/
	class Direction final
	{
	public:
		/*!
		方向の番号
		PathFinder::SearchDirectionと並びを合わせて下さい
		*/
		enum Index : uint8_t
		{
			North,
			East,
			South,
			West
		};

	public:
		/*!
		コンストラクタ
		\param[in]	index	方向
		*/
		explicit Direction(const Index index = Index::North) noexcept;

		/*!
		デストラクタ
		*/
		~Direction() = default;

		/*!
		方向を取得します
		\return		方向
		*/
		const Index Get() const noexcept;

		/*!
		方向を設定します
		\param[in]	index	方向
		*/
		void Set(const Index index) noexcept;

		/*!
		方向を設定します
		\param[in]	index	方向
		*/
		Direction& operator=(const Index index) noexcept;

		/*!
		一致
		*/
		bool operator==(const Direction& other) const noexcept;

		/*!
		不一致
		*/
		bool operator!=(const Direction& other) const noexcept;

		/*!
		方向が南北に向いているか調べます
		*/
		bool IsNorthSouth() const noexcept;

		/*!
		方向が南北に向いているか調べます
		*/
		static bool IsNorthSouth(const Index index) noexcept;

		/*!
		角度を取得します
		*/
		float ToDegree() const noexcept;

		/*!
		ラジアンを取得します
		*/
		float ToRadian() const noexcept;

		/*!
		方向を反対にします
		\return		反対の方向
		*/
		Direction Inverse() const noexcept;

		/*!
		方向を反対にします
		*/
		void SetInverse() noexcept;

		/*!
		ベクターを取得します
		\return		ベクター
		*/
		const FIntVector& GetVector() const noexcept;

		/*!
		ベクターを取得します
		\param[in]	index	方向
		\return		ベクター
		*/
		static const FIntVector& GetVector(const Index index) noexcept;

		/*!
		ベクター一覧の開始イテレーターを取得します
		*/
		static std::array<FIntVector, 4>::const_iterator Begin() noexcept;

		/*!
		ベクター一覧の終了イテレーターを取得します
		*/
		static std::array<FIntVector, 4>::const_iterator End() noexcept;

		/*!
		ランダムな方向を取得します
		\return		ランダムな方向
		*/
		static Direction CreateFromRandom(Random& random) noexcept;

	private:
		Index mIndex;
	};

	inline Direction::Direction(const Index index) noexcept
		: mIndex(index)
	{
	}

	inline const Direction::Index Direction::Get() const noexcept
	{
		return mIndex;
	}

	inline void Direction::Set(const Direction::Index index) noexcept
	{
		mIndex = index;
	}

	inline Direction& Direction::operator=(const Direction::Index index) noexcept
	{
		mIndex = index;
		return *this;
	}

	inline bool Direction::operator==(const Direction& other) const noexcept
	{
		return mIndex == other.mIndex;
	}

	inline bool Direction::operator!=(const Direction& other) const noexcept
	{
		return mIndex != other.mIndex;
	}

	inline bool Direction::IsNorthSouth() const noexcept
	{
		return IsNorthSouth(mIndex);
	}

	inline bool Direction::IsNorthSouth(const Index index) noexcept
	{
		const uint8_t isNorthSouth = static_cast<uint8_t>(index) & 1;
		return isNorthSouth == 0;
	}

	inline float Direction::ToDegree() const noexcept
	{
		return static_cast<float>(mIndex) * 90.f;
	}

	inline float Direction::ToRadian() const noexcept
	{
		return static_cast<float>(mIndex) * 1.5707963267948966192313216916398f;
	}

	inline Direction Direction::Inverse() const noexcept
	{
		Direction result(mIndex);
		result.SetInverse();
		return result;
	}

	inline void Direction::SetInverse() noexcept
	{
		std::uint_fast8_t value = static_cast<std::uint_fast8_t>(mIndex);
		value += 2;
		value &= 3;
		mIndex = static_cast<Index>(value);
	}

	inline const FIntVector& Direction::GetVector() const noexcept
	{
		return GetVector(mIndex);
	}

	inline Direction Direction::CreateFromRandom(Random& random) noexcept
	{
		return Direction(static_cast<Direction::Index>(random.Get<int32_t>() % 4));
	}
}
