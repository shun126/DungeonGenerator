/*!
四面体 ヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once
#include "Circle.h"
#include "Point.h"
#include <array>
#include <algorithm>

namespace dungeon
{
	/*!
	四面体クラス
	*/
	class Tetrahedron final
	{
	public:
		static constexpr size_t VertexSize = 4;

	public:
		/*!
		コンストラクタ
		*/
		Tetrahedron() = default;

		/*!
		コンストラクタ
		*/
		Tetrahedron(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3);

		/*!
		デストラクタ
		*/
		~Tetrahedron() = default;

		/*!
		他の四面体と共有点を持つか
		*/
		bool HasCommonPoints(const Tetrahedron& t) const;

		/*
		外接球をの中心点と半径を計算
		*/
		Circle GetCircumscribedSphere() const;

		/*!
		値のハッシュ値を取得します
		\return		値のハッシュ値
		*/
		uint32_t GetHash() const;

		/*!
		等価性の判定  
		*/
		bool operator==(const Tetrahedron& t) const;

		/*!
		等価性の判定
		*/
		bool operator!=(const Tetrahedron& t) const;

		/*!
		頂点を取得します
		*/
		const std::shared_ptr<const Point>& operator[](const size_t index) const;

	private:
		/*
		二次元行列の計算
		*/
		static double dit_2(const double (&dit)[2][2]);

		/*
		三次行列の計算
		*/
		static double dit_3(const double (&dit)[3][3]);

		/*
		4次元の行列
		*/
		static double dit_4(const double (&dit)[4][4]);

	private:
		std::array<std::shared_ptr<const Point>, VertexSize> mPoints;
	};

	inline Tetrahedron::Tetrahedron(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3)
		: mPoints{ { p0, p1, p2, p3 } }
	{
	}
}

namespace std
{
	/*!
	unordered_map,unordered_set等で利用するハッシュ関数
	*/
	template<>
	struct hash<dungeon::Tetrahedron>
	{
		size_t operator()(const dungeon::Tetrahedron& tetrahedron) const noexcept
		{
			return tetrahedron.GetHash();
		}
	};
}
