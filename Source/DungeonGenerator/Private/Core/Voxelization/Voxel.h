/**
グリッドに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Grid.h"
#include "../Helper/Identifier.h"
#include <functional>
#include <map>
#include <memory>

namespace dungeon
{
	// 前方宣言
	class PathGoalCondition;
	struct GenerateParameter;

	/**
	グリッドクラス
	*/
	class Voxel final
	{
	public:
		enum class Error : uint8_t
		{
			Success,
			GoalPointIsOutsideGoalRange,
		};

	public:
		/**
		コンストラクタ
		*/
		explicit Voxel(const GenerateParameter& parameter) noexcept;

		/**
		デストラクタ
		*/
		~Voxel() = default;

		/**
		ボクセル空間の幅を取得します
		*/
		uint32_t GetWidth() const noexcept;

		/**
		ボクセル空間の奥行きを取得します
		*/
		uint32_t GetDepth() const noexcept;

		/**
		ボクセル空間の高さを取得します
		*/
		uint32_t GetHeight() const noexcept;

		/**
		グリッド内のグリッドを取得します
		@param[in]	x		X座標
		@param[in]	y		Y座標
		@param[in]	z		Z座標
		TODO:座標関連はFIntVectorに統一して下さい
		@return		グリッド
		*/
		const Grid& Get(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/**
		グリッド内のグリッドを取得します
		グリッド内のグリッドを取得します
		@param[in]	index	配列番号
		@return		グリッド
		*/
		const Grid& Get(const FIntVector& location) const noexcept;

		/**
		グリッド内のグリッドを取得します
		グリッド内のグリッドを取得します
		@param[in]	index	配列番号
		@return		グリッド
		*/
		const Grid& Get(const size_t index) const noexcept;

		/**
		グリッド内のグリッドを設定します
		@param[in]	x		X座標
		@param[in]	y		Y座標
		@param[in]	z		Z座標
		@param[in]	grid	グリッド
		TODO:座標関連はFIntVectorに統一して下さい
		*/
		void Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) noexcept;

		/**
		矩形の範囲にGridを書き込みます
		@param[in]	min			最小座標
		@param[in]	max			最大座標
		@param[in]	fillGrid	塗りつぶすグリッド
		@param[in]	floorGrid	一階部分のグリッド
		*/
		void Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) noexcept;

		/**
		天井がメッシュ生成禁止か設定します
		@param[in]	noRoofMeshGeneration	天井メッシュの生成禁止
		*/
		void NoRoofMeshGeneration(const FIntVector& location, const bool noRoofMeshGeneration) noexcept;

		/**
		床がメッシュ生成禁止か設定します
		@param[in]	noFloorMeshGeneration	床メッシュの生成禁止
		*/
		void NoFloorMeshGeneration(const FIntVector& location, const bool noFloorMeshGeneration) noexcept;

		/*
		北側の壁がメッシュ生成禁止か設定します
		*/
		void NoNorthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept;

		/*
		南側の壁がメッシュ生成禁止か設定します
		*/
		void NoSouthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept;

		/*
		東側の壁がメッシュ生成禁止か設定します
		*/
		void NoEastWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept;

		/*
		西側の壁がメッシュ生成禁止か設定します
		*/
		void NoWestWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept;

		/**
		門を生成可能な場所を探します
		@param[out]		result				FIntVector
		@param[in]		start				スタート部屋のFIntVector
		@param[in]		identifier			スタート部屋のIdentifier
		@param[in]		goal				ゴール部屋のFIntVector
		@param[in]		shared				trueなら通路を共有する
		@return			trueならば検索成功
		*/
		bool SearchGateLocation(std::map<size_t, FIntVector>& result, const FIntVector& start, const Identifier& identifier, const FIntVector& goal, const bool shared) noexcept;

		/**
		経路をGridに書き込みます
		@param[in]	start					始点
		@param[in]	idealGoal				理想的な終点（goalCondition範囲内に含めて下さい）
		@param[in]	goalCondition			終了条件
		@param[in]	identifier				通路の識別子
		@param[in]	mergeRooms				部屋を結合する
		@param[in]	generateIntersections	交差点を生成する	
		@return		falseならば到達できなかった
		*/
		bool Aisle(const FIntVector& start, const FIntVector& idealGoal, const PathGoalCondition& goalCondition, const Identifier& identifier, const bool mergeRooms, const bool generateIntersections) noexcept;

		/**
		グリッド内のグリッドを更新します
		@param[in]	func	グリッドを更新する関数
		*/
		void Each(std::function<bool(const FIntVector& location, Grid& grid)> func) noexcept;

		/**
		グリッド内のグリッドを更新します
		@param[in]	func	グリッドを参照して更新する関数
		*/
		void Each(std::function<bool(const FIntVector& location, const Grid& grid)> func) const noexcept;

		/**
		グリッド内のグリッドを取得します
		@param[in]	index	配列番号
		@return		グリッド
		*/
		Grid& operator[](const size_t index) noexcept;

		/**
		グリッド内のグリッドを取得します
		@param[in]	index	配列番号
		@return		グリッド
		*/
		const Grid& operator[](const size_t index) const noexcept;

		/**
		座標からボクセルのインデックスを取得します
		@param[in]	x		X座標
		@param[in]	y		Y座標
		@param[in]	z		Z座標
		@return		インデックス
		*/
		size_t Index(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/**
		座標からボクセルのインデックスを取得します
		@param[in]	location	座標
		@return		インデックス
		*/
		size_t Index(const FIntVector& location) const noexcept;

		/**
		座標がボクセル空間内に含まれているか調べます
		@param[in]	location	座標
		@return		trueならば座標はボクセル空間内内部を指している
		*/
		bool Contain(const FIntVector& location) const noexcept;

		/**
		生成時に発生したエラーを取得します
		*/
		Error GetLastError() const noexcept;

		/**
		Calculate CRC32
		@return		CRC32
		*/
		uint32_t CalculateCRC32(uint32_t hash = 0xffffffffU) const noexcept;

	private:
		/**
		通行可能か調べます
		@param[in]	location		座標
		@param[in]	includeAisle	通行可能なグリッドに通路を含める
		@return		trueならば通行可能
		*/
		bool IsPassable(const FIntVector& location, const bool includeAisle) const noexcept;

		/**
		ゴールに到達したか？
		進入方向の許可を含めた確認が必要ならDirection付きの関数を利用する事
		@param[in]	location		座標
		@param[in]	goalAltitude	ゴールの高度
		@param[in]	goalCondition	ゴールの条件
		@return		trueならばゴールに到達
		*/
		bool IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept;

		/**
		ゴールに到達したか？
		進入方向の許可を含めた確認をします。不要ならDirection無しの関数を利用する事
		@param[in]	location			座標
		@param[in]	goalAltitude		ゴールの高度
		@param[in]	goalCondition		ゴールの条件
		@param[in]	enteringDirection	進入方向
		@return		trueならばゴールに到達
		*/
		bool IsReachedGoalWithDirection(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition, const Direction& enteringDirection) noexcept;

	private:
		std::unique_ptr<Grid[]> mGrids;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;

		Error mLastError = Error::Success;
	};
}

#include "Voxel.inl"
