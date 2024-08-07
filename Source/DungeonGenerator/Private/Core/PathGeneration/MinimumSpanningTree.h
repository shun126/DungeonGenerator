/**
最小スパニングツリーに関するヘッダーファイル

@cite		https://algo-logic.info/kruskal-mst/
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DelaunayTriangulation3D.h"
#include "../RoomGeneration/Aisle.h"

namespace dungeon
{
	class Random;

	/**
	Minimum Spanning Tree Class
	*/
	class MinimumSpanningTree final
	{
		using Edge = Aisle;

	public:
		/**
		コンストラクタ
		*/
		MinimumSpanningTree(const std::shared_ptr<Random>& random, const DelaunayTriangulation3D& delaunayTriangulation, const uint8_t aisleComplexity) noexcept;

		/**
		コンストラクタ
		*/
		MinimumSpanningTree(const std::shared_ptr<Random>& random, const std::vector<std::shared_ptr<const Point>>& points, const uint8_t aisleComplexity) noexcept;

		/**
		デストラクタ
		*/
		~MinimumSpanningTree() = default;

		/**
		生成した辺を更新します
		*/
		void ForEach(std::function<void(Edge&)> func) noexcept;

		/**
		生成した辺を参照します
		*/
		void ForEach(std::function<void(const Edge&)> func) const noexcept;

		/**
		最小スパニングツリーの辺の個数を取得します
		@return		最小スパニングツリーの辺の個数
		*/
		size_t Size() const noexcept;

		/**
		開始地点にふさわしい点を取得します
		@return		開始地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetStartPoint() const noexcept;

		/**
		ゴール地点にふさわしい点を取得します
		@return		ゴール地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetGoalPoint() const noexcept;

		/**
		行き止まりの点を更新します
		@param[in]	func	点を元に更新する関数
		*/
		void EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const noexcept;

		/**
		スタート地点から最も深い距離を取得します
		@return		最も遠い距離
		*/
		uint8_t GetDistance() const noexcept;

	private:
		////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		頂点配列 クラス
		*/
		class Verteces final
		{
		public:
			/**
			頂点から頂点インデックスを取得します
			@param[in]	point	頂点
			@return		頂点インデックス
			*/
			size_t Index(const std::shared_ptr<const Point>& point) noexcept;

			/**
			インデックスから頂点を取得します
			@param[in]	index	頂点インデックス
			@return		頂点
			*/
			const std::shared_ptr<const Point>& Get(const size_t index) const noexcept;

			/**
			頂点からインデックスを取得します
			@param[in]	point	頂点
			@return		頂点インデックス（見つからない場合は~0が返る）
			*/
			size_t Find(const std::shared_ptr<const Point>& point) const noexcept;

		private:
			std::vector<std::shared_ptr<const Point>> mVerteces;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		頂点インデックスを使った辺クラス
		*/
		class IndexedEdge final
		{
		public:
			/**
			コンストラクタ
			@param[in]	e0		辺の頂点番号
			@param[in]	e1		辺の頂点番号
			@param[in]	length	頂点間の距離
			*/
			IndexedEdge(const size_t e0, const size_t e1, const float length) noexcept;

			/*
			コピーコンストラクタ
			*/
			IndexedEdge(const IndexedEdge& other) noexcept;

			/*
			ムーブコンストラクタ
			*/
			IndexedEdge(IndexedEdge&& other) noexcept;

			/*
			コピー代入
			*/
			IndexedEdge& operator=(const IndexedEdge& other) noexcept;

			/*
			ムーブ代入
			*/
			IndexedEdge& operator=(IndexedEdge&& other) noexcept;

			/**
			頂点番号を取得します
			@param[in]	index	頂点インデックス
			@return		頂点番号
			*/
			size_t GetEdge(const size_t index) const noexcept;

			/*
			辺の長さを取得します
			@return		頂点間の距離
			*/
			float GetLength() const noexcept;

			/*
			同じ辺か調べます
			*/
			static bool Identical(const IndexedEdge& left, const IndexedEdge& right) noexcept;

		private:
			std::array<size_t, 2> mEdges;
			float mLength;
		};

		////////////////////////////////////////////////////////////////////////////////////////////////
		/**
		ルートノード
		*/
		struct RouteNode final
		{
			size_t mVertexIndex;
			float mCost;
		};

		/**
		初期化
		*/
		void Initialize(const std::shared_ptr<Random>& random, const Verteces& verteces, std::vector<IndexedEdge>& edges, const uint8_t aisleComplexity) noexcept;

		/**
		最小コストになるように経路を生成する
		@param[out]	result			std::vector<RouteNode>&
		@param[in]	edges			std::vector<IndexedEdge>& edges,
		@param[in]	vertexIndex		const size_t vertexIndex,
		@param[in]	cost			const float cost);
		*/
		static void Cost(std::vector<RouteNode>& result, std::vector<IndexedEdge>& edges, const size_t vertexIndex, const float cost) noexcept;

		/*
		重複した辺が登録されているか調べます
		*/
		static bool RedundancyCheck(const std::vector<IndexedEdge>& indexedEdges, const IndexedEdge& indexedEdge) noexcept;

		/**
		開始地点にふさわしい点を検索します
		@return		開始地点にふさわしい点
		*/
		std::shared_ptr<const Point> FindStartPoint() const noexcept;

		/**
		スタート地点から部屋までの距離を設定します
		@param[in]	verteces	頂点
		@param[in]	edges		辺
		@param[in]	index		辺の番号
		@param[in]	depth		距離
		@return		最も遠い距離
		*/
		uint8_t SetDistanceFromStartToRoom(const Verteces& verteces, const std::vector<IndexedEdge>& edges, const size_t index, const uint8_t depth) noexcept;

	private:
		std::vector<Aisle> mEdges;

		std::vector<std::shared_ptr<const Point>> mLeafPoints;
		std::shared_ptr<const Point> mStartPoint;
		std::shared_ptr<const Point> mGoalPoint;

		uint8_t mDistance = 0;
	};
}

#include "MinimumSpanningTree.inl"
