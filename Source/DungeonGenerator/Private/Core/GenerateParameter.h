/**
ダンジョン生成パラメータに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Core/Math/Random.h"

namespace dungeon
{
	/**
	デフォルトダンジョン生成パラメータクラス
	*/
	struct GenerateParameter final
	{
		/**
		コンストラクタ
		*/
		GenerateParameter() = default;

		/**
		デストラクタ
		*/
		~GenerateParameter() = default;

		/**
		ダンジョンの階層
		*/
		uint8_t GetNumberOfCandidateFloors() const noexcept { return mNumberOfCandidateFloors; }

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t GetNumberOfCandidateRooms() const noexcept { return mNumberOfCandidateRooms; }

		/**
		部屋の最小の幅
		*/
		uint32_t GetMinRoomWidth() const noexcept { return mMinRoomWidth; }

		/**
		部屋の最大の幅
		*/
		uint32_t GetMaxRoomWidth() const noexcept { return mMaxRoomWidth; }

		/**
		部屋の最小の奥行き
		*/
		uint32_t GetMinRoomDepth() const noexcept { return mMinRoomDepth; }

		/**
		部屋の最大の奥行き
		*/
		uint32_t GetMaxRoomDepth() const noexcept { return mMaxRoomDepth; }

		/**
		部屋の最小の高さ
		*/
		uint32_t GetMinRoomHeight() const noexcept { return mMinRoomHeight; }

		/**
		部屋の最大の高さ
		*/
		uint32_t GetMaxRoomHeight() const noexcept { return mMaxRoomHeight; }

		/**
		部屋と部屋の水平方向の余白
		*/
		uint32_t GetHorizontalRoomMargin() const noexcept { return mHorizontalRoomMargin; };

		/**
		部屋と部屋の垂直方向の余白
		*/
		uint32_t GetVerticalRoomMargin() const noexcept { return mVerticalRoomMargin; };

		/**
		乱数発生
		*/
		Random& GetRandom() const noexcept { return const_cast<GenerateParameter*>(this)->mRandom; }





		/**
		ダンジョンの幅
		*/
		uint32_t GetWidth() const noexcept { return mWidth; }

		/**
		ダンジョンの奥行き
		*/
		uint32_t GetDepth() const noexcept { return mDepth; }

		/**
		ダンジョンの高さ
		*/
		uint32_t GetHeight() const noexcept { return mHeight; }

		/**
		ダンジョンの幅
		*/
		uint32_t mWidth = 0;

		/**
		ダンジョンの奥行き
		*/
		uint32_t mDepth = 0;

		/**
		ダンジョンの高さ
		*/
		uint32_t mHeight = 3;




		/**
		生成する階層の候補
		*/
		uint8_t mNumberOfCandidateFloors = 3;

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t mNumberOfCandidateRooms = 5;

		/**
		部屋の最小の幅
		*/
		uint32_t mMinRoomWidth = 2;

		/**
		部屋の最大の幅
		*/
		uint32_t mMaxRoomWidth = 3;

		/**
		部屋の最小の奥行き
		*/
		uint32_t mMinRoomDepth = 2;

		/**
		部屋の最大の奥行き
		*/
		uint32_t mMaxRoomDepth = 3;

		/**
		部屋の最小の高さ
		*/
		uint32_t mMinRoomHeight = 1;

		/**
		部屋の最大の高さ
		*/
		uint32_t mMaxRoomHeight = 2;

		/**
		部屋と部屋の水平方向の余白
		*/
		uint32_t mHorizontalRoomMargin = 0;

		/**
		部屋と部屋の垂直方向の余白
		*/
		uint32_t mVerticalRoomMargin = 0;

		/**
		乱数生成器
		*/
		Random mRandom;
	};
}

#include "GenerateParameter.inl"
