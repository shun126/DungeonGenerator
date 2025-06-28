/**
グリッドに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Grid.h"
#include "../Helper/Identifier.h"
#include "../PathGeneration/PathGoalCondition.h"
#include "../PathGeneration/PathFinder.h"
#include <atomic>
#include <memory>
#include <vector>

namespace dungeon
{
	// 前方宣言
	class PathGoalCondition;
	class PathFinder;
	class Room;
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
		TODO: 座標関連はFIntVectorに統一して下さい
		@return		グリッド
		*/
		const Grid& Get(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/**
		グリッド内のグリッドを取得します
		@param[in]	location	グリッド座標
		@return		グリッド
		*/
		const Grid& Get(const FIntVector& location) const noexcept;

		/**
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
		TODO: 座標関連はFIntVectorに統一して下さい
		*/
		void Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) const noexcept;

		/**
		グリッド内のグリッドを設定します
		@param[in]	location	グリッド座標
		@param[in]	grid	グリッド
		*/
		void Set(const FIntVector& location, const Grid& grid) const noexcept;

		/**
		矩形の範囲にGridを書き込みます
		@param[in]	min			最小座標
		@param[in]	max			最大座標
		@param[in]	fillGrid	塗りつぶすグリッド
		@param[in]	floorGrid	一階部分のグリッド
		*/
		void Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) const noexcept;

		/**
		天井がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noRoofMeshGeneration	天井メッシュの生成禁止
		*/
		void NoRoofMeshGeneration(const FIntVector& location, const bool noRoofMeshGeneration) const noexcept;

		/**
		床がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noFloorMeshGeneration	床メッシュの生成禁止
		*/
		void NoFloorMeshGeneration(const FIntVector& location, const bool noFloorMeshGeneration) const noexcept;

		/**
		北側の壁がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		*/
		void NoNorthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		南側の壁がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		*/
		void NoSouthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		東側の壁がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		*/
		void NoEastWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		西側の壁がメッシュ生成禁止か設定します
		@param[in]	location				グリッドの位置
		@param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		*/
		void NoWestWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		 * サブレベル適用グリッドを設定します
		 * @param[in]	location			グリッドの位置
		 */
		void UseSubLevel(const FIntVector& location) noexcept;

		/**
		 * 候補位置
		 */
		struct CandidateLocation final
		{
			uint32_t mPriority;
			FIntVector mLocation;

			CandidateLocation(const uint32_t priority, const FIntVector& location)
				: mPriority(priority)
				, mLocation(location)
			{}
		};

		/**
		門を生成可能な場所を探します
		1. 部屋の内部から部屋の外郭を検索します。
		2. 部屋の外のグリッドは Grid::Type::Empty である必要があります。
		@param[out]		result				FIntVector配列
		@param[in]		maxResultCount		resultの最大数
		@param[in]		start				スタート部屋のFIntVector
		@param[in]		identifier			スタート部屋のIdentifier
		@param[in]		goal				ゴール部屋のFIntVector
		@param[in]		shared				trueなら通路を共有する
		@return			trueならば検索成功
		*/
		bool SearchGateLocation(std::vector<CandidateLocation>& result, const size_t maxResultCount, const FIntVector& start, const Identifier& identifier, const FIntVector& goal, const bool shared) const noexcept;

		/**
		 * 通路生成パラメータ
		 */
		struct AisleParameter final
		{
			PathGoalCondition mGoalCondition;	//!< 終了条件
			Identifier mIdentifier;				//!< 通路の識別子
			bool mMergeRooms;					//!< 部屋を結合する
			bool mGenerateIntersections;		//!< 交差点を生成する
			bool mUniqueLocked;					//!< ユニーク鍵のある通路
			bool mLocked;						//!< 鍵のある通路
			uint8_t mDepthRatioFromStart;		//!< スタート部屋からゴール部屋の部屋数からこの部屋の深さの割合（256段階）
		};

		/**
		経路をGridに書き込みます
		@param[in]	startToGoal				始点にできる位置
		@param[in]	goalToStart				終点に出来る位置
		@param[in]	aisleParameter			通路生成パラメータ
		@return		falseならば到達できなかった
		*/
		bool Aisle(const std::vector<CandidateLocation>& startToGoal, const std::vector<CandidateLocation>& goalToStart, const AisleParameter& aisleParameter) noexcept;

		/**
		 * 最も長い直線の長さを取得します
		 */
		const FIntVector2& GetLongestStraightPath() const noexcept;

	private:
		struct Route final
		{
			FIntVector mStart;
			FIntVector mIdealGoal;
			Route(const FIntVector& start, const FIntVector& idealGoal);
		};
		bool AisleImpl(const std::vector<Route>& route, const AisleParameter& aisleParameter) noexcept;
		std::shared_ptr<PathFinder::Result> FindAisle(const Route& route, const AisleParameter& aisleParameter, const size_t index) const noexcept;
		void WriteAisleToGrid(const std::shared_ptr<PathFinder::Result>& pathResult, const AisleParameter& aisleParameter) const;

		/**
		 * 並んだ門のグリッドを省略できるか調べます
		 * @param location	調べるグリッドの位置
		 * @param direction	調べるグリッドの方向
		 * @param nodeType	一つ前のグリッドの種類
		 * @return trueなら門は省略できる
		 */
		bool CheckDoorAligned(const FIntVector& location, const Direction& direction, const PathFinder::NodeType nodeType) const noexcept;

	public:
		/**
		グリッド内のグリッドを更新します
		@param[in]	function	グリッドを参照して更新する関数
		*/
		template<typename Function>
		void Each(Function&& function) const noexcept
		{
			for (uint32_t z = 0; z < mHeight; ++z)
			{
				for (uint32_t y = 0; y < mDepth; ++y)
				{
					for (uint32_t x = 0; x < mWidth; ++x)
					{
						const size_t index = Index(x, y, z);
						Grid& grid = mGrids.get()[index];
						if (std::forward<Function>(function)(FIntVector(x, y, z), grid) == false)
							return;
					}
				}
			}
		}

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
		uint32_t CalculateCRC32(const uint32_t hash = 0xffffffffU) const noexcept;

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
		static bool IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept;

		/**
		ゴールに到達したか？
		進入方向の許可を含めた確認をします。不要ならDirection無しの関数を利用する事
		@param[in]	location			座標
		@param[in]	goalAltitude		ゴールの高度
		@param[in]	goalCondition		ゴールの条件
		@param[in]	enteringDirection	進入方向
		@return		trueならばゴールに到達
		*/
		bool IsReachedGoalWithDirection(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition, const Direction& enteringDirection) const noexcept;

	public:
		/**
		 * デバッグ用の画像を出力します
		 * @param filename	ファイル名
		 */
		void GenerateImageForDebug(const std::string& filename) const;

	private:
		std::unique_ptr<Grid[]> mGrids;
		FIntVector2 mLongestStraightPath;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;

		Error mLastError = Error::Success;
	};
}

#include "Voxel.inl"
