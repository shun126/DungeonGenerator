/*!
ダンジョン生成パラメータに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Core/Math/Random.h"

namespace dungeon
{
	/*!
	デフォルトダンジョン生成パラメータクラス
	*/
	struct GenerateParameter final
	{
		/*!
		コンストラクタ
		*/
		GenerateParameter() = default;

		/*!
		デストラクタ
		*/
		~GenerateParameter() = default;

		/*!
		ダンジョンの幅
		*/
		uint32_t GetWidth() const noexcept { return mWidth; }

		/*!
		ダンジョンの奥行き
		*/
		uint32_t GetDepth() const noexcept { return mDepth; }

		/*!
		ダンジョンの高さ
		*/
		uint32_t GetHeight() const noexcept { return mHeight; }

		/*!
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint32_t GetNumberOfCandidateRooms() const noexcept { return mNumberOfCandidateRooms; }

		/*!
		部屋の最小の幅
		*/
		uint32_t GetMinRoomWidth() const noexcept { return mMinRoomWidth; }

		/*!
		部屋の最大の幅
		*/
		uint32_t GetMaxRoomWidth() const noexcept { return mMaxRoomWidth; }

		/*!
		部屋の最小の奥行き
		*/
		uint32_t GetMinRoomDepth() const noexcept { return mMinRoomDepth; }

		/*!
		部屋の最大の奥行き
		*/
		uint32_t GetMaxRoomDepth() const noexcept { return mMaxRoomDepth; }

		/*!
		部屋の最小の高さ
		*/
		uint32_t GetMinRoomHeight() const noexcept { return mMinRoomHeight; }

		/*!
		部屋の最大の高さ
		*/
		uint32_t GetMaxRoomHeight() const noexcept { return mMaxRoomHeight; }

		/*!
		床下の高さ
		*/
		uint16_t GetUnderfloorHeight() const noexcept { return mUnderfloorHeight; }

		/*!
		部屋と部屋の余白
		*/
		uint32_t GetRoomMargin() const noexcept { return mRoomMargin; };

		/*!
		乱数発生
		*/
		Random& GetRandom() const noexcept { return const_cast<GenerateParameter*>(this)->mRandom; }



		/*!
		ダンジョンの幅
		*/
		uint32_t mWidth = 0;

		/*!
		ダンジョンの奥行き
		*/
		uint32_t mDepth = 0;

		/*!
		ダンジョンの高さ
		*/
		uint32_t mHeight = 0;

		/*!
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint32_t mNumberOfCandidateRooms = 5;

		/*!
		部屋の最小の幅
		*/
		uint32_t mMinRoomWidth = 2;

		/*!
		部屋の最大の幅
		*/
		uint32_t mMaxRoomWidth = 3;

		/*!
		部屋の最小の奥行き
		*/
		uint32_t mMinRoomDepth = 2;

		/*!
		部屋の最大の奥行き
		*/
		uint32_t mMaxRoomDepth = 3;

		/*!
		部屋の最小の高さ
		*/
		uint32_t mMinRoomHeight = 1;

		/*!
		部屋の最大の高さ
		*/
		uint32_t mMaxRoomHeight = 2;

		/*!
		部屋と部屋の余白
		*/
		uint32_t mRoomMargin = 0;

		/*!
		床下の高さ
		*/
		uint16_t mUnderfloorHeight = 0;

		/*!
		乱数生成器
		*/
		Random mRandom;
	};
}

#include "GenerateParameter.inl"
