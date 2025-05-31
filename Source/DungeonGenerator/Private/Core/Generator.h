/**
ダンジョン生成ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once 
#include "GenerateParameter.h"
#include "Math/PerlinNoise.h"
#include "RoomGeneration/Aisle.h"
#include "RoomGeneration/Room.h"
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace dungeon
{
	// 前方宣言
	class Grid;
	class MinimumSpanningTree;
	class Voxel;

	/**
	ダンジョン生成クラス
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

			// from Voxel class
			___StartVoxelError,
			GoalPointIsOutsideGoalRange,
		};

	public:
		/**
		コンストラクタ
		*/
		Generator() = default;
		Generator(const Generator&) = delete;
		Generator& operator=(const Generator&) = delete;

		/**
		デストラクタ
		*/
		virtual ~Generator() = default;

		/**
		生成
		@param[in]	parameter	生成パラメータ
		@return		trueならば生成成功。falseならばGetLastErrorにて詳細を取得できます。
		*/
		bool Generate(const GenerateParameter& parameter) noexcept;

		/**
		生成時に発生したエラーを取得します
		*/
		Error GetLastError() const noexcept;

		/**
		生成パラメータを取得します
		*/
		const GenerateParameter& GetGenerateParameter() const noexcept;

		/**
		グリッド化された情報を取得
		*/
		const std::shared_ptr<Voxel>& GetVoxel() const noexcept;

		const Grid& GetGrid(const FIntVector& location) const noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Room
		size_t GetRoomCount() const noexcept;

		/**
		生成された部屋を更新します
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
		生成された部屋を参照します
		*/
		template<typename Function>
		void ForEach(Function&& function) const noexcept
		{
			for (const auto& room : mRooms)
			{
				std::forward<Function>(function)(room);
			}
		}

		// 深度による検索
		std::shared_ptr<Room> FindByIdentifier(const Identifier& identifier) const noexcept;

		// 深度による検索
		std::vector<std::shared_ptr<Room>> FindByDepth(const uint8_t depth) const noexcept;

		// ブランチによる検索
		std::vector<std::shared_ptr<Room>> FindByBranch(const uint8_t branchId) const noexcept;

		// 到達可能な部屋を検索
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
		指定した座標が何階か検索します
		@param[in]	height	ボクセル空間の高さ
		@return		階層
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
		位置から部屋を検索します
		最初にヒットした部屋を返します
		@param[in]	point		検索位置
		@return		nullptrなら検索失敗
		*/
		std::shared_ptr<Room> Find(const Point& point) const noexcept;

		/**
		位置から部屋を検索します
		検索位置を含むすべての部屋を返します
		@param[in]	point		検索位置
		@return		コンテナのサイズが0なら検索失敗
		*/
		std::vector<std::shared_ptr<Room>> FindAll(const Point& point) const noexcept;

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

		////////////////////////////////////////////////////////////////////////////////////////////
		void PreGenerateVoxel(const std::function<void(const std::shared_ptr<Voxel>&)>& function) noexcept;
		void PostGenerateVoxel(const std::function<void(const std::shared_ptr<Voxel>&)>& function) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Branch
	public:
		/**
		 * 分岐番号を記録します
		 */
		bool MarkBranchIdAndDepthFromStart() noexcept;

		/**
		 * スタートから最も遠い部屋の深さを取得します
		 */
		uint8_t GetDeepestDepthFromStart() const noexcept;

	private:
		void MarkBranchIdAndDepthFromStartRecursive(const std::shared_ptr<Room>& room, uint8_t& branchId, const uint8_t depth) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
	public:
		/**
		Calculate CRC32
		@return		CRC32
		*/
		uint32_t CalculateCRC32(const uint32_t hash = 0xffffffffU) const noexcept;

	private:
		bool GenerateImpl() noexcept;
		bool GenerateRooms() noexcept;
		enum class SeparateRoomsResult : uint8_t
		{
			Failed,
			Completed,
			Moved
		};
		SeparateRoomsResult SeparateRooms(const size_t phase, const size_t subPhase) noexcept;
		void SeparateRoom(const std::shared_ptr<Room>& fixedRoom, const std::vector<std::shared_ptr<Room>>& intersectedRooms, const bool activateOuterMovement) const noexcept;
		bool ExtractionAisles() noexcept;
		bool GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept;
		void SetRoomParts() noexcept;
		bool AdjustedStartAndGoalSubLevel() const noexcept;
		void AdjustRoomSize() const noexcept;
		bool ExpandSpace(const int32_t horizontalMargin = 3, const int32_t verticalMargin = 1) noexcept;
		void AdjustPoints() noexcept;
		void InvokeRoomCallbacks() const noexcept;
		bool DetectFloorHeightAndDepthFromStart() noexcept;
		bool GenerateVoxel() noexcept;
		bool GenerateAisleVoxel(const size_t aisleIndex, const Aisle& aisle, const std::shared_ptr<const Point>& startPoint, const std::shared_ptr<const Point>& goalPoint, const uint8_t depthRatioFromStart, const bool generateIndoorSlope) noexcept;
		void GenerateStructuralColumnVoxel(const std::shared_ptr<Room>& room) const;
		bool CanFillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const;
		void FillStructuralColumnVoxel(const int32 x, const int32 y, const int32 minZ, const int32 maxZ) const;

		/**
		リセット
		*/
		void Reset();


#if WITH_EDITOR
		/**
		 * デバッグ用にパーリンノイズの画像を出力します
		 * @param perlinNoise		パーリンノイズ
		 * @param octaves			ノイズのオクターブ値
		 * @param noiseBoostRatio	出力ノイズ
		 * @param filename			ファイル名
		 */
		static void GenerateHeightImageForDebug(const PerlinNoise& perlinNoise, const std::size_t octaves, const float noiseBoostRatio, const std::string& filename) noexcept;

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
		デバッグ用に部屋と通路の情報をダンプします
		@param[in]	index	通路配列番号
		*/
		void DumpAisleAndRoomInformation(const size_t index) const noexcept;

		void DumpVoxel(const std::shared_ptr<const Point>& point) const noexcept;
		void DumpVoxel(const std::shared_ptr<Room>& room) const noexcept;
#endif

	private:
		GenerateParameter mGenerateParameter;

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
		std::function<void(const std::shared_ptr<Voxel>&)> mOnPostGenerateVoxel;

		std::function<void(QueryPartsType&)> mOnQueryParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadStartParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnLoadGoalParts;

		uint8_t mDeepestDepthFromStart = 0;

		Error mLastError = Error::Success;
	};
}

#include "Generator.inl"
