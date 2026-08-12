/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Grid.h"
#include "../Helper/Identifier.h"
#include "../PathGeneration/PathGoalCondition.h"
#include "../PathGeneration/PathFinder.h"
#include <atomic>
#include <Math/Box.h>
#include <Math/UnrealMathUtility.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if WITH_DEV_AUTOMATION_TESTS
class FDungeonVerticallySeparatedAisleCandidateTest;
#endif

namespace dungeon
{
	// 前方宣言
	class PathGoalCondition;
	class PathFinder;
	class Room;
	struct GenerateParameter;
	namespace bmp
	{
		class Canvas;
	}

	/**
	 * Represents Voxel.
	 * 立体的なグリッドクラス
	 */
	class Voxel final
	{
	public:
		enum class Error : uint8_t
		{
			Success,
			GoalPointIsOutsideGoalRange,
		};

		/**
		 * Represents Voxel.
		 * コンストラクタ
		 */
		explicit Voxel(const GenerateParameter& parameter) noexcept;

		/**
		 * Destroys the ~Voxel instance.
		 * デストラクタ
		 */
		~Voxel() = default;

		/**
		 * Returns Width.
		 * ボクセル空間の幅を取得します
		 */
		uint32_t GetWidth() const noexcept;

		/**
		 * Returns Depth.
		 * ボクセル空間の奥行きを取得します
		 */
		uint32_t GetDepth() const noexcept;

		/**
		 * Returns Height.
		 * ボクセル空間の高さを取得します
		 */
		uint32_t GetHeight() const noexcept;

		/**
		 * グリッド内のグリッドを取得します
		 * @param[in]	x		X座標
		 * @param[in]	y		Y座標
		 * @param[in]	z		Z座標
		 * TODO: 座標関連はFIntVectorに統一して下さい
		 * @return		グリッド
		 */
		const Grid& Get(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/**
		 * グリッド内のグリッドを取得します
		 * @param[in]	location	グリッド座標
		 * @return		グリッド
		 */
		const Grid& Get(const FIntVector& location) const noexcept;

		/**
		 * グリッド内のグリッドを取得します
		 * @param[in]	index	配列番号
		 * @return		グリッド
		 */
		const Grid& Get(const size_t index) const noexcept;

		/**
		 * グリッド内のグリッドを設定します
		 * @param[in]	x		X座標
		 * @param[in]	y		Y座標
		 * @param[in]	z		Z座標
		 * @param[in]	grid	グリッド
		 * TODO: 座標関連はFIntVectorに統一して下さい
		 */
		void Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) const noexcept;

		/**
		 * グリッド内のグリッドを設定します
		 * @param[in]	location	グリッド座標
		 * @param[in]	grid	グリッド
		 */
		void Set(const FIntVector& location, const Grid& grid) const noexcept;

	private:
		/**
		 * グリッド内のグリッド参照を取得します
		 * 範囲外を指定した場合はアサートで停止するのでContainでindexを調べて下さい。
		 * @param[in]	index	配列番号
		 * @return		グリッド参照
		 */
		Grid& GetRef(const size_t index) const noexcept;

	public:
		/**
		 * 矩形の範囲にGridを書き込みます
		 * @param[in]	min			最小座標
		 * @param[in]	max			最大座標
		 * @param[in]	fillGrid	塗りつぶすグリッド
		 * @param[in]	floorGrid	一階部分のグリッド
		 */
		void Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) const noexcept;

		/**
		 * 天井がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noRoofMeshGeneration	天井メッシュの生成禁止
		 */
		void NoRoofMeshGeneration(const FIntVector& location, const bool noRoofMeshGeneration) const noexcept;

		/**
		 * 床がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noFloorMeshGeneration	床メッシュの生成禁止
		 */
		void NoFloorMeshGeneration(const FIntVector& location, const bool noFloorMeshGeneration) const noexcept;

