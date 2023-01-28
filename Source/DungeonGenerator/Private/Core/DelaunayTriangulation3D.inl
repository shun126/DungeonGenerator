/**
三次元ドロネー三角形分割に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once

namespace dungeon
{
	inline void DelaunayTriangulation3D::ForEach(std::function<void(const Triangle&)> func) noexcept
	{
		for (auto& triangle : mTriangles)
		{
			func(triangle);
		}
	}

	inline void DelaunayTriangulation3D::ForEach(std::function<void(const Triangle&)> func) const noexcept
	{
		for (auto& triangle : mTriangles)
		{
			func(triangle);
		}
	}

	inline bool DelaunayTriangulation3D::IsValid() const noexcept
	{
		return mTriangles.empty() == false;
	}
}
