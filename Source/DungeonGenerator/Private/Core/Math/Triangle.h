/*!
三角形に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once
#include <array>

namespace dungeon
{
	class Point;

	/*!
	三角形クラス
	*/
	class Triangle final
	{
	public:
		/*!
		コンストラクタ
		3頂点を与えて三角形をつくる
		*/
		Triangle(const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3) noexcept;

		/*!
		コンストラクタ
		3頂点を与えて三角形をつくる
		*/
		Triangle(const FVector& p1, const FVector& p2, const FVector& p3) noexcept;

		/*!
		コピーコンストラクタ
		*/
		Triangle(const Triangle& other) noexcept;

		/*!
		ムーブコンストラクタ
		*/
		Triangle(Triangle&& other) noexcept;

		/*!
		デストラクタ
		*/
		~Triangle() = default;

		/*
		他の三角形と共有点を持つか
		\param[in]	other	比較する三角形
		\return		trueならば共通の頂点を持っている
		*/
		bool HasCommonPoints(const Triangle& other) const noexcept;

		/*!
		頂点を取得します
		\param[in]	index	頂点番号
		\return		頂点
		*/
		const std::shared_ptr<const Point>& GetPoint(const size_t index) const noexcept;

		/*!
		面積を取得します
		\return		面積
		*/
		double Area() const noexcept;

		/*!
		右回りの面か取得します
		\return		trueならば右回りの辺
		*/
		bool IsCW() const noexcept;

		/*!
		左回りの面か取得します
		\return		trueならば左回りの辺
		*/
		bool IsCCW() const noexcept;

		/*!
		右回りの面に変換します
		*/
		void CW() noexcept;

		/*!
		左回りの面に変換します
		*/
		void CCW() noexcept;

		/*!
		コピー代入
		*/
		Triangle& operator=(const Triangle& other) noexcept;

		/*!
		ムーブ代入
		*/
		Triangle& operator=(Triangle&& other) noexcept;

		/*!
		同値判定
		\param[in]	other	比較する三角形
		\return		trueならば同じ
		*/
		bool operator==(const Triangle& other) const noexcept;

		/*!
		異値判定
		\param[in]	other	比較する三角形
		\return		trueならば異なる
		*/
		bool operator!=(const Triangle& other) const noexcept;

		/*!
		頂点を取得します
		*/
		FVector operator[](const size_t index) const noexcept;

	private:
		//! 頂点
		std::array<std::shared_ptr<const Point>, 3> mPoints;
	};
}

#include "Point.h"

namespace dungeon
{
	inline Triangle::Triangle(const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3) noexcept
		: mPoints{ { p1, p2, p3 } }
	{
	}

	inline Triangle::Triangle(const FVector& p1, const FVector& p2, const FVector& p3) noexcept
		: mPoints{ {
				std::make_shared<const Point>(p1),
				std::make_shared<const Point>(p2),
				std::make_shared<const Point>(p3)
			} }
	{
	}

	inline Triangle::Triangle(const Triangle& other) noexcept
		: mPoints(other.mPoints)
	{
	}

	inline Triangle::Triangle(Triangle&& other) noexcept
		: mPoints(std::move(other.mPoints))
	{
	}

	inline Triangle& Triangle::operator=(const Triangle& other) noexcept
	{
		mPoints = other.mPoints;
		return *this;
	}

	inline Triangle& Triangle::operator=(Triangle&& other) noexcept
	{
		mPoints = std::move(other.mPoints);
		return *this;
	}

	inline bool Triangle::HasCommonPoints(const Triangle& other) const noexcept
	{
		return
			mPoints[0] == other.mPoints[0] || mPoints[0] == other.mPoints[1] || mPoints[0] == other.mPoints[2] ||
			mPoints[1] == other.mPoints[0] || mPoints[1] == other.mPoints[1] || mPoints[1] == other.mPoints[2] ||
			mPoints[2] == other.mPoints[0] || mPoints[2] == other.mPoints[1] || mPoints[2] == other.mPoints[2];
	}

	inline const std::shared_ptr<const Point>& Triangle::GetPoint(const size_t index) const noexcept
	{
		return mPoints.at(index);
	}

	inline double Triangle::Area() const noexcept
	{
		const double dx1 = mPoints[1]->X - mPoints[0]->X;
		const double dy1 = mPoints[1]->Y - mPoints[0]->Y;
		const double dx2 = mPoints[2]->X - mPoints[0]->X;
		const double dy2 = mPoints[2]->Y - mPoints[0]->Y;
		const double s = std::abs(dx1 * dy2 - dy1 * dx2) * 0.5f;
		return s;
	}

	inline bool Triangle::IsCW() const noexcept
	{
		const FVector e2(*mPoints[1] - *mPoints[0]);
		const FVector e3(*mPoints[2] - *mPoints[0]);
		const double crossProduct = FVector::DotProduct(e2, e3);
		return crossProduct < 0.f;
	}

	inline bool Triangle::IsCCW() const noexcept
	{
		return !IsCW();
	}

	inline void Triangle::CW() noexcept
	{
		if (!IsCW())
		{
			std::swap(mPoints[1], mPoints[2]);
		}
	}

	inline void Triangle::CCW() noexcept
	{
		if (!IsCCW())
		{
			std::swap(mPoints[1], mPoints[2]);
		}
	}

	inline bool Triangle::operator==(const Triangle& other) const noexcept
	{
		/*
		同値判定に頂点を用いると三角形の頂点の順番を網羅的に考慮する分条件判定が多くなるので、
		将来は改善が必要です
		*/
		return
			(mPoints[0] == other.mPoints[0] && mPoints[1] == other.mPoints[1] && mPoints[2] == other.mPoints[2]) ||
			(mPoints[0] == other.mPoints[1] && mPoints[1] == other.mPoints[2] && mPoints[2] == other.mPoints[0]) ||
			(mPoints[0] == other.mPoints[2] && mPoints[1] == other.mPoints[0] && mPoints[2] == other.mPoints[1]) ||

			(mPoints[0] == other.mPoints[2] && mPoints[1] == other.mPoints[1] && mPoints[2] == other.mPoints[0]) ||
			(mPoints[0] == other.mPoints[1] && mPoints[1] == other.mPoints[0] && mPoints[2] == other.mPoints[2]) ||
			(mPoints[0] == other.mPoints[0] && mPoints[1] == other.mPoints[2] && mPoints[2] == other.mPoints[1]);
	}

	inline bool Triangle::operator!=(const Triangle& other) const noexcept
	{
		return !(*this == other);
	}

	inline FVector Triangle::operator[](const size_t index) const noexcept
	{
		return *mPoints.at(index);
	}
}
