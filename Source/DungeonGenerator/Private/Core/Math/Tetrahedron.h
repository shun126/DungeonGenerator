/**
四面体 ヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once
#include "Circle.h"
#include <array>

namespace dungeon
{
	class Point;

	/**
	四面体クラス
	*/
	class Tetrahedron final
	{
	public:
		static constexpr size_t VertexSize = 4;

	public:
		/**
		コンストラクタ
		*/
		Tetrahedron() = default;

		/**
		コンストラクタ
		*/
		Tetrahedron(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3) noexcept;

		/**
		デストラクタ
		*/
		~Tetrahedron() = default;

		/**
		他の四面体と共有点を持つか
		*/
		bool HasCommonPoints(const Tetrahedron& t) const noexcept;

		/*
		外接球をの中心点と半径を計算
		*/
		Circle GetCircumscribedSphere() const noexcept;

		/**
		値のハッシュ値を取得します
		\return		値のハッシュ値
		*/
		uint32_t GetHash() const noexcept;

		/**
		等価性の判定  
		*/
		bool operator==(const Tetrahedron& t) const noexcept;

		/**
		等価性の判定
		*/
		bool operator!=(const Tetrahedron& t) const noexcept;

		/**
		頂点を取得します
		*/
		const std::shared_ptr<const Point>& operator[](const size_t index) const noexcept;

	private:
		/*
		二次元行列の計算
		*/
		static double dit_2(const double (&dit)[2][2]) noexcept;

		/*
		三次行列の計算
		*/
		static double dit_3(const double (&dit)[3][3]) noexcept;

		/*
		4次元の行列
		*/
		static double dit_4(const double (&dit)[4][4]) noexcept;

	private:
		std::array<std::shared_ptr<const Point>, VertexSize> mPoints;
	};
}

#include "Tetrahedron.inl"
