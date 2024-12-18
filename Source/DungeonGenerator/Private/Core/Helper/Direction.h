/**
方向

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/IntVector.h>
#include <array>
#include <memory>

namespace dungeon
{
	class Random;

	/**
	方向クラス
	*/
	class Direction final
	{
	public:
		/**
		方向の番号
		Grid::Attributeと並びを合わせて下さい
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
		/**
		コンストラクタ
		@param[in]	index	方向
		*/
		explicit Direction(const Index index = Index::North) noexcept;

		/**
		コピーコンストラクタ
		@param[in]	other	方向
		*/
		Direction(const Direction& other) noexcept;

		/**
		デストラクタ
		*/
		~Direction() = default;

		/**
		方向を取得します
		@return		方向
		*/
		Index Get() const noexcept;

		/**
		方向を設定します
		@param[in]	index	方向
		*/
		void Set(const Index index) noexcept;

		/**
		方向を設定します
		@param[in]	index	方向
		*/
		Direction& operator=(const Index index) noexcept;

		/**
		方向を代入します
		@param[in]	other	代入するDirection
		*/
		Direction& operator=(const Direction& other) noexcept;

		/**
		一致
		*/
		bool operator==(const Direction& other) const noexcept;

		/**
		不一致
		*/
		bool operator!=(const Direction& other) const noexcept;

		/**
		方向が南北に向いているか調べます
		*/
		bool IsNorthSouth() const noexcept;

		/**
		方向が南北に向いているか調べます
		*/
		static bool IsNorthSouth(const Index index) noexcept;

		/**
		角度を取得します
		*/
		float ToDegree() const noexcept;

		/**
		ラジアンを取得します
		*/
		float ToRadian() const noexcept;

		/**
		方向を反対にします
		@return		反対の方向
		*/
		Direction Inverse() const noexcept;

		/**
		方向を反対にします
		*/
		void SetInverse() noexcept;

		/**
		ベクターを取得します
		@return		ベクター
		*/
		const FIntVector& GetVector() const noexcept;

		/**
		ベクターを取得します
		@param[in]	index	方向
		@return		ベクター
		*/
		static const FIntVector& GetVector(const Index index) noexcept;

		/**
		ベクター一覧の開始イテレーターを取得します
		*/
		static std::array<FIntVector, 4>::const_iterator Begin() noexcept;

		/**
		ベクター一覧の終了イテレーターを取得します
		*/
		static std::array<FIntVector, 4>::const_iterator End() noexcept;

		/**
		ランダムな方向を取得します
		@return		ランダムな方向
		*/
		static Direction CreateFromRandom(const std::shared_ptr<Random>& random) noexcept;

#if WITH_EDITOR
		/**
		方向名を取得します
		*/
		const FString& GetName() const noexcept;
#endif

	private:
		Index mIndex;
	};
}

#include "Direction.inl"
