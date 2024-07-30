/**
ダンジョン生成パラメータに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/IntVector.h>
#include <memory>

namespace dungeon
{
	class Random;

	/**
	デフォルトダンジョン生成パラメータクラス
	*/
	struct GenerateParameter final
	{
	public:
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
		void SetNumberOfCandidateFloors(const uint8_t count) noexcept;

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t GetNumberOfCandidateRooms() const noexcept;
		void SetNumberOfCandidateRooms(const uint8_t count) noexcept;

		/**
		部屋の最小の幅
		*/
		uint32_t GetMinRoomWidth() const noexcept;
		void SetMinRoomWidth(const uint32_t width) noexcept;

		/**
		部屋の最大の幅
		*/
		uint32_t GetMaxRoomWidth() const noexcept;
		void SetMaxRoomWidth(const uint32_t width) noexcept;

		/**
		部屋の最小の奥行き
		*/
		uint32_t GetMinRoomDepth() const noexcept;
		void SetMinRoomDepth(const uint32_t depth) noexcept;

		/**
		部屋の最大の奥行き
		*/
		uint32_t GetMaxRoomDepth() const noexcept;
		void SetMaxRoomDepth(const uint32_t depth) noexcept;

		/**
		部屋の最小の高さ
		*/
		uint32_t GetMinRoomHeight() const noexcept;
		void SetMinRoomHeight(const uint32_t height) noexcept;

		/**
		部屋の最大の高さ
		*/
		uint32_t GetMaxRoomHeight() const noexcept;
		void SetMaxRoomHeight(const uint32_t height) noexcept;

		/**
		部屋と部屋の水平方向の余白
		*/
		uint32_t GetHorizontalRoomMargin() const noexcept;
		void SetHorizontalRoomMargin(const uint32_t margin) noexcept;

		/**
		部屋と部屋の垂直方向の余白
		*/
		uint32_t GetVerticalRoomMargin() const noexcept;
		void SetVerticalRoomMargin(const uint32_t margin) noexcept;

		/*
		部屋と部屋を結合するか？
		*/
		bool IsMergeRooms() const noexcept;
		void SetMergeRooms(const bool mergeRooms) noexcept;

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
		ダンジョンの幅
		*/
		void SetWidth(const uint32_t width) noexcept;

		/**
		ダンジョンの奥行き
		*/
		uint32_t GetDepth() const noexcept;

		/**
		ダンジョンの奥行き
		*/
		void SetDepth(const uint32_t depth) noexcept;

		/**
		ダンジョンの高さ
		*/
		uint32_t GetHeight() const noexcept;

		/**
		ダンジョンの高さ
		*/
		void SetHeight(const uint32_t height) noexcept;

		bool IsGenerateStartRoomReserved() const noexcept;
		bool IsGenerateGoalRoomReserved() const noexcept;

		bool UseMissionGraph() const noexcept;
		void SetMissionGraph(const bool use) noexcept;

		/*
		通路の複雑さを取得します（追加する通路の数）
		Aisle complexity (0 being the minimum aisle)
		*/
		uint8_t GetAisleComplexity() const noexcept;
		void SetAisleComplexity(const uint8_t complexity) noexcept;
		bool IsAisleComplexity() const noexcept;

		/*
		スタート部屋のサイズ
		*/
		const FIntVector& GetStartRoomSize() const noexcept;

		/*
		スタート部屋のサイズ
		*/
		void SetStartRoomSize(const FIntVector& size) noexcept;

		/*
		ゴール部屋のサイズ
		*/
		const FIntVector& GetGoalRoomSize() const noexcept;

		/*
		ゴール部屋のサイズ
		*/
		void SetGoalRoomSize(const FIntVector& size) noexcept;

	private:

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
		uint32_t mHeight = 0;



		/**
		生成する階層の数の候補
		最終的に生成される階層の数ではありません。
		*/
		uint8_t mNumberOfCandidateFloors = 0;

		/**
		生成する部屋の数の候補
		部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
		*/
		uint8_t mNumberOfCandidateRooms = 1;

		/*
		Horizontal room-to-room coupling

		有効にすると部屋と部屋を結合します
		*/
		bool mMergeRooms = false;

		/**
		MissionGraphを有効化
		*/
		bool mUseMissionGraph = true;

		/*
		通路の複雑さ（追加する通路の数）
		Aisle complexity (0 being the minimum aisle)
		*/
		uint8_t mAisleComplexity = 0;

		/**
		部屋の最小の幅
		*/
		uint32_t mMinRoomWidth = 0;

		/**
		部屋の最大の幅
		*/
		uint32_t mMaxRoomWidth = 0;

		/**
		部屋の最小の奥行き
		*/
		uint32_t mMinRoomDepth = 0;

		/**
		部屋の最大の奥行き
		*/
		uint32_t mMaxRoomDepth = 0;

		/**
		部屋の最小の高さ
		*/
		uint32_t mMinRoomHeight = 0;

		/**
		部屋の最大の高さ
		*/
		uint32_t mMaxRoomHeight = 0;

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
		std::shared_ptr<Random> mRandom;

		/*
		スタート部屋のサイズ
		*/
		FIntVector mStartRoomSize = { 0, 0, 0 };

		/*
		ゴール部屋のサイズ
		*/
		FIntVector mGoalRoomSize = { 0, 0, 0 };
	};
}

#include "GenerateParameter.inl"
