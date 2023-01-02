/*!
最小スパニングツリーに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		https://algo-logic.info/kruskal-mst/
*/

#pragma once
#include "DelaunayTriangulation3D.h"
#include "Aisle.h"

namespace dungeon
{
	/*!
	最小スパニングツリークラス
	*/
	class MinimumSpanningTree final
	{
		using Edge = Aisle;

	public:
		/*!
		コンストラクタ
		*/
		explicit MinimumSpanningTree(const DelaunayTriangulation3D& delaunayTriangulation);

		/*!
		コンストラクタ
		*/
		explicit MinimumSpanningTree(const std::vector<std::shared_ptr<const Point>>& points);

		/*!
		デストラクタ
		*/
		~MinimumSpanningTree() = default;

		/*!
		生成した辺を更新します
		*/
		void ForEach(std::function<void(Edge&)> func)
		{
			for (auto& node : mEdges)
			{
				func(node);
			}
		}

		/*!
		生成した辺を参照します
		*/
		void ForEach(std::function<void(const Edge&)> func) const
		{
			for (auto& node : mEdges)
			{
				func(node);
			}
		}

		/*!
		最小スパニングツリーの辺の個数を取得します
		\return		最小スパニングツリーの辺の個数
		*/
		size_t Size() const
		{
			return mEdges.size();
		}

		/*!
		開始地点にふさわしい点を取得します
		\return		開始地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetStartPoint() const
		{
			return mStartPoint;
		}

		/*!
		ゴール地点にふさわしい点を取得します
		\return		ゴール地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetGoalPoint() const
		{
			return mGoalPoint;
		}

		/*!
		行き止まりの点を更新します
		\param[in]	func	点を元に更新する関数
		*/
		void EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const
		{
			for (const auto& point : mLeafPoints)
			{
				func(point);
			}
		}

	private:
		/*!
		頂点配列 クラス
		*/
		class Verteces final
		{
		public:
			/*!
			頂点から頂点インデックスを取得します
			\param[in]	point	頂点
			\return		頂点インデックス
			*/
			size_t Index(const std::shared_ptr<const Point>& point);

			/*!
			インデックスから頂点を取得します
			\param[in]	index	頂点インデックス
			\return		頂点
			*/
			const std::shared_ptr<const Point>& Get(const size_t index) const;

			/*!
			頂点からインデックスを取得します
			\param[in]	point	頂点
			\return		頂点インデックス（見つからない場合は~0が返る）
			*/
			size_t Find(const std::shared_ptr<const Point>& point) const;

		private:
			std::vector<std::shared_ptr<const Point>> mVerteces;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////
		/*!
		頂点インデックスを使った辺クラス
		*/
		class IndexedEdge final
		{
		public:
			/*!
			コンストラクタ
			\param[in]	e0		辺の頂点番号
			\param[in]	e1		辺の頂点番号
			\param[in]	length	頂点間の距離
			*/
			IndexedEdge(const size_t e0, const size_t e1, const float length);

			/*!
			頂点番号を取得します
			\param[in]	index	頂点インデックス
			\return		頂点番号
			*/
			size_t GetEdge(const size_t index) const;

			/*
			辺の長さを取得します
			\return		頂点間の距離
			*/
			float GetLength() const;

		private:
			std::array<size_t, 2> mEdges;
			float mLength;
		};

		/*!
		ルートノード
		*/
		struct RouteNode final
		{
			size_t mVertexIndex;
			float mCost;
		};

		/*!
		初期化
		*/
		void Initialize(const Verteces& verteces, std::vector<IndexedEdge>& edges);

		/*!
		最小コストになるように経路を生成する
		\param[out]	result			std::vector<RouteNode>&
		\param[in]	edges			std::vector<IndexedEdge>& edges,
		\param[in]	vertexIndex		const size_t vertexIndex,
		\param[in]	cost			const float cost);
		*/
		static void Cost(std::vector<RouteNode>& result, std::vector<IndexedEdge>& edges, const size_t vertexIndex, const float cost);

		/*!
		開始地点にふさわしい点を検索します
		\return		開始地点にふさわしい点
		*/
		std::shared_ptr<const Point> FindStartPoint() const;




		void hohoho(const Verteces& verteces, const std::vector<IndexedEdge>& edges, const size_t index, const uint8_t depth);

	private:
		std::vector<Aisle> mEdges;

		std::vector<std::shared_ptr<const Point>> mLeafPoints;
		std::shared_ptr<const Point> mStartPoint;
		std::shared_ptr<const Point> mGoalPoint;
	};
}
