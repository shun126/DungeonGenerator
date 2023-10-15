/**
ダンジョン生成パラメータに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <memory>

namespace dungeon
{
	class Random;

	/**
	デフォルトダンジョン生成パラメータクラス
	*/
	struct GenerateParameter final
	{
		/**
		コンストラクタ
		*/
		GenerateParameter();

		/**
		デストラクタ
		*/
		~GenerateParameter() = default;

		/**
		ダンジョンの階層の候補
		部屋の初期生成数であり、最終的に生成される階層の数ではありません。
		*/
		uint8_t GetNumberOfCandidateFloors() const noexcept;

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t GetNumberOfCandidateRooms() const noexcept;

		/**
		部屋の最小の幅
		*/
		uint32_t GetMinRoomWidth() const noexcept;

		/**
		部屋の最大の幅
		*/
		uint32_t GetMaxRoomWidth() const noexcept;

		/**
		部屋の最小の奥行き
		*/
		uint32_t GetMinRoomDepth() const noexcept;

		/**
		部屋の最大の奥行き
		*/
		uint32_t GetMaxRoomDepth() const noexcept;

		/**
		部屋の最小の高さ
		*/
		uint32_t GetMinRoomHeight() const noexcept;

		/**
		部屋の最大の高さ
		*/
		uint32_t GetMaxRoomHeight() const noexcept;

		/**
		部屋と部屋の水平方向の余白
		*/
		uint32_t GetHorizontalRoomMargin() const noexcept;

		/**
		部屋と部屋の垂直方向の余白
		*/
		uint32_t GetVerticalRoomMargin() const noexcept;

		/**
		乱数発生
		*/
		std::shared_ptr<Random> GetRandom() noexcept;

		/**
		乱数発生
		*/
		std::shared_ptr<Random> GetRandom() const noexcept;




		/**
		ダンジョンの幅
		*/
		uint32_t GetWidth() const noexcept;

		/**
		ダンジョンの奥行き
		*/
		uint32_t GetDepth() const noexcept;

		/**
		ダンジョンの高さ
		*/
		uint32_t GetHeight() const noexcept;


		bool IsGenerateStartRoomReserved() const noexcept;
		bool IsGenerateGoalRoomReserved() const noexcept;




		/**
		ダンジョンの幅
		*/
		uint32_t mWidth;

		/**
		ダンジョンの奥行き
		*/
		uint32_t mDepth;

		/**
		ダンジョンの高さ
		*/
		uint32_t mHeight;



		/**
		生成する階層の数の候補
		最終的に生成される階層の数ではありません。
		*/
		uint8_t mNumberOfCandidateFloors;

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t mNumberOfCandidateRooms;

		/**
		部屋の最小の幅
		*/
		uint32_t mMinRoomWidth;

		/**
		部屋の最大の幅
		*/
		uint32_t mMaxRoomWidth;

		/**
		部屋の最小の奥行き
		*/
		uint32_t mMinRoomDepth;

		/**
		部屋の最大の奥行き
		*/
		uint32_t mMaxRoomDepth;

		/**
		部屋の最小の高さ
		*/
		uint32_t mMinRoomHeight;

		/**
		部屋の最大の高さ
		*/
		uint32_t mMaxRoomHeight;

		/**
		部屋と部屋の水平方向の余白
		*/
		uint32_t mHorizontalRoomMargin;

		/**
		部屋と部屋の垂直方向の余白
		*/
		uint32_t mVerticalRoomMargin;

		/**
		乱数生成器
		*/
		std::shared_ptr<Random> mRandom;

		FIntVector mStartRoomSize = { 0, 0, 0 };
		FIntVector mGoalRoomSize = { 0, 0, 0 };
	};
}

#include "GenerateParameter.inl"
