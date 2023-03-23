/**
最小スパニングツリーに関するソースファイル

\cite		https://algo-logic.info/kruskal-mst/
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MinimumSpanningTree.h"
#include "Room.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace dungeon
{
	////////////////////////////////////////////////////////////////////////////////////////////////
	size_t MinimumSpanningTree::Verteces::Index(const std::shared_ptr<const Point>& point) noexcept
	{
		const auto i = std::find_if(mVerteces.begin(), mVerteces.end(), [point](const std::shared_ptr<const Point>& p) { return p == point; });
		if (i == mVerteces.end())
		{
			const size_t index = mVerteces.size();
			mVerteces.emplace_back(point);
			return index;
		}
		else
		{
			return std::distance(mVerteces.begin(), i);
		}
	}

	const std::shared_ptr<const Point>& MinimumSpanningTree::Verteces::Get(const size_t index) const noexcept
	{
		return mVerteces.at(index);
	}

	size_t MinimumSpanningTree::Verteces::Find(const std::shared_ptr<const Point>& point) const noexcept
	{
		const auto i = std::find_if(mVerteces.begin(), mVerteces.end(), [&point](const std::shared_ptr<const Point>& p)
			{
				return p == point;
			}
		);
		return i == mVerteces.end() ? static_cast<size_t>(~0) : std::distance(mVerteces.begin(), i);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	MinimumSpanningTree::IndexedEdge::IndexedEdge(const size_t e0, const size_t e1, const float length) noexcept
		: mLength(length)
	{
		mEdges[0] = e0;
		mEdges[1] = e1;
	}

	size_t MinimumSpanningTree::IndexedEdge::GetEdge(const size_t index) const noexcept
	{
		return mEdges.at(index);
	}

	float MinimumSpanningTree::IndexedEdge::GetLength() const noexcept
	{
		return mLength;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	UnionFind：素集合系管理の構造体(union by rank)
	IsSame(x, y): x と y が同じ集合にいるか。 計算量はならし O(α(n))
	Unite(x, y): x と y を同じ集合にする。計算量はならし O(α(n))
	*/
	class UnionFind final
	{
	public:
		/**
		コンストラクタ
		\param[in]	edgeSize	edgeの数
		*/
		explicit UnionFind(const size_t edgeSize) noexcept
		{
			mRank.resize(edgeSize, 0);
			mParents.resize(edgeSize, 0);
			for (size_t i = 0; i < edgeSize; ++i)
			{
				MakeTree(i);
			}
		}

		/**
		x と y が同じ集合にいるか調べます
		\param[in]	x	頂点番号
		\param[in]	y	頂点番号	
		\return		trueならば同じ集合にいるか調べます
		*/
		bool IsSame(const size_t x, const size_t y) noexcept
		{
			return FindRoot(x) == FindRoot(y);
		}

		/**
		x と y を同じ集合にする
		\param[in]	x	頂点番号
		\param[in]	y	頂点番号
		*/
		void Unite(size_t x, size_t y) noexcept
		{
			x = FindRoot(x);
			y = FindRoot(y);
			if (mRank[x] > mRank[y])
			{
				mParents[y] = x;
			}
			else
			{
				mParents[x] = y;
				if (mRank[x] == mRank[y])
				{
					++mRank[y];
				}
			}
		}

	private:
		/**
		木の生成
		\param[in]	index	頂点番号
		*/
		void MakeTree(const size_t index) noexcept
		{
			mParents[index] = index;
			mRank[index] = 0;
		}

		/**
		親の頂点番号を検索
		\param[in]	index	頂点番号
		\return		親の頂点番号
		*/
		size_t FindRoot(size_t index) noexcept
		{
			if (index != mParents[index])
				mParents[index] = FindRoot(mParents[index]);
			return mParents[index];
		}

	private:
		std::vector<size_t> mRank;
		std::vector<size_t> mParents;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////
	MinimumSpanningTree::MinimumSpanningTree(const DelaunayTriangulation3D& delaunayTriangulation) noexcept
	{
		Verteces verteces;
		std::vector<IndexedEdge> indexedEdges;

		// 全ての辺の集合を作成
		delaunayTriangulation.ForEach([&verteces, &indexedEdges](const Triangle& triangle) {
			const std::shared_ptr<const Point>& p1 = triangle.GetPoint(0);
			const std::shared_ptr<const Point>& p2 = triangle.GetPoint(1);
			const std::shared_ptr<const Point>& p3 = triangle.GetPoint(2);
			const size_t v1 = verteces.Index(p1);
			const size_t v2 = verteces.Index(p2);
			const size_t v3 = verteces.Index(p3);
			indexedEdges.emplace_back(v1, v2, Point::Dist(*p1, *p2));
			indexedEdges.emplace_back(v2, v3, Point::Dist(*p2, *p3));
			indexedEdges.emplace_back(v3, v1, Point::Dist(*p3, *p1));
		});

		Initialize(verteces, indexedEdges);
	}

	MinimumSpanningTree::MinimumSpanningTree(const std::vector<std::shared_ptr<const Point>>& points) noexcept
	{
		Verteces verteces;
		std::vector<IndexedEdge> edges;

		// 全ての辺の集合を作成
		for(size_t i = 0; i < points.size(); ++i)
		{
			const size_t n = (i + 1) % points.size();
			const std::shared_ptr<const Point>& p1 = points[i];
			const std::shared_ptr<const Point>& p2 = points[n];
			const size_t v1 = verteces.Index(p1);
			const size_t v2 = verteces.Index(p2);
			edges.emplace_back(v1, v2, Point::Dist(*p1, *p2));
		}

		Initialize(verteces, edges);
	}

	/*
	クラスカル法
	*/
	void MinimumSpanningTree::Initialize(const Verteces& verteces, std::vector<IndexedEdge>& edges) noexcept
	{
		// コストが少ない順に整列
		std::sort(edges.begin(), edges.end(), [](const IndexedEdge& l, const IndexedEdge& r)
			{
				return l.GetLength() < r.GetLength();
			});

		// 木を生成
		{
			UnionFind unionFind(edges.size());

			auto edge = edges.begin();
			while (edge != edges.end())
			{
				// 閉路にならなければ加える
				if (!unionFind.IsSame(edge->GetEdge(0), edge->GetEdge(1)))
				{
					unionFind.Unite(edge->GetEdge(0), edge->GetEdge(1));

					const std::shared_ptr<const Point>& v0 = verteces.Get(edge->GetEdge(0));
					const std::shared_ptr<const Point>& v1 = verteces.Get(edge->GetEdge(1));
					mEdges.emplace_back(v0, v1);
					++edge;
				}
				else
				{
					// 閉路なので取り除く
					edge = edges.erase(edge);
				}
			}
		}

		// スタートを記録
		mStartPoint = FindStartPoint();

		size_t startPointIndex = verteces.Find(mStartPoint);
		// cppcheck-suppress [knownConditionTrueFalse, unmatchedSuppression]
		if (startPointIndex != static_cast<size_t>(~0))
		{
			// スタートから各部屋の深さ（部屋の数）を設定
			mDistance = SetDistanceFromStartToRoom(verteces, edges, startPointIndex, 0);

			// ゴールを記録
			{
				std::vector<RouteNode> routeNodes;
				Cost(routeNodes, edges, startPointIndex, 0.f);
				if (routeNodes.empty())
				{
					mGoalPoint = mStartPoint;
				}
				else
				{
					// 最も遠いポイントをゴールとして記録
					float maxCost = std::numeric_limits<float>::lowest();
					size_t vertexIndex = 0;
					for (const auto& routeNode : routeNodes)
					{
						if (maxCost < routeNode.mCost)
						{
							maxCost = routeNode.mCost;
							vertexIndex = routeNode.mVertexIndex;
						}
					}
					mGoalPoint = verteces.Get(vertexIndex);

					// 末端のポイントを記録
					mLeafPoints.reserve(routeNodes.size() - 1);
					for (const auto& routeNode : routeNodes)
					{
						if (vertexIndex != routeNode.mVertexIndex)
						{
							mLeafPoints.emplace_back(
								verteces.Get(routeNode.mVertexIndex)
							);
						}
					}
				}
			}
		}
	}

	// スタートから各部屋の深さ（部屋の数）を設定
	uint8_t MinimumSpanningTree::SetDistanceFromStartToRoom(const Verteces& verteces, const std::vector<IndexedEdge>& edges, const size_t index, const uint8_t depth) noexcept
	{
		uint8_t distanceFromStart = 0;
		for (auto& edge : edges)
		{
			for (size_t i = 0; i < 2; ++i)
			{
				const size_t vertexIndex = edge.GetEdge(i);
				if (index == vertexIndex)
				{
					if (const std::shared_ptr<Room>& room = verteces.Get(vertexIndex)->GetOwnerRoom())
					{
						if (room->GetDepthFromStart() > depth)
							room->SetDepthFromStart(depth);
					}

					// 辺の反対側の部屋を設定
					const size_t otherIndex = edge.GetEdge((i + 1) % 2);
					if (const std::shared_ptr<const Room>& otherRoom = verteces.Get(otherIndex)->GetOwnerRoom())
					{
						if (otherRoom->GetDepthFromStart() > depth)
						{
							uint8_t distance = SetDistanceFromStartToRoom(verteces, edges, otherIndex, depth + 1);
							if (distanceFromStart < distance)
								distanceFromStart = distance;
						}
					}
				}
			}
		}
		return distanceFromStart;
	}

	void MinimumSpanningTree::Cost(std::vector<RouteNode>& result, std::vector<IndexedEdge>& edges, const size_t vertexIndex, const float cost) noexcept
	{
		std::vector<std::pair<size_t, float>> routes;

		auto edge = edges.begin();
		while (edge != edges.end())
		{
			size_t index = 0;
			while (index < 2)
			{
				if (edge->GetEdge(index) == vertexIndex)
					break;
				++index;
			}

			if (index < 2)
			{
				// 追跡するルートを記録
				routes.push_back(
					std::pair<size_t, float>(
						edge->GetEdge((index + 1) & 1),		// 次の頂点インデックスを取得
						cost + edge->GetLength()			// 今回のコストを追加
						)
				);

				// 今回のedgeを除去
				edge = edges.erase(edge);
			}
			else
			{
				++edge;
			}
		}

		if (routes.empty())
		{
			RouteNode routeNode;
			routeNode.mVertexIndex = vertexIndex;
			routeNode.mCost = cost;
			result.emplace_back(std::move(routeNode));
		}
		else
		{
			for (const auto& route : routes)
			{
				Cost(result, edges, route.first, route.second);
			}
		}
	}

	std::shared_ptr<const Point> MinimumSpanningTree::FindStartPoint() const noexcept
	{
		/*
		全ての辺の中央の位置と
		Yが最も大きい点をを求める
		*/
		std::shared_ptr<Point> center = std::make_shared<Point>(0.f, 0.f, 0.f);
		std::shared_ptr<const Point> bottom = nullptr;
		size_t count = 0;
		{
			float maxY = std::numeric_limits<float>::lowest();
			for (const Edge& edge : mEdges)
			{
				*center += static_cast<FVector>(*edge.GetPoint(0));
				*center += static_cast<FVector>(*edge.GetPoint(1));
				count += 2;

				for (size_t i = 0; i < 2; ++i)
				{
					if (maxY < edge.GetPoint(i)->Y)
					{
						maxY = edge.GetPoint(i)->Y;
						bottom = edge.GetPoint(i);
					}
				}
			}
		}

		if (bottom == nullptr)
		{
			return center;
		}

		if (count > 0)
		{
			*center /= static_cast<float>(count);
		}

		// 最もYが大きく、最も中央にある点を探す
		{
			const Point candidateStartPoint(center->X, bottom->Y, bottom->Z);
			std::shared_ptr<const Point> resultPoint = nullptr;
			float min = std::numeric_limits<float>::max();
			for (const auto& edge : mEdges)
			{
				for (size_t i = 0; i < 2; ++i)
				{
					const float distance = Point::Dist(*edge.GetPoint(i), candidateStartPoint);
					if (min > distance)
					{
						min = distance;
						resultPoint = edge.GetPoint(i);
					}
				}
			}
			return resultPoint;
		}
	}
}
