/**
ダンジョン生成ヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once 
#include "Aisle.h"
#include "GenerateParameter.h"
#include "Room.h"
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <thread>
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

	スレッドを生成してダンジョンを生成します。
	IsGenerated以外のメンバー関数を実行すると生成完了までブロックします。
	*/
	class Generator : public std::enable_shared_from_this<Generator>
	{
	public:
		enum class Error : uint8_t
		{
			Success,
			TriangulationFailed,
			RouteSearchFailed,

			// from Voxel class
			___StartVoxelError,
			GoalPointIsOutsideGoalRange,
		};

	public:
		/**
		コンストラクタ
		*/
		Generator() noexcept;

		/**
		デストラクタ
		*/
		virtual ~Generator() noexcept;

		/**
		生成
		スレッドを生成してダンジョンを生成します。
		IsGenerated以外のメンバー関数を実行すると生成完了までブロックします。

		\param[in]	parameter	生成パラメータ
		*/
		void Generate(const GenerateParameter& parameter) noexcept;

		/**
		生成が完了したか？
		\return		trueならば生成完了
		*/
		bool IsGenerated() const noexcept;

		/**
		生成が完了まで待つ
		*/
		void WaitGenerate() const noexcept;

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
		void ForEach(std::function<void(const std::shared_ptr<Room>&)> func) noexcept
		{
			WaitGenerate();

			for (const auto& room : mRooms)
			{
				func(room);
			}
		}

		/**
		生成された部屋を参照します
		*/
		void ForEach(std::function<void(const std::shared_ptr<const Room>&)> func) const noexcept
		{
			WaitGenerate();

			for (const auto& room : mRooms)
			{
				func(room);
			}
		}

		// 深度による検索
		std::vector<std::shared_ptr<Room>> FindByDepth(const uint8_t depth) const noexcept;

		// ブランチによる検索
		std::vector<std::shared_ptr<Room>> FindByBranch(const uint8_t branchId) const noexcept;

		// 到達可能な部屋を検索
		std::vector<std::shared_ptr<Room>> FindByRoute(const std::shared_ptr<Room>& startRoom) const noexcept;

	private:
		void FindByRoute(std::vector<std::shared_ptr<Room>>& result, std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<const Room>& room) const noexcept;

	public:
		////////////////////////////////////////////////////////////////////////////////////////////
		// Aisle
		void EachAisle(std::function<void(const Aisle& edge)> func) const noexcept
		{
			WaitGenerate();

			for (const auto& aisle : mAisles)
			{
				func(aisle);
			}
		}

		// 部屋に接続している通路を検索
		void FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(Aisle& edge)> func) noexcept
		{
			WaitGenerate();

			for (auto& aisle : mAisles)
			{
				const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (room == room0 || room == room1)
				{
					if (func(aisle))
						break;
				}
			}
		} 

		void FindAisle(const std::shared_ptr<const Room>& room, std::function<bool(const Aisle& edge)> func) const noexcept
		{
			WaitGenerate();

			for (const auto& aisle : mAisles)
			{
				const auto& room0 = aisle.GetPoint(0)->GetOwnerRoom();
				const auto& room1 = aisle.GetPoint(1)->GetOwnerRoom();
				if (room == room0 || room == room1)
				{
					if (func(aisle))
						break;
				}
			}
		}

		void OnQueryParts(std::function<void(const std::shared_ptr<const Room>&)> func) noexcept
		{
			mQueryParts = func;
		}


		////////////////////////////////////////////////////////////////////////////////////////////
		// Point
		/**
		位置から部屋を検索します
		最初にヒットした部屋を返します
		\param[in]	point		検索位置
		\return		nullptrなら検索失敗
		*/
		std::shared_ptr<Room> Find(const Point& point) const noexcept;

		/**
		位置から部屋を検索します
		検索位置を含むすべての部屋を返します
		\param[in]	point		検索位置
		\return		コンテナのサイズが0なら検索失敗
		*/
		std::vector<std::shared_ptr<Room>> FindAll(const Point& point) const noexcept;

		/**
		開始地点にふさわしい点を取得します
		\return		開始地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetStartPoint() const noexcept
		{
			return mStartPoint;
		}

		/**
		ゴール地点にふさわしい点を取得します
		\return		ゴール地点にふさわしい点
		*/
		const std::shared_ptr<const Point>& GetGoalPoint() const noexcept
		{
			return mGoalPoint;
		}

		/**
		行き止まりの点を更新します
		\param[in]	func	点を元に更新する関数
		*/
		void EachLeafPoint(std::function<void(const std::shared_ptr<const Point>& point)> func) const noexcept
		{
			WaitGenerate();

			for (auto& point : mLeafPoints)
			{
				func(point);
			}
		}







		void DumpRoomDiagram(const std::string& path) const noexcept;
		void DumpRoomDiagram(std::ofstream& stream, std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<const Room>& room) const noexcept;

		void DumpAisle(const std::string& path) const noexcept;

		void Branch() noexcept;
		void Branch(std::unordered_set<const Aisle*>& generatedEdges, const std::shared_ptr<Room>& room, uint8_t& branchId) noexcept;

		uint8_t GetDeepestDepthFromStart() const noexcept
		{
			return mDistance;
		}

	private:
		/**
		生成
		*/
		void GenerateImpl() noexcept;

		/**
		部屋の生成
		*/
		void GenerateRooms(const GenerateParameter& parameter) noexcept;

		/**
		部屋の重なりを解消します
		*/
		void SeparateRooms(const GenerateParameter& parameter) noexcept;

		/**
		全ての部屋が収まるように空間を拡張します
		*/
		void ExpandSpace(GenerateParameter& parameter) noexcept;

		/**
		重複した部屋や範囲外の部屋を除去をします
		*/
		void RemoveInvalidRooms(const GenerateParameter& parameter) noexcept;

		/**
		通路の抽出
		*/
		void ExtractionAisles(const GenerateParameter& parameter) noexcept;

		/**
		ボクセル情報を生成
		*/
		void GenerateVoxel(const GenerateParameter& parameter) noexcept;

		/**
		通路の生成
		*/
		void GenerateAisle(const MinimumSpanningTree& minimumSpanningTree) noexcept;

		/**
		幅と奥行きを指定した中心から方向ベクターが指す接点までの距離を計算します
		\param[in]		width		矩形の幅
		\param[in]		depth		矩形の奥行き
		\param[in]		direction	方向ベクトル（単位化しないで下さい）
		\return			中心から接点までの距離
		*/
		float GetDistanceCenterToContact(const float width, const float depth, const FVector& direction, const float margin = 0.f) const noexcept;

		/**
		リセット
		*/
		void Reset();

	private:
		GenerateParameter mGenerateParameter;

		std::shared_ptr<Voxel> mVoxel;
		std::vector<std::shared_ptr<Room>> mRooms;

		std::vector<std::shared_ptr<const Point>> mLeafPoints;
		std::shared_ptr<const Point> mStartPoint;
		std::shared_ptr<const Point> mGoalPoint;

		std::vector<Aisle> mAisles;

		std::function<void(const std::shared_ptr<const Room>&)> mQueryParts;

		std::thread mGenerateThread;
		std::promise<void> mGeneratePromise;
		std::future<void> mGenerateFuture;
		std::atomic_bool mGenerated = {false};

		uint8_t mDistance = 0;

		Error mLastError = Error::Success;
	};
}

#include "Generator.inl"
