/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "GenerateParameter.h"
#include "Layout/LayoutGraph.h"
#include "RoomGeneration/Aisle.h"
#include "RoomGeneration/Room.h"
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


namespace dungeon
{
	// 前方宣言
	class Grid;
	class MinimumSpanningTree;
	class Voxel;

	/**
	 * Represents Generator.
	 * ダンジョン生成クラス
	 */
	class Generator : public std::enable_shared_from_this<Generator>
	{
	public:
		enum class Error : uint8_t
		{
			Success,
			SeparateRoomsFailed,
			TriangulationFailed,
			GateSearchFailed,
			RouteSearchFailed,
			MissionGraphValidationFailed,
			RoomIsolated,

			// from Voxel class
			___StartVoxelError,
			GoalPointIsOutsideGoalRange,
		};

		/**
		 * Conditions that did not stop the generation but changed what the dungeon offers.
		 * The generated dungeon is complete and reachable, but it does not match every request.
		 * 生成は止めなかったものの、ダンジョンの内容が要求どおりにならなかった事を表します。
		 * 生成されたダンジョンは完成しており到達可能ですが、要求の一部が満たされていません。
		 */
		enum class Warning : uint8_t
		{
			//!< The layout offered no room for the unique key, so no lock was placed / ユニーク鍵を置ける部屋が無いため、ロックを一つも配置しなかった
			KeysAndLocksNotPlaced = 1 << 0,
		};

	public:
		/**
		 * Represents Generator.
		 * コンストラクタ
		 */
		Generator() = default;
		Generator(const Generator&) = delete;
		Generator& operator=(const Generator&) = delete;

		/**
		 * Destroys the ~Generator instance.
		 * デストラクタ
		 */
		virtual ~Generator() = default;

		/**
		 * 生成
		 * @param[in]	parameter	生成パラメータ
		 * @return		trueならば生成成功。falseならばGetLastErrorにて詳細を取得できます。
		 */
		bool Generate(const GenerateParameter& parameter) noexcept;

		/**
		 * Returns LastError.
		 * 生成時に発生したエラーを取得します
		 */
		Error GetLastError() const noexcept;

		/**
		 * Returns the kind of room that ran out of gates.
		 * The value is only meaningful while GetLastError returns GateSearchFailed.
		 * 門が足りなくなった部屋の種類を取得します。
		 * GetLastErrorがGateSearchFailedを返す場合のみ意味を持ちます。
		 */
		Room::Parts GetLastErrorRoomParts() const noexcept;

		/**
		 * Returns the warnings raised while generating.
		 * The value is a bitwise OR of Warning. Zero means the dungeon matches every request.
		 * 生成中に発生した警告を取得します。
		 * 値はWarningのビット和です。0ならば要求どおりのダンジョンが生成されています。
		 */
		uint8_t GetWarningFlags() const noexcept;

		/**
		 * Returns GenerateParameter.
		 * 生成パラメータを取得します
		 */
		const GenerateParameter& GetGenerateParameter() const noexcept;

		/**
		 * Gets metrics from the selected layout candidate.
		 * LastLayoutMetrics を返します。
		 */
		const FDungeonLayoutMetrics& GetLastLayoutMetrics() const noexcept;

		/**
		 * Gets score information from the selected layout candidate.
		 * LastLayoutScore を返します。
		 */
		const FDungeonLayoutScore& GetLastLayoutScore() const noexcept;

		/**
		 * Returns Voxel.
		 * グリッド化された情報を取得
		 */
		const std::shared_ptr<Voxel>& GetVoxel() const noexcept;

		/**
		 * Returns Grid.
		 * グリッド化された情報を取得
		 */
		const Grid& GetGrid(const FIntVector& location) const noexcept;

		/**
		 * Returns Grid.
		 * グリッド化された情報を取得
		 */
		const Grid& GetGrid(const int32 x, const int32 y, const int32 z) const noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Room
		size_t GetRoomCount() const noexcept;

		/**
		 * Represents ForEach.
		 * 生成された部屋を更新します
		 */
		template<typename Function>
		void ForEach(Function&& function) noexcept
		{
			for (const auto& room : mRooms)
			{
				std::forward<Function>(function)(room);
			}
		}

