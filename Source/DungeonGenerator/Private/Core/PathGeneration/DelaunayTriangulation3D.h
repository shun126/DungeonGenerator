/**
三次元ドロネー三角形分割に関するヘッダーファイル

@cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "../Math/Tetrahedron.h"
#include "../Math/Triangle.h"
#include <functional>
#include <vector>

namespace dungeon
{
	/**
	三次元ドロネー三角形分割クラス

	コンストラクタに与えられた座標を元に三角形を生成します
	*/
	class DelaunayTriangulation3D
	{
		using TeraType = std::pair<Tetrahedron, bool>;
		// unordered_mapを使いたいがレプリケーション時に辺の番号が一致しなくなる
		using TetraMap = std::vector<TeraType>;

	public:
		/**
		コンストラクタ
		与えられた点のリストをもとにDelaunay分割を行う
		*/
		explicit DelaunayTriangulation3D(const std::vector<std::shared_ptr<const Point>>& pointList) noexcept;

		/**
		デストラクタ
		*/
		virtual ~DelaunayTriangulation3D() = default;

		/**
		三角形を更新します
		*/
		template<typename Function>
		void ForEach(Function&& function) noexcept
		{
			for (auto& triangle : mTriangles)
			{
				std::forward<Function>(function)(triangle);
			}
		}

		/**
		三角形を更新します
		*/
		template<typename Function>
		void ForEach(Function&& function) const noexcept
		{
			for (auto& triangle : mTriangles)
			{
				std::forward<Function>(function)(triangle);
			}
		}

		/**
		有効な分割か調べます
		@return		有効ならばtrue
		*/
		bool IsValid() const noexcept
		{
			return mTriangles.empty() == false;
		}

	private:
		// 外接する四面体を生成
		Tetrahedron MakeHugeTetrahedron(const std::vector<std::shared_ptr<const Point>>& pointList) noexcept;

		// 四面体の重複管理
		static void AddElementToRedundanciesMap(TetraMap& tetraMap, const Tetrahedron& t) noexcept;

	private:
		std::vector<Triangle> mTriangles;
	};
}
