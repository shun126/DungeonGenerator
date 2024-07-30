/**
ダンジョン生成ヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once 
#include "GenerateParameter.h"
#include "RoomGeneration/Aisle.h"
#include "RoomGeneration/Room.h"
#include <atomic>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace dungeon
{
	// 前方宣言
	class Grid;
	class MinimumSpanningTree;
	class PerlinNoise;
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
		void ForEach(std::function<void(const std::shared_ptr<Room>&)> func) noexcept;

		/**
		生成された部屋を参照します
		*/
		void ForEach(std::function<void(const std::shared_ptr<const Room>&)> func) const noexcept;

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
		const std::vector<int32_t>& GetFloorHeight() const;

		/*
		指定した座標が何階か検索します
		*/
		const size_t FindFloor(const int32_t height) const;

	public:
		////////////////////////////////////////////////////////////////////////////////////////////
		// Aisle
		void EachAisle(std::function<bool(const Aisle& edge)> func) const noexcept;

		// 部屋に接続している通路を検索
		void FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(Aisle& edge)> func) noexcept;

		void FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(const Aisle& edge)> func) const noexcept;

		void OnQueryParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept;
		void OnStartParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept;
		void OnGoalParts(std::function<void(const std::shared_ptr<Room>&)> func) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		// Point
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

		/**
		行き止まりの点を更新します
		@param[in]	func	点を元に更新する関数
		*/
		void EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const noexcept;


		////////////////////////////////////////////////////////////////////////////////////////////
		void PreGenerateVoxel(std::function<void(const std::shared_ptr<Voxel>&)> func) noexcept;
		void PostGenerateVoxel(std::function<void(const std::shared_ptr<Voxel>&)> func) noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		/*
		markdown + mermaidによるフローチャートを出力します
		*/
		void DumpRoomDiagram(const std::string& path) const noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		/*
		分岐番号を記録します
		*/
		bool MarkBranchId() noexcept;

	private:
		bool MarkBranchId(std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<Room>& room, uint8_t& branchId) noexcept;

	public:
		/*
		スタートから最も遠い部屋の深さを取得します
		*/
		uint8_t GetDeepestDepthFromStart() const noexcept;

		////////////////////////////////////////////////////////////////////////////////////////////
		/**
		Calculate CRC32
		@return		CRC32
		*/
		uint32_t CalculateCRC32(uint32_t hash = 0xffffffffU) const noexcept;

	private:
		/**
		生成
		*/
		bool GenerateImpl(GenerateParameter& parameter) noexcept;

		/**
		部屋の生成
		*/
		bool GenerateRooms(const GenerateParameter& parameter) noexcept;

		/**
		部屋の重なりを解消します
		*/
		bool SeparateRooms(const GenerateParameter& parameter, const size_t phase) noexcept;

		/**
		全ての部屋が収まるように空間を拡張します
		*/
		bool ExpandSpace(GenerateParameter& parameter, const int32_t margin) noexcept;

		/*
		階層の高さを検出
		*/	
		bool DetectFloorHeight() noexcept;
	
		/**
		通路の抽出
		*/
		bool ExtractionAisles(const GenerateParameter& parameter, const size_t phase) noexcept;

		/**
		部屋のパーツ（役割）を設定する
		*/
		void SetRoomParts(GenerateParameter& parameter) noexcept;

		/**
		開始部屋と終了部屋のサブレベルを配置する隙間を調整
		*/
		bool AdjustedStartAndGoalSublevel(GenerateParameter& parameter) noexcept;

		/**
		部屋のコールバックを呼ぶ
		*/
		void InvokeRoomCallbacks(GenerateParameter& parameter) noexcept;

		/**
		ボクセル情報を生成
		*/
		bool GenerateVoxel(const GenerateParameter& parameter) noexcept;

		/**
		通路の生成
		*/
		bool GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept;

		/**
		リセット
		*/
		void Reset();

		/*
		デバッグ用に部屋の位置を画像に出力します
		*/
		void GenerateRoomImageForDebug(const std::string& filename) const;

		/*
		デバッグ用に高さマップを画像に出力します
		*/
		void GenerateHeightImageForDebug(const PerlinNoise& perlinNoise, const float noiseBoostRatio, const std::string& filename) const;

		/*
		デバッグ用に部屋の構造をダイアグラムに出力します
		*/
		void DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& passableAisles, const std::shared_ptr<const Room>& room) const noexcept;

#if WITH_EDITOR
		/*
		デバッグ用に部屋と通路の情報をダンプします
		*/
		void DumpAisleAndRoomInfomation(const size_t index) const noexcept;
#endif

	private:
		GenerateParameter mGenerateParameter;

		std::shared_ptr<Voxel> mVoxel;

		std::list<std::shared_ptr<Room>> mRooms;
		//std::shared_ptr<Room> mStartRoom;
		//std::shared_ptr<Room> mGoalRoom;

		std::vector<int32_t> mFloorHeight;

		std::vector<std::shared_ptr<const Point>> mLeafPoints;
		std::shared_ptr<const Point> mStartPoint;
		std::shared_ptr<const Point> mGoalPoint;

		std::vector<Aisle> mAisles;

		std::function<void(const std::shared_ptr<Voxel>&)> mOnPreGenerateVoxel;
		std::function<void(const std::shared_ptr<Voxel>&)> mOnPostGenerateVoxel;

		std::function<void(const std::shared_ptr<Room>&)> mOnQueryParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnStartParts;
		std::function<void(const std::shared_ptr<Room>&)> mOnGoalParts;

		uint8_t mDistance = 0;

		Error mLastError = Error::Success;
	};
}

#include "Generator.inl"
