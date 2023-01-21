/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <cassert>
#include <cstdint>
#include <ctime>
#include <limits>

namespace dungeon
{
	/*!
	乱数クラス
	*/
	class Random
	{
	public:
		//! コンストラクタ
		Random();

		/*!
		コンストラクタ
		\param[in]	seed	乱数の種
		*/
		explicit Random(const uint32_t seed);

		/*!
		コピーコンストラクタ
		*/
		explicit Random(const Random& other);

		/*!
		ムーブコンストラクタ
		*/
		explicit Random(Random&& other) noexcept;

		/*!
		コピー代入
		*/
		Random& operator = (const Random& other);

		/*!
		ムーブ代入
		*/
		Random& operator = (Random&& other) noexcept;

		/*!
		乱数の種を設定します
		\param[in]	seed	乱数の種
		*/
		void SetSeed(const uint32_t seed);

		/*!
		符号の乱数を取得します
		\return	-1または1
		*/
		template <typename T>
		T GetSign();

		/*!
		乱数を取得します
		\return		整数では[type_min,type_max]、実数では[0,1]の範囲を等確率で返す
		*/
		template <typename T>
		T Get();

		/*!
		乱数を取得します
		\param[in]	to	範囲
		\return		整数では[0,to)、実数では[0,to]の範囲を等確率で返す
		*/
		template <typename T>
		T Get(const T to);

		/*!
		乱数を取得します
		\param[in]	from	開始範囲
		\param[in]	to		終了範囲
		\return		整数では[from,to)、実数では[from,to]の範囲を等確率で返す
		*/
		template <typename T>
		T Get(const T from, const T to);

		/*!
		インスタンスを取得します
		*/
		static Random& Instance()
		{
			static Random instance;
			return instance;
		}

	private:
		/*!
		uint32_t型の乱数を取得します
		\return		[0,std::numeric_limits<int32_t>::max]の範囲を返す
		*/
		uint32_t GetU32();

	private:
		uint32_t mX = 123456789;
		uint32_t mY = 362436069;
		uint32_t mZ = 521288629;
		uint32_t mW = 88675123;
	};

	/*
	constexpr if内のブロックの評価を遅延させるためのテンプレート
	*/
	template <typename T>
	constexpr bool RandamFalse_v = false;

	inline Random::Random()
	{
		SetSeed(static_cast<uint32_t>(std::time(nullptr)));
	}

	inline Random::Random(const uint32_t seed)
	{
		SetSeed(seed);
	}

	inline Random::Random(const Random& other)
		: mX(other.mX)
		, mY(other.mY)
		, mZ(other.mZ)
		, mW(other.mW)
	{
	}

	inline Random::Random(Random&& other) noexcept
		: mX(std::move(other.mX))
		, mY(std::move(other.mY))
		, mZ(std::move(other.mZ))
		, mW(std::move(other.mW))
	{
	}

	inline Random& Random::operator = (const Random& other)
	{
		mX = other.mX;
		mY = other.mY;
		mZ = other.mZ;
		mW = other.mW;
		return *this;
	}

	inline Random& Random::operator = (Random&& other) noexcept
	{
		mX = std::move(other.mX);
		mY = std::move(other.mY);
		mZ = std::move(other.mZ);
		mW = std::move(other.mW);
		return *this;
	}

	inline void Random::SetSeed(const uint32_t seed)
	{
		mX = 123456789;
		mY = 362436069;
		mZ = 521288629;
		mW = 88675123;

		// 全て0にならないようにする
		do
		{
			mX ^= seed; mX ^= mX >> 21;	mX ^= mX >> 4; mX *= 1332534557;
			mY ^= seed; mY ^= mY >> 21;	mY ^= mY >> 4; mY *= 1332534557;
			mZ ^= seed; mZ ^= mZ >> 21;	mZ ^= mZ >> 4; mZ *= 1332534557;
			mW ^= seed; mW ^= mW >> 21;	mW ^= mW >> 4; mW *= 1332534557;
		} while (mX == 0 && mY == 0 && mZ == 0 && mW == 0);
	}

	template<typename T>
	inline T Random::GetSign()
	{
		static_assert(std::is_signed<T>::value == true, "T must be signed type");
		return static_cast<T>(GetU32() & 0x2) - 1;
	}

	template<typename T>
	inline T Random::Get()
	{
		if constexpr (std::is_same<T, double>())
		{
			static const double INVERT = 1. / static_cast<double>(0xFFFFFFFF);
			return static_cast<double>(GetU32()) * INVERT;
		}
		else if constexpr (std::is_same<T, float>())
		{
			static const float INVERT = 1.f / static_cast<float>(0xFFFFFFFF);
			return static_cast<float>(GetU32()) * INVERT;
		}
		else if constexpr (std::is_same<T, uint64_t>())
		{
			const uint64_t hight = static_cast<uint64_t>(GetU32()) << 32ULL;
			const uint64_t low = static_cast<uint64_t>(GetU32());
			return hight | low;
		}
		else if constexpr (std::is_same<T, int64_t>())
		{
			const uint64_t hight = static_cast<uint64_t>(GetU32()) << 32ULL;
			const uint64_t low = static_cast<uint64_t>(GetU32());
			return static_cast<int64_t>(hight | low);
		}
		else if constexpr (std::is_same<T, bool>())
		{
			// 線形合同法と異なり、XorShiftは最下位ビットの短周期の規則性はない。
			// 0または1を直接得ることができるため、最下位ビットを取る実装としている。
			return static_cast<bool>(GetU32() & 0x1);
		}
		else if constexpr (std::is_integral<T>::value)
		{
			return static_cast<T>(GetU32());
		}
		else
		{
			static_assert(RandamFalse_v<T>, "Type T is unsupported type");
		}
	}

	template <typename T>
	inline T Random::Get(const T to)
	{
		if constexpr (std::is_floating_point<T>::value)
		{
			return Get<T>() * to;
		}
		else
		{
			return Get<T>() % to;
		}
	}

	template <typename T>
	inline T Random::Get(const T from, const T to)
	{
		check(from <= to);
		if constexpr (std::is_floating_point<T>::value)
		{
			return from + Get<T>() * (to - from);
		}
		else
		{
			return from + Get<T>() % (to - from);
		}
	}

	inline uint32_t Random::GetU32()
	{
		const uint32_t t = (mX ^ (mX << 11));
		mX = mY;
		mY = mZ;
		mZ = mW;
		mW = (mW ^ (mW >> 19)) ^ (t ^ (t >> 8));
		return mW;
	}
}
