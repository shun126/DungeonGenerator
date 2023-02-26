/**
四面体 ヘッダーファイル

\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Point.h"
#include <algorithm>

namespace dungeon
{
	inline Tetrahedron::Tetrahedron(const std::shared_ptr<const Point>& p0, const std::shared_ptr<const Point>& p1, const std::shared_ptr<const Point>& p2, const std::shared_ptr<const Point>& p3) noexcept
		: mPoints{ { p0, p1, p2, p3 } }
	{
	}
}

namespace std
{
	/**
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
