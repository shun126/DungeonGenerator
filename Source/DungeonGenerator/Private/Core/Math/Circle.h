/**
球に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once
#include <Math/Vector.h>

namespace dungeon
{
	/**
	球クラス
	*/
	struct Circle
	{
		FVector mCenter;
		double mRadius;
	};
}