		/**
		 * 北側の壁がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		 */
		void NoNorthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		 * 南側の壁がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		 */
		void NoSouthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		 * 東側の壁がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		 */
		void NoEastWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		 * 西側の壁がメッシュ生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noWallMeshGeneration	壁メッシュの生成禁止
		 */
		void NoWestWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) const noexcept;

		/**
		 * ドアが生成禁止か設定します
		 * @param[in]	location				グリッドの位置
		 * @param[in]	noDoorGeneration		ドアの生成禁止
		 */
		void NoDoorGeneration(const FIntVector& location, const bool noDoorGeneration) const noexcept;

		/**
		 * サブレベル適用グリッドを設定します
		 * @param[in]	location			グリッドの位置
		 */
		void UseSubLevel(const FIntVector& location) const noexcept;

		/**
		 * Identifier value that never matches a generated room.
		 * 生成された部屋と一致しない識別子の値です。
		 */
		static constexpr Identifier::IdentifierType InvalidRoomIdentifier = static_cast<Identifier::IdentifierType>(~0);

		/**
		 * Returns whether CandidateLocation.
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
		 * 門を生成可能な場所を探します
		 * 1. 部屋の内部から部屋の外郭を検索します。
		 * 2. 部屋の外のグリッドは Grid::Type::Empty である必要があります。
		 * @param[out]		result				FIntVector配列
		 * @param[in]		maxResultCount		resultの最大数
		 * @param[in]		start				スタート部屋のFIntVector
		 * @param[in]		identifier			スタート部屋のIdentifier
		 * @param[in]		goal				ゴール部屋のFIntVector
		 * @param[in]		shared				trueなら通路を共有する
		 * @return			trueならば検索成功
		 */
		bool SearchGateLocation(std::vector<CandidateLocation>& result, const size_t maxResultCount, const FIntVector& start, const Identifier& identifier, const FIntVector& goal, const bool shared) const noexcept;

		/**
		 * Represents AisleParameter.
		 * 通路生成パラメータ
		 */
		struct AisleParameter final
		{
			PathGoalCondition mGoalCondition;	//!< 終了条件
			Identifier mIdentifier;				//!< 通路の識別子
			/**
			 * Zone index inherited from the deeper endpoint room.
			 * 深度の大きい接続先の部屋から継承する Zone インデックスです。
			 */
			int32 mZoneIndex = INDEX_NONE;
			bool mGenerateIntersections;		//!< 交差点を生成する
			bool mUniqueLocked;					//!< ユニーク鍵のある通路
			bool mLocked;						//!< 鍵のある通路
			uint8_t mDepthRatioFromStart;		//!< スタート部屋からゴール部屋の部屋数からこの部屋の深さの割合（256段階）
			/**
			 * Identifiers of the rooms this aisle connects.
			 * この通路が接続する部屋の識別子です。
			 * 門のために確保されたグリッドを通行できるかの判定に使用します。
			 */
			Identifier::IdentifierType mStartRoomIdentifier = InvalidRoomIdentifier;
			Identifier::IdentifierType mGoalRoomIdentifier = InvalidRoomIdentifier;
		};

		/**
		 * 経路をGridに書き込みます
		 * @param[in]	startToGoal				始点にできる位置
		 * @param[in]	goalToStart				終点に出来る位置
		 * @param[in]	aisleParameter			通路生成パラメータ
		 * @return		falseならば到達できなかった
		 */
		bool Aisle(const std::vector<CandidateLocation>& startToGoal, const std::vector<CandidateLocation>& goalToStart, const AisleParameter& aisleParameter) noexcept;

		/**
		 * 部屋の門にできる面の外側のグリッドを集めます
		 * SearchGateLocationが門の候補にできる面だけを対象にします
		 * @param[out]	result		グリッド座標の配列（呼び出し前に消去されます）
		 * @param[in]	rect		部屋の矩形
		 * @param[in]	groundZ		部屋の床の高さ
		 * @param[in]	identifier	部屋の識別子
		 */
		void CollectGateApproachLocations(std::vector<FIntVector>& result, const FIntRect& rect, const int32 groundZ, const Identifier& identifier) const noexcept;

		/**
		 * 門を生成するためにグリッドを確保します
		 * 確保したグリッドは、指定した部屋に接続しない通路が通行できなくなります
		 * @param[in]	locations	グリッド座標の配列
		 * @param[in]	identifier	確保する部屋の識別子
		 */
		void ReserveGateApproachLocations(const std::vector<FIntVector>& locations, const Identifier& identifier) noexcept;

		/**
		 * 門のために確保した全てのグリッドを解放します
		 */
		void ReleaseGateApproachLocations() noexcept;

		/**
		 * 施錠される通路の識別子を登録します
		 * 門や通路を共有できるかの判定に使用します
		 * @param[in]	identifiers	施錠される通路の識別子
		 */
		void SetLockedAisleIdentifiers(std::unordered_set<Identifier::IdentifierType>&& identifiers) noexcept;

		/**
		 * 登録した施錠される通路の識別子を消去します
		 */
		void ClearLockedAisleIdentifiers() noexcept;

		/**
		 * Registers how tight each room's remaining gate budget is.
		 * An aisle that runs along the wall of a room it does not connect to consumes that room's
		 * gate candidates, so the search pays the registered cost for every such grid.
		 * 部屋ごとの門の余裕の少なさを登録します。
		 * 接続しない部屋の壁際を通る通路は、その部屋の門の候補を奪ってしまうため、
		 * 該当するグリッドを通るたびに登録したコストを支払わせます。
		 * 経路探索は並列に実行されるため、通路の生成を開始する前に設定して下さい。
		 * @param[in]	scarcity	部屋の識別子と追加コストの対応表
		 */
		void SetRoomGateScarcity(std::unordered_map<Identifier::IdentifierType, uint32_t>&& scarcity) noexcept;

		/**
		 * 登録した部屋ごとの門の余裕の少なさを消去します
		 */
		void ClearRoomGateScarcity() noexcept;

		/**
		 * Returns LongestStraightPath.
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
		 * グリッド内のグリッドを更新します
		 * @param[in]	function	グリッドを参照して更新する関数
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
						const auto original = mWidth;

						const size_t index = Index(x, y, z);
						Grid& grid = mGrids.get()[index];
						if (std::forward<Function>(function)(FIntVector(x, y, z), grid) == false)
							return;

						check(original == mWidth);
					}
				}
			}
		}

		/**
		 * 指定範囲内のグリッドを走査します
		 * @param[in]	range	走査範囲（グリッド座標系）
		 * @param[in]	function	グリッドを処理して反復する関数
		 */
		template<typename Function>
		void Each(const FBox& range, Function&& function) const noexcept
		{
			if (mWidth == 0 || mDepth == 0 || mHeight == 0)
				return;

			const int32_t minX = FMath::Clamp(FMath::FloorToInt(range.Min.X), 0, static_cast<int32_t>(mWidth));
			const int32_t minY = FMath::Clamp(FMath::FloorToInt(range.Min.Y), 0, static_cast<int32_t>(mDepth));
			const int32_t minZ = FMath::Clamp(FMath::FloorToInt(range.Min.Z), 0, static_cast<int32_t>(mHeight));
			const int32_t maxX = FMath::Clamp(FMath::CeilToInt(range.Max.X), 0, static_cast<int32_t>(mWidth));
			const int32_t maxY = FMath::Clamp(FMath::CeilToInt(range.Max.Y), 0, static_cast<int32_t>(mDepth));
			const int32_t maxZ = FMath::Clamp(FMath::CeilToInt(range.Max.Z), 0, static_cast<int32_t>(mHeight));

			if (maxX <= minX || maxY <= minY || maxZ <= minZ)
				return;

			for (int32_t z = minZ; z < maxZ; ++z)
			{
				for (int32_t y = minY; y < maxY; ++y)
				{
					for (int32_t x = minX; x < maxX; ++x)
					{
						const size_t index = Index(static_cast<uint32_t>(x), static_cast<uint32_t>(y), static_cast<uint32_t>(z));
						Grid& grid = mGrids.get()[index];
						if (std::forward<Function>(function)(FIntVector(x, y, z), grid) == false)
							return;
					}
				}
			}
		}

		/**
		 * グリッド内のグリッドを取得します
		 * @param[in]	index	配列番号
		 * @return		グリッド
		 */
		Grid& operator[](const size_t index) noexcept;

		/**
		 * グリッド内のグリッドを取得します
		 * @param[in]	index	配列番号
		 * @return		グリッド
		 */
		const Grid& operator[](const size_t index) const noexcept;

		/**
		 * 座標からボクセルのインデックスを取得します
		 * @param[in]	x		X座標
		 * @param[in]	y		Y座標
		 * @param[in]	z		Z座標
		 * @return		インデックス
		 */
		size_t Index(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept;

		/**
		 * 座標からボクセルのインデックスを取得します
		 * @param[in]	location	座標
		 * @return		インデックス
		 */
		size_t Index(const FIntVector& location) const noexcept;

		/**
		 * 座標がボクセル空間内に含まれているか調べます
		 * @param[in]	location	座標
		 * @return		trueならば座標はボクセル空間内内部を指している
		 */
		bool Contain(const FIntVector& location) const noexcept;

		/**
		 * Returns LastError.
		 * 生成時に発生したエラーを取得します
		 */
		Error GetLastError() const noexcept;

		/**
		 * Calculate CRC32
		 * @return		CRC32
		 * CRC32 を計算します。
		 */
		uint32_t CalculateCRC32(const uint32_t hash = 0xffffffffU) const noexcept;

	private:
		/**
		 * 通行可能か調べます
		 * @param[in]	location		座標
		 * @param[in]	includeAisle	通行可能なグリッドに通路を含める
		 * @return		trueならば通行可能
		 */
		bool IsPassable(const FIntVector& location, const bool includeAisle) const noexcept;

		/**
		 * 通路が通行可能か調べます
		 * 門のために確保されたグリッドの判定を含みます
		 * @param[in]	location		座標
		 * @param[in]	includeAisle	通行可能なグリッドに通路を含める
		 * @param[in]	aisleParameter	通路生成パラメータ
		 * @return		trueならば通行可能
		 */
		bool IsPassableForAisle(const FIntVector& location, const bool includeAisle, const AisleParameter& aisleParameter) const noexcept;

		/**
		 * 門のために確保されたグリッドを通行できるか調べます
		 * 確保した部屋に接続する通路であれば通行できます
		 * @param[in]	location		座標
		 * @param[in]	aisleParameter	通路生成パラメータ
		 * @return		trueならば通行可能
		 */
		bool IsGateApproachAvailable(const FIntVector& location, const AisleParameter& aisleParameter) const noexcept;

		/**
		 * 施錠される通路のグリッドか調べます
		 * @param[in]	grid	グリッド
		 * @return		trueならば施錠される通路のグリッド
		 */
		bool IsLockedAisleGrid(const Grid& grid) const noexcept;

		/**
		 * 指定した部屋が鍵をかけた門を持つ施錠通路のグリッドかを返します
		 * @param[in]	grid			判定するグリッド
		 * @param[in]	roomIdentifier	門を開けようとしている部屋の識別子
		 * @return		扉の向こう側の通路ならばtrue
		 */
		bool IsLockedAisleGridBehindDoorOf(const Grid& grid, const Identifier& roomIdentifier) const noexcept;

		/**
		 * 接続しない部屋の壁際を通る事に対する追加コストを返します
		 * @param[in]	location		判定するグリッドの位置
		 * @param[in]	aisleParameter	通路検索パラメーター
		 * @return		追加コスト。接している部屋が無ければ0
		 */
		uint32_t GetRoomProximityCost(const FIntVector& location, const AisleParameter& aisleParameter) const noexcept;

		/**
		 * ゴールに到達したか？
		 * 進入方向の許可を含めた確認が必要ならDirection付きの関数を利用する事
		 * @param[in]	location		座標
		 * @param[in]	goalAltitude	ゴールの高度
		 * @param[in]	goalCondition	ゴールの条件
		 * @return		trueならばゴールに到達
		 */
		static bool IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept;

#if WITH_DEV_AUTOMATION_TESTS
		friend class ::FDungeonVerticallySeparatedAisleCandidateTest;
#endif

		/**
		 * ゴールに到達したか？
		 * 進入方向の許可を含めた確認をします。不要ならDirection無しの関数を利用する事
		 * @param[in]	location			座標
		 * @param[in]	goalAltitude		ゴールの高度
		 * @param[in]	goalCondition		ゴールの条件
		 * @param[in]	enteringDirection	進入方向
		 * @return		trueならばゴールに到達
		 */
		bool IsReachedGoalWithDirection(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition, const Direction& enteringDirection) const noexcept;

	public:
		/**
		 * Sets Floor.
		 * 床があるか設定します
		 */
		void SetFloor(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Sets Ceiling.
		 * 天井があるか設定します
		 */
		void SetCeiling(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Sets NorthWall.
		 * 北側に壁があるか設定します
		 */
		void SetNorthWall(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Sets SouthWall.
		 * 南側に壁があるか設定します
		 */
		void SetSouthWall(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Sets EastWall.
		 * 東側に壁があるか設定します
		 */
		void SetEastWall(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Sets WestWall.
		 * 西側に壁があるか設定します
		 */
		void SetWestWall(const FIntVector& position, const bool enable) const noexcept;

		/**
		 * Returns whether Floor.
		 * 床があるか取得します
		 */
		bool HasFloor(const FIntVector& position) const noexcept;

		/**
		 * Returns whether Ceiling.
		 * 天井があるか取得します
		 */
		bool HasCeiling(const FIntVector& position) const noexcept;

		/**
		 * Returns whether NorthWall.
		 * 北側に壁があるか取得します
		 */
		bool HasNorthWall(const FIntVector& position) const noexcept;

		/**
		 * Returns whether SouthWall.
		 * 南側に壁があるか取得します
		 */
		bool HasSouthWall(const FIntVector& position) const noexcept;

		/**
		 * Returns whether EastWall.
		 * 東側に壁があるか取得します
		 */
		bool HasEastWall(const FIntVector& position) const noexcept;

		/**
		 * Returns whether WestWall.
		 * 西側に壁があるか取得します
		 */
		bool HasWestWall(const FIntVector& position) const noexcept;

		/**
		 * Outputs a debug image with labeled axes and three evenly spaced floor-level dots per grid in the XZ view.
		 * 軸名とXZ側面図の階層位置を示す1グリッド3点の点線を表示したデバッグ画像を出力します
		 * @param filename	Output filename
		 * 					出力するファイル名
		 * @param floorHeights	Finalized floor heights in voxel coordinates
		 * 						ボクセル座標で確定した階層の高さ一覧
		 */
		void GenerateImageForDebug(const std::string& filename, const std::vector<int32_t>& floorHeights) const;

		/**
		 * Saves the same image as GenerateImageForDebug into the artifact directory to keep the history of generated dungeons.
		 * The artifact directory is never cleared, so this is only compiled when DEBUG_GENERATE_ARTIFACT_FILE is defined.
		 * GenerateImageForDebugと同じ画像を、生成したダンジョンの経歴として成果物ディレクトリへ保存します。
		 * 成果物ディレクトリは削除されないため、DEBUG_GENERATE_ARTIFACT_FILEを定義した時だけ有効になります。
		 * @param filePath	Full path to write. The caller builds it so that related files share one name
		 * 					書き出すパス。関連するファイルと名前を揃えられるよう呼び出し側が組み立てます
		 * @param floorHeights	Finalized floor heights in voxel coordinates
		 * 						ボクセル座標で確定した階層の高さ一覧
		 */
		void GenerateImageForArtifact(const std::string& filePath, const std::vector<int32_t>& floorHeights) const;

		/**
		 * Records the endpoints of an aisle that could not be generated.
		 * The aisle owns no grid, so the debug image can only show where it was meant to run.
		 * 生成できなかった通路の両端を記録します。
		 * 失敗した通路はグリッドを持たないため、デバッグ画像にはどこを結ぶはずだったかだけを描けます。
		 * @param[in]	start	通路の開始位置
		 * @param[in]	goal	通路の終了位置
		 */
		void AddFailedAisleEndpoints(const FIntVector& start, const FIntVector& goal) noexcept;

	private:
		/**
		 * Draws the voxel space onto the canvas. Shared by the debug and the artifact image.
		 * ボクセル空間をキャンバスへ描画します。デバッグ画像と成果物画像で共有します。
		 * @param canvas	Canvas to draw on. Drawing methods are const, so the canvas is taken as const
		 * 					描画先のキャンバス。描画関数はconstなのでconstで受け取ります
		 * @param floorHeights	Finalized floor heights in voxel coordinates
		 * 						ボクセル座標で確定した階層の高さ一覧
		 */
		void DrawImageForDebug(const bmp::Canvas& canvas, const std::vector<int32_t>& floorHeights) const;

		std::unique_ptr<Grid[]> mGrids;
		FIntVector2 mLongestStraightPath;
		uint32_t mWidth;
		uint32_t mDepth;
		uint32_t mHeight;

		Error mLastError = Error::Success;

		/**
		 * Grids kept free so that size-fixed rooms can still place their gates.
		 * サイズを変更できない部屋が門を置けるように確保しておくグリッドです。
		 * キーはグリッド番号、値は確保した部屋の識別子です。
		 */
		std::unordered_map<size_t, std::vector<Identifier::IdentifierType>> mGateApproachReservations;

		/**
		 * Identifiers of aisles that will be locked by the mission graph.
		 * ミッショングラフによって施錠される通路の識別子です。
		 * 鍵は門グリッドのPropsに書かれるため、これらの通路は門を共有できません。
		 */
		std::unordered_set<Identifier::IdentifierType> mLockedAisleIdentifiers;

		/**
		 * Endpoints of the aisles that could not be generated.
		 * 生成できなかった通路の両端です。デバッグ画像へ通常とは別の色で描きます。
		 */
		std::vector<std::pair<FIntVector, FIntVector>> mFailedAisleEndpoints;

		/**
		 * Room that holds the locked gate of each locked aisle.
		 * 施錠される通路ごとに、鍵をかけた門を持つ部屋の識別子です。
		 * その部屋から見ると通路は扉の向こう側にあり、反対側の部屋から見ると扉の手前側にあります。
		 */
		mutable std::unordered_map<Identifier::IdentifierType, Identifier::IdentifierType> mLockedAisleDoorRooms;

		/**
		 * Extra path cost charged for running along the wall of each room.
		 * 部屋の壁際を通る事に対する追加コストです。
		 * 門の候補が残り少ない部屋ほど大きな値になります。
		 */
		std::unordered_map<Identifier::IdentifierType, uint32_t> mRoomGateScarcity;
	};
}

#include "Voxel.inl"
