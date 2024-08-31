/**
三次元ドロネー三角形分割に関するソースファイル

@cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DelaunayTriangulation3D.h"
#include <algorithm>
#include <cmath>
#include <list>

namespace dungeon
{
	DelaunayTriangulation3D::DelaunayTriangulation3D(const std::vector<std::shared_ptr<const Point>>& pointList) noexcept
	{
		std::list<Tetrahedron> tetrahedrons;
		
		if (pointList.empty())
			return;

		// 巨大な外部四面体を作る  
		Tetrahedron hugeTetrahedron = MakeHugeTetrahedron(pointList);
		tetrahedrons.emplace_back(hugeTetrahedron);

		// 点を逐次添加し、反復的に四面体分割を行う  
		for (const std::shared_ptr<const Point>& point : pointList)
		{
			// 追加候補の四面体を保持する一時マップ  
			TetraMap rddcMap;

			// 現在の四面体セットから要素を一つずつ取り出して、    
			// 与えられた点が各々の四面体の外接球の中に含まれるかどうか判定    
			{
				auto tIter = tetrahedrons.begin();
				while (tIter != tetrahedrons.end())
				{
					// 四面体セットから要素を取りだして…
					const Tetrahedron& t = *tIter;

					// 外接球を求める
					const Circle c = t.GetCircumscribedSphere();

					// 外接球に点が内包されている？
					const double distance = FVector::Distance(c.mCenter, *point);
					if (distance < c.mRadius)
					{
						AddElementToRedundanciesMap(rddcMap, Tetrahedron(t[0], t[1], t[2], point));
						AddElementToRedundanciesMap(rddcMap, Tetrahedron(t[0], t[1], t[3], point));
						AddElementToRedundanciesMap(rddcMap, Tetrahedron(t[0], t[2], t[3], point));
						AddElementToRedundanciesMap(rddcMap, Tetrahedron(t[1], t[2], t[3], point));
						tIter = tetrahedrons.erase(tIter);
					}
					else
					{
						++tIter;
					}
				}
			}

			for (auto iter = rddcMap.begin(); iter != rddcMap.end(); ++iter)
			{
				if (iter->second)
					tetrahedrons.emplace_back(iter->first);
			}
		}

#if 0
		// TODO: 最後に、外部三角形の頂点を削除  
		{
			auto tIter = tetrahedrons.begin();
			while (tIter != tetrahedrons.end())
			{
				if (hugeTetrahedron.HasCommonPoints(*tIter))
				{
					tIter = tetrahedrons.erase(tIter);
				}
				else
				{
					++tIter;
				}
			}
		}
#endif

		mTriangles.reserve(tetrahedrons.size());
		for (const Tetrahedron& tetra : tetrahedrons)
		{
			if (tetra[0]->GetOwnerRoom() && tetra[1]->GetOwnerRoom() && tetra[2]->GetOwnerRoom())
				mTriangles.emplace_back(tetra[0], tetra[1], tetra[2]);

			if (tetra[0]->GetOwnerRoom() && tetra[2]->GetOwnerRoom() && tetra[3]->GetOwnerRoom())
				mTriangles.emplace_back(tetra[0], tetra[2], tetra[3]);

			if (tetra[0]->GetOwnerRoom() && tetra[3]->GetOwnerRoom() && tetra[1]->GetOwnerRoom())
				mTriangles.emplace_back(tetra[0], tetra[3], tetra[1]);

			if (tetra[1]->GetOwnerRoom() && tetra[2]->GetOwnerRoom() && tetra[3]->GetOwnerRoom())
				mTriangles.emplace_back(tetra[1], tetra[2], tetra[3]);
		}
	}

	Tetrahedron DelaunayTriangulation3D::MakeHugeTetrahedron(const std::vector<std::shared_ptr<const Point>>& pointList) noexcept
	{
		FVector max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());
		FVector min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

		// 中心座標と半径を求める
		for (const std::shared_ptr<const Point>& point : pointList)
		{
			max.X = std::max(max.X, point->X);
			min.X = std::min(min.X, point->X);
			max.Y = std::max(max.Y, point->Y);
			min.Y = std::min(min.Y, point->Y);
			max.Z = std::max(max.Z, point->Z);
			min.Z = std::min(min.Z, point->Z);
		}
		const FVector center = min + (max - min) * 0.5f;
		const float radius = FVector::Distance(center, min) + 1.f;

		// 全ての頂点を含む四面体を生成
		const std::shared_ptr<const Point> v0 = std::make_shared<const Point>(
			center.X,
			center.Y + 3.0f * radius,
			center.Z);

		const std::shared_ptr<const Point> v1 = std::make_shared<const Point>(
			center.X - 2.0f * std::sqrt(2.0f) * radius,
			center.Y - radius,
			center.Z);

		const std::shared_ptr<const Point> v2 = std::make_shared<const Point>(
			center.X + std::sqrt(2.0f) * radius,
			center.Y - radius,
			center.Z + std::sqrt(6.0f) * radius);

		const std::shared_ptr<const Point> v3 = std::make_shared<const Point>(
			center.X + std::sqrt(2.0f) * radius,
			center.Y - radius,
			center.Z - std::sqrt(6.0f) * radius);

		return Tetrahedron(v0, v1, v2, v3);
	}

	void DelaunayTriangulation3D::AddElementToRedundanciesMap(DelaunayTriangulation3D::TetraMap& tetraMap, const Tetrahedron& t) noexcept
	{
		auto i = std::find_if(tetraMap.begin(), tetraMap.end(), [&t](const TeraType& value)
			{
				return t == value.first;
			}
		);
		if (i != tetraMap.end())
		{
			i->second = false;
		}
		else
		{
			tetraMap.emplace_back(TetraMap::value_type(t, true));
		}
	}
}
