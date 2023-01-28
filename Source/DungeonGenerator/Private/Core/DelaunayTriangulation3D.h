/**
三次元ドロネー三角形分割に関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
\cite		http://tercel-sakuragaoka.blogspot.com/2011/11/c-3-delaunay.html
*/

#pragma once
#include "Math/Tetrahedron.h"
#include "Math/Triangle.h"
#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

namespace dungeon
{
	/**
	三次元ドロネー三角形分割クラス

	コンストラクタに与えられた座標を元に三角形を生成します
	*/
	class DelaunayTriangulation3D
	{
		using TetraMap = std::unordered_map<Tetrahedron, bool>;

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
		void ForEach(std::function<void(const Triangle&)> func) noexcept;

		/**
		三角形を更新します
		*/
		void ForEach(std::function<void(const Triangle&)> func) const noexcept;

		/**
		有効な分割か調べます
		\return		有効ならばtrue
		*/
		bool IsValid() const noexcept;

	private:
		// 外接する四面体を生成
		Tetrahedron MakeHugeTetrahedron(const std::vector<std::shared_ptr<const Point>>& pointList) noexcept;

		// 四面体の重複管理
		static void AddElementToRedundanciesMap(TetraMap& tetraMap, const Tetrahedron& t) noexcept;

	private:
		std::vector<Triangle> mTriangles;
	};
}

#include "DelaunayTriangulation3D.inl"
