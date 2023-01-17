/*!
グリッドに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "Grid.h"
#include "Identifier.h"
#include <functional>
#include <memory>

namespace dungeon
{
	// 前方宣言
	class PathGoalCondition;
	struct GenerateParameter;

	/*!
	グリッドクラス
	*/
	class Voxel final
	{
	public:
		/*!
		コンストラクタ
		*/
		explicit Voxel(const GenerateParameter& parameter) noexcept;

		/*!
		デストラクタ
		*/
		~Voxel() = default;

		/*!
		ボクセル空間の幅を取得します
		*/
		uint32_t GetWidth() const noexcept;

		/*!
		ボクセル空間の奥行きを取得します
		*/
		uint32_t GetDepth() const noexcept;

		/*!
		ボクセル空間の高さを取得します
		*/
		uint32_t GetHeight() const noexcept;

		/*!
		グリッド内のグリッドを取得します
		\param[in]	x		X座標
		\param[in]	y		Y座標
		\param[in]	z		Z座標
		TODO:座標関連はFIntVectorに統一して下さい
		\return		グリッド
		*/
		const Grid& Get(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/*!
		グリッド内のグリッドを設定します
		\param[in]	x		X座標
		\param[in]	y		Y座標
		\param[in]	z		Z座標
		\param[in]	grid	グリッド
		TODO:座標関連はFIntVectorに統一して下さい
		*/
		void Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) noexcept;

		/*!
		矩形の範囲にGridを書き込みます
		\param[in]	min			最小座標
		\param[in]	max			最大座標
		\param[in]	fillGrid	塗りつぶすグリッド
		\param[in]	floorGrid	一階部分のグリッド
		*/
		void Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) noexcept;

		/*!
		経路をGridに書き込みます
		\param[in]	start			始点
		\param[in]	idealGoal		理想的な終点（goalCondition範囲内に含めて下さい）
		\param[in]	goalCondition	終了条件
		\param[in]	identifier		識別子
		\return		falseならば到達できなかった
		*/
		bool Aisle(const FIntVector& start, const FIntVector& idealGoal, const PathGoalCondition& goalCondition, const Identifier& identifier) noexcept;

		/*!
		グリッド内のグリッドを更新します
		\param[in]	func	グリッドを更新する関数
		*/
		void Each(std::function<bool(const FIntVector& location, Grid& grid)> func) noexcept;

		/*!
		グリッド内のグリッドを更新します
		\param[in]	func	グリッドを参照して更新する関数
		*/
		void Each(std::function<bool(const FIntVector& location, const Grid& grid)> func) const noexcept;

		/*!
		グリッド内のグリッドを取得します
		\param[in]	index	配列番号
		\return		グリッド
		*/
		Grid& operator[](const size_t index) noexcept;

		/*!
		グリッド内のグリッドを取得します
		\param[in]	index	配列番号
		\return		グリッド
		*/
		const Grid& operator[](const size_t index) const noexcept;

	private:
		/*!
		座標からボクセルのインデックスを取得します
		\param[in]	x		X座標
		\param[in]	y		Y座標
		\param[in]	z		Z座標
		\return		インデックス
		*/
		size_t Index(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/*!
		座標からボクセルのインデックスを取得します
		\param[in]	location	座標
		\return		インデックス
		*/
		size_t Index(const FIntVector& location) const noexcept;

		/*!
		座標がボクセル空間内に含まれているか調べます
		\param[in]	location	座標
		\return		trueならば座標はボクセル空間内内部を指している
		*/
		bool Contain(const FIntVector& location) const noexcept;

		/*!
		空きグリッドか調べます
		\param[in]	location	座標
		\return		trueならば空いている
		*/
		bool IsEmpty(const FIntVector& location) const noexcept;

		/*!
		水平方向の移動で進入できるか？
		\param[in]	location	座標
		\return		trueならば進入可能
		*/
		bool IsHorizontallyPassable(const FIntVector& location) const noexcept;

		/*!
		水平方向の移動で進入できるか？
		\param[in]	baseGrid	基準グリッド
		\param[in]	location	座標
		\return		trueならば進入可能
		*/
		bool IsHorizontallyPassable(const Grid& baseGrid, const FIntVector& location) const noexcept;

		/*!
		ゴールに到達したか？
		\param[in]	location		座標
		\param[in]	goalAltitude	ゴールの高度
		\param[in]	goalCondition	ゴールの条件
		\return		trueならばゴールに到達
		*/
		static bool IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept;

	private:
		std::unique_ptr<Grid[]> mGrids;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;
	};
}
