/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

namespace dungeon
{
	/**
	乱数クラス
	*/
	class Random
	{
	public:
		//! コンストラクタ
		Random();

		/**
		コンストラクタ
		\param[in]	seed	乱数の種
		*/
		explicit Random(const uint32_t seed);

		/**
		コピーコンストラクタ
		*/
		explicit Random(const Random& other);

		/**
		ムーブコンストラクタ
		*/
		explicit Random(Random&& other) noexcept;

		/**
		コピー代入
		*/
		Random& operator = (const Random& other);

		/**
		ムーブ代入
		*/
		Random& operator = (Random&& other) noexcept;

		/**
		乱数の種を設定します
		\param[in]	seed	乱数の種
		*/
		void SetSeed(const uint32_t seed);

		/**
		符号の乱数を取得します
		\return	-1または1
		*/
		template <typename T>
		T GetSign();

		/**
		乱数を取得します
		\return		整数では[type_min,type_max]、実数では[0,1]の範囲を等確率で返す
		*/
		template <typename T>
		T Get();

		/**
		乱数を取得します
		\param[in]	to	範囲
		\return		整数では[0,to)、実数では[0,to]の範囲を等確率で返す
		*/
		template <typename T>
		T Get(const T to);

		/**
		乱数を取得します
		\param[in]	from	開始範囲
		\param[in]	to		終了範囲
		\return		整数では[from,to)、実数では[from,to]の範囲を等確率で返す
		*/
		template <typename T>
		T Get(const T from, const T to);

		/**
		インスタンスを取得します
		*/
		static Random& Instance()
		{
			static Random instance;
			return instance;
		}

	private:
		/**
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
}

#include "Random.inl"
