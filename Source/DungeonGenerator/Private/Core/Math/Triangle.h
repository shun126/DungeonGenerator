/**
三角形に関するヘッダーファイル

@cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <array>

namespace dungeon
{
	class Point;

	/**
	三角形クラス
	*/
	class Triangle final
	{
	public:
		/**
		コンストラクタ
		3頂点を与えて三角形をつくる
		*/
		Triangle(const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3) noexcept;

		/**
		コンストラクタ
		3頂点を与えて三角形をつくる
		*/
		Triangle(const FVector& p1, const FVector& p2, const FVector& p3) noexcept;

		/**
		コピーコンストラクタ
		*/
		Triangle(const Triangle& other) noexcept;

		/**
		ムーブコンストラクタ
		*/
		Triangle(Triangle&& other) noexcept;

		/**
		デストラクタ
		*/
		~Triangle() = default;

		/*
		他の三角形と共有点を持つか
		@param[in]	other	比較する三角形
		@return		trueならば共通の頂点を持っている
		*/
		bool HasCommonPoints(const Triangle& other) const noexcept;

		/**
		頂点を取得します
		@param[in]	index	頂点番号
		@return		頂点
		*/
		const std::shared_ptr<const Point>& GetPoint(const size_t index) const noexcept;

		/**
		面積を取得します
		@return		面積
		*/
		double Area() const noexcept;

		/**
		右回りの面か取得します
		@return		trueならば右回りの辺
		*/
		bool IsCW() const noexcept;

		/**
		左回りの面か取得します
		@return		trueならば左回りの辺
		*/
		bool IsCCW() const noexcept;

		/**
		右回りの面に変換します
		*/
		void CW() noexcept;

		/**
		左回りの面に変換します
		*/
		void CCW() noexcept;

		/**
		コピー代入
		*/
		Triangle& operator=(const Triangle& other) noexcept;

		/**
		ムーブ代入
		*/
		Triangle& operator=(Triangle&& other) noexcept;

		/**
		同値判定
		@param[in]	other	比較する三角形
		@return		trueならば同じ
		*/
		bool operator==(const Triangle& other) const noexcept;

		/**
		異値判定
		@param[in]	other	比較する三角形
		@return		trueならば異なる
		*/
		bool operator!=(const Triangle& other) const noexcept;

		/**
		頂点を取得します
		*/
		FVector operator[](const size_t index) const noexcept;

	private:
		//! 頂点
		std::array<std::shared_ptr<const Point>, 3> mPoints;
	};
}

#include "Triangle.inl"