		/**
		 * Represents ForEach.
		 * 生成された部屋を参照します
		 */
		template<typename Function>
		void ForEach(Function&& function) const noexcept
		{
			for (const auto& room : mRooms)
			{
				std::forward<Function>(function)(room);
			}
		}

		/**
		 * Finds ByRoute.
		 * 到達可能な部屋を検索
		 */
		std::vector<std::shared_ptr<Room>> FindByRoute(const std::shared_ptr<Room>& room) const noexcept;

	private:
		void FindByRoute(std::vector<std::shared_ptr<Room>>& passableRooms, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept;
		static bool IsRoutePassable(const std::shared_ptr<Room>& room) noexcept;

	public:
		////////////////////////////////////////////////////////////////////////////////////////////
		// Floor
		/**
		 * 階層の高さの一覧を取得
		 * @return	階層の高さの一覧
		 */
		const std::vector<int32_t>& GetFloorHeight() const;

		/**
		 * 指定した座標が何階か検索します
		 * @param[in]	height	ボクセル空間の高さ
		 * @return		階層
		 */
		size_t FindFloor(const int32_t height) const;

	public:
		////////////////////////////////////////////////////////////////////////////////////////////
		// Aisle
		/**
		 * 全ての通路を更新
		 * @tparam Function	更新関数
		 * @param function	更新関数
		 */
		template<typename Function>
		void EachAisle(Function&& function) const noexcept
		{
			for (const auto& aisle : mAisles)
			{
				if (std::forward<Function>(function)(aisle) == false)
					break;
			}
		}

		/**
		 * 部屋に接続している通路を検索
		 * @tparam	Function	一致した時の関数
		 * @param	room		検索する部屋
		 * @param	function	一致した時の関数
		 */
		template<typename Function>
		void FindAisle(const std::shared_ptr<const Room>& room, Function&& function) const noexcept
		{
			for (const auto& aisle : mAisles)
			{
				const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (room == room0 || room == room1)
				{
					if (std::forward<Function>(function)(aisle) == true)
						break;
				}
			}
		}

		using QueryPartsType = std::list<std::pair<uint32_t, FIntVector>>;
		void OnQueryParts(const std::function<void(QueryPartsType&)>& function) noexcept;
		void OnLoadParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept;
		void OnLoadStartParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept;
		void OnLoadGoalParts(const std::function<void(const std::shared_ptr<Room>&)>& function) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Point
	public:
		/**
		 * 位置から部屋を検索します
		 * 最初にヒットした部屋を返します
		 * @param[in]	point		検索位置
		 * @return		nullptrなら検索失敗
		 */
		std::shared_ptr<Room> Find(const Point& point) const noexcept;

		/**
		 * 開始地点にふさわしい点を取得します
		 * @return		開始地点にふさわしい点
		 */
		const std::shared_ptr<const Point>& GetStartPoint() const noexcept;

		/**
		 * ゴール地点にふさわしい点を取得します
		 * @return		ゴール地点にふさわしい点
		 */
		const std::shared_ptr<const Point>& GetGoalPoint() const noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		void PreGenerateVoxel(const std::function<void(const std::shared_ptr<Voxel>&)>& function) noexcept;
		////////////////////////////////////////////////////////////////////////////////////////////
		// Branch
	public:
		/**
		 * Represents MarkBranchIdAndDepthFromStart.
		 * 分岐番号を記録します
		 */
		bool MarkBranchIdAndDepthFromStart() noexcept;

		/**
		 * Returns DeepestDepthFromStart.
		 * スタートから最も遠い部屋の深さを取得します
		 */
		uint8_t GetDeepestDepthFromStart() const noexcept;

	private:
		void MarkBranchIdAndDepthFromStartRecursive(const std::shared_ptr<Room>& room, uint8_t& branchId, const uint8_t depth) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Attribute
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

		////////////////////////////////////////////////////////////////////////////////////////////
		/**
		 * Calculate CRC32
		 * @return		CRC32
		 * CRC32 を計算します。
		 */
		uint32_t CalculateCRC32(const uint32_t hash = 0xffffffffU) const noexcept;

	private:
		bool GenerateImpl() noexcept;
		std::vector<LayoutCandidate> BuildIntentLayoutCandidates() const noexcept;
		bool SelectDistanceAwareLayout(size_t phase, std::vector<LayoutCandidate>& candidates) noexcept;
		bool RefreshEndpointPoliciesFromCurrentLayout() noexcept;
		bool FinalizeEndpointLayout(size_t phase) noexcept;
		enum class ResolveLayoutCollisionsResult : uint8_t
		{
			Failed,
			Completed,
			Moved
		};
		ResolveLayoutCollisionsResult ResolveLayoutCollisions(size_t phase, const size_t subPhase) noexcept;
		bool ExtractionAisles() noexcept;
		bool GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept;
		void SetRoomParts() noexcept;
		bool AdjustReservedSubLevels(size_t phase) noexcept;
		void ApplyEndpointRoomSizes() const noexcept;
		void AdjustRoomSize(size_t phase) const noexcept;
		/**
		 * Minimizes aisle distance while preserving collision-free room margins.
		 * 部屋の余白と非交差を維持しながら、通路距離を最小化します。
		 */
		bool OptimizeAisleDistance(size_t phase) const noexcept;
		bool ExpandSpace(size_t phase, int32_t horizontalMargin = 1, int32_t verticalMargin = 1) noexcept;
		void AdjustPoints() noexcept;
		void RefreshLockedRouteRoomFlags() noexcept;
		void InvokeRoomCallbacks() const noexcept;
		bool DetectFloorHeightAndDepthFromStart() noexcept;
		bool GenerateVoxel(size_t phase) noexcept;
		void UpdateMeshAttributes() const noexcept;

		/**
		 * Represents the reason why an aisle could not be generated into the voxel space.
		 * 通路をボクセル空間へ生成できなかった理由を表します。
		 */
		enum class AisleVoxelFailure : uint8_t
		{
			/** 開始部屋に門を生成できるグリッドが見つからなかった */
			StartGateNotFound,
			/** ゴール部屋に門を生成できるグリッドが見つからなかった */
			GoalGateNotFound,
			/** 門は生成できたが開始門と終了門を結ぶ経路が見つからなかった */
			RouteNotFound,
		};

		/**
		 * Represents the outcome of generating a single aisle into the voxel space.
		 * 通路一本のボクセル生成結果を表します。
		 */
		enum class AisleVoxelResult : uint8_t
		{
			/** 通路を生成した */
			Succeeded,
			/** 生成できなかったが、この通路が無くても到達可能なので生成を続行する */
			Skipped,
			/** ダンジョンとして成立しないので生成を中止する */
			Failed,
		};

		AisleVoxelResult GenerateAisleVoxel(const size_t aisleIndex, const Aisle& aisle, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint, const uint8_t depthRatioFromStart, const int32 aisleZoneIndex, const bool generateIndoorSlope) noexcept;

		/**
		 * 通路のボクセル生成に失敗した時の報告と、生成を中止するかどうかの判定を行います
		 * @param[in]	aisleIndex	通路配列番号
		 * @param[in]	aisle		生成に失敗した通路
		 * @param[in]	failure		失敗した理由
		 * @param[in]	startPoint	通路の開始位置
		 * @param[in]	goalPoint	通路の終了位置
		 * @return		Skippedならばそのまま生成を続行できます。Failedならば生成を中止します。
		 */
		AisleVoxelResult HandleAisleVoxelFailure(const size_t aisleIndex, const Aisle& aisle, const AisleVoxelFailure failure, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint) noexcept;

		/**
		 * サイズを変更できない部屋が門を置けるように、門の外側のグリッドを確保します
		 * 部屋のボクセルを生成した後、通路のボクセルを生成する前に呼び出して下さい
		 */
		void ReserveGateApproachVoxel() const noexcept;

		/**
		 * 部屋のサイズを変更できないか調べます
		 * @param[in]	room	部屋
		 * @return		trueならばサイズを変更できません
		 */
		bool IsRoomSizeFixed(const Room& room) const noexcept;

		/**
		 * 生成した通路で全ての部屋へ到達できるかを検証します
		 * 生成を諦めた通路は接続に数えません
		 * @return		全ての部屋へ到達できるならtrue
		 */
		bool VerifyRoomReachability() noexcept;

		/**
		 * 部屋の壁際を通る事に対する追加コストを作り直します
		 * @param[in]	remainingGates	部屋の識別子と、まだ受け入れる必要のある通路の本数
		 */
		void UpdateRoomGateScarcity(const std::unordered_map<Identifier::IdentifierType, uint8_t>& remainingGates) const noexcept;

		void ExpandAisleHeightVoxel(const Aisle& aisle) const noexcept;
		void GenerateRoomSkylightVoxel(const std::shared_ptr<Room>& room, const uint8_t depthRatioFromStart) noexcept;
		void GenerateStructuralColumnVoxel(const std::shared_ptr<Room>& room) const;
		bool CanFillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const;
		void FillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const;

		/**
		 * Represents Reset.
		 * リセット
		 */
		void Reset();


#if WITH_EDITOR
		/**
		 * デバッグ用に部屋の位置を画像に出力します
		 * @param filename	ファイル名
		 */
		void GenerateRoomImageForDebug(const std::string& filename) const;

	public:
		/**
		 * markdown + mermaidによるフローチャートを出力します
		 * @param path		ファイル名
		 */
		void DumpRoomDiagram(const std::string& path) const noexcept;

	private:
		/**
		 * デバッグ用に部屋の構造をダイアグラムに出力します
		 * @param stream			出力先
		 * @param passableAisles	通路
		 * @param room				部屋
		 */
		void DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept;

		/**
		 * デバッグ用に部屋と通路の情報をダンプします
		 * @param[in]	index	通路配列番号
		 */
		void DumpAisleAndRoomInformation(const size_t index) const noexcept;

		void DumpVoxel(const std::shared_ptr<const Point>& point) const noexcept;
		void DumpVoxel(const std::shared_ptr<Room>& room) const noexcept;

		/**
		 * 通路のボクセル生成に失敗した理由の名前を取得します
		 * @param[in]	failure	失敗した理由
		 * @return		失敗した理由の名前
		 */
		static const TCHAR* GetAisleVoxelFailureName(const AisleVoxelFailure failure) noexcept;

		/**
		 * デバッグ用に通路のボクセル生成に失敗した状況を出力します
		 * @param[in]	aisleIndex	通路配列番号
		 * @param[in]	aisle		生成に失敗した通路
		 * @param[in]	failure		失敗した理由
		 * @param[in]	startPoint	通路の開始位置
		 * @param[in]	goalPoint	通路の終了位置
		 */
		void ReportAisleVoxelFailure(const size_t aisleIndex, const Aisle& aisle, const AisleVoxelFailure failure, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint) const noexcept;
#endif

	private:
		GenerateParameter mGenerateParameter;
		/**
		 * Selected intent graph retained so final room floors can update their zones.
		 * 最終的な部屋の階層に合わせてZoneを更新するために保持する、選択済みの意図グラフです。
		 */
		LayoutGraph mLayoutGraph;

		std::shared_ptr<Voxel> mVoxel;

		std::list<std::shared_ptr<Room>> mRooms;
		std::vector<int32_t> mFloorHeight;

		std::shared_ptr<Room> mStartRoom;
		// TODO: mStartRoomに置き換えてください
		std::shared_ptr<const Point> mStartPoint;

		std::shared_ptr<Room> mGoalRoom;
		// TODO: mGoalRoomに置き換えてください
		std::shared_ptr<const Point> mGoalPoint;

		std::vector<Aisle> mAisles;

		std::function<void(const std::shared_ptr<Voxel>&)> mOnPreGenerateVoxel;

		std::function<void(QueryPartsType&)> mOnQueryParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadStartParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadGoalParts;

		uint8_t mDeepestDepthFromStart = 0;

		Error mLastError = Error::Success;
		uint8_t mWarningFlags = 0;

		/**
		 * Identifiers of the aisles that were given up during voxel generation.
		 * ボクセル生成で作るのを諦めた通路の識別子です。
		 * 実際には掘られていないため、部屋の接続としては数えません。
		 */
		std::unordered_set<Identifier::IdentifierType> mAbandonedAisleIdentifiers;

		//!< Parts of the room that ran out of gates / 門が足りなくなった部屋の種類
		Room::Parts mLastErrorRoomParts = Room::Parts::Unidentified;
		FDungeonLayoutMetrics mLastLayoutMetrics;
		FDungeonLayoutScore mLastLayoutScore;
	};
}

#include "Generator.inl"
