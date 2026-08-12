/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Voxel.h"
#include "../GenerateParameter.h"
#include "../Debug/Config.h"
#include "../Debug/Debug.h"
#include "../Helper/Crc.h"
#include "../Math/Math.h"
#include "../PathGeneration/PathFinder.h"

#if defined(BUILD_TARGET_UNREAL_ENGINE)
#include <Async/ParallelFor.h>
#endif

#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <vector>

namespace dungeon
{
	Voxel::Voxel(const GenerateParameter& parameter) noexcept
		: mGrids(std::make_unique<Grid[]>(static_cast<size_t>(parameter.GetWidth())* parameter.GetDepth()* parameter.GetHeight()))
		, mLongestStraightPath(EForceInit::ForceInitToZero)
		, mWidth(parameter.GetWidth())
		, mDepth(parameter.GetDepth())
		, mHeight(parameter.GetHeight())
	{
	}

	bool Voxel::SearchGateLocation(std::vector<CandidateLocation>& result, const size_t maxResultCount, const FIntVector& start, const Identifier& identifier, const FIntVector& goal, const bool shared) const noexcept
	{
		result.clear();

#if defined(JENKINS_FOR_DEVELOP)
		{
			const auto& startIdentifier = mGrids.get()[Index(start)].GetIdentifier();
			check(startIdentifier == identifier);
		}
#endif

		/*
		 * 最も近い位置を調べる関数
		 * 戻り値がfalseならば検索を終了する。
		 */
		auto checker = [this, &result, &goal, identifier, shared](const FIntVector& location) -> bool
		{
			// 指定位置のグリッドを取得
			const auto gridIndex = Index(location);
			const auto& grid = mGrids.get()[gridIndex];
			if (grid.GetIdentifier() != identifier)
				return false;

			if (grid.IsKindOfSlopeType())
				return true;

			// 門にできないグリッドなら次の検索へ
			if (shared)
			{
				if (grid.Is(Grid::Type::Deck) == false && grid.IsKindOfGateType() == false)
					return true;

				/*
				 * 鍵のかかった門は共有できない
				 * 鍵は門グリッドのPropsに書かれるため、共有すると誤った通路に鍵が適用される
				 * 門グリッドは部屋の識別子を保持したままなので、通路の識別子ではなくPropsで判定する
				 * 施錠される通路を先に生成しているため、この時点で鍵は書き込み済みになっている
				 */
				if (grid.IsKindOfGateType() && grid.GetProps() != Grid::Props::None)
					return true;
			}
			else
			{
				if (grid.Is(Grid::Type::Deck) == false)
					return true;
			}

			bool validLocation = false;
			for (std::uint_fast8_t i = 0; i < 4; ++i)
			{
				// 壁の生成が出来ないなら他のグリッドへ
				const Direction direction(static_cast<Direction::Index>(i));
				if (grid.IsNoWallMeshGeneration(direction))
					continue;

				const Grid& aroundGrid = Get(location + direction.GetVector());
				// 部屋の中心と同じ識別子なら他のグリッドへ
				if (aroundGrid.GetIdentifier() == identifier)
					continue;

				// 空白なら接続を許可
				if (aroundGrid.Is(Grid::Type::Empty) == true)
				{
					validLocation = true;
					break;
				}

				// 門や通路を共通で使用する場合
				if (shared)
				{
					// スロープにつながるグリッド
					// 門を上書きするとスロープを破壊するので次の検索へ
					if (aroundGrid.IsKindOfSlopeType() && (aroundGrid.GetDirection().IsNorthSouth() == direction.IsNorthSouth()))
						return true;

					// 通路なら接続を許可
					if (aroundGrid.IsKindOfAisleType())
					{
						// 鍵のかかった扉の向こう側にある通路へ口を開けると、扉を通らずに関門を越えられてしまう
						if (IsLockedAisleGridBehindDoorOf(aroundGrid, identifier))
							continue;

						validLocation = true;
						break;
					}
				}
			}

			// 門の候補として登録
			if (validLocation)
			{
				uint32_t squareDistance = PathFinder::Heuristics(location, goal);
				if (shared == true && grid.IsKindOfGateType() == false)
					squareDistance |= 0x80000000;
				result.emplace_back(squareDistance, location);
			}

			return true;
		};

		/*
		 * スタート位置周辺で門にできそうな場所を検索
		 * （スタート位置は部屋の中心）
		 * 部屋の中心から外側に向かって門を作れそうなグリッドを検索
		 * テストが成功したグリッドはresultに集められる。
		 */
		for (int32 x = start.X; x >= 0; --x)
		{
			if (!checker(FIntVector(x, start.Y, start.Z)))
				break;
			for (int32 y = start.Y - 1; y >= 0; --y)
			{
				if (!checker(FIntVector(x, y, start.Z)))
					break;
			}
			for (uint32 y = start.Y + 1; y < mDepth; ++y)
			{
				if (!checker(FIntVector(x, y, start.Z)))
					break;
			}
		}
		for (uint32 x = start.X + 1; x < mWidth; ++x)
		{
			if (!checker(FIntVector(x, start.Y, start.Z)))
				break;
			for (int32 y = start.Y - 1; y >= 0; --y)
			{
				if (!checker(FIntVector(x, y, start.Z)))
					break;
			}
			for (uint32 y = start.Y + 1; y < mDepth; ++y)
			{
				if (!checker(FIntVector(x, y, start.Z)))
					break;
			}
		}

		std::stable_sort(result.begin(), result.end(), [](const CandidateLocation& l, const CandidateLocation& r)
			{
				return l.mPriority < r.mPriority;
			}
		);

		if (result.size() > maxResultCount)
		{
			result.erase(result.begin() + maxResultCount, result.end());
		}

		return result.empty() == false;
	}

	bool Voxel::Aisle(const std::vector<CandidateLocation>& startToGoal, const std::vector<CandidateLocation>& goalToStart, const AisleParameter& aisleParameter) noexcept
	{
		// 経路の一覧を準備
		bool terminate = false;
		std::vector<Route> route;
		if (aisleParameter.mGenerateIntersections == true)
		{
			/*
			 * ドア～ドアを優先的に検索
			 * SearchGateLocationは門ではない候補に0x80000000を立てて後回しにするので、
			 * 既存の門だけを残すにはビットが立っている候補を除外する
			 */
			route.reserve(startToGoal.size() * goalToStart.size());
			for (const auto& startLocation : startToGoal)
			{
				if ((startLocation.mPriority & 0x80000000) != 0)
					continue;
				for (const auto& goalLocation : goalToStart)
				{
					if ((goalLocation.mPriority & 0x80000000) != 0)
						continue;
					route.emplace_back(startLocation.mLocation, goalLocation.mLocation);
				}
			}
			terminate = AisleImpl(route, aisleParameter);
		}

		if (terminate == false)
		{
			// 門の有無を問わず全ての組み合わせを検索
			route.clear();
			route.reserve(startToGoal.size() * goalToStart.size());
			for (const auto& startLocation : startToGoal)
			{
				for (const auto& goalLocation : goalToStart)
				{
					route.emplace_back(startLocation.mLocation, goalLocation.mLocation);
				}
			}
			terminate = AisleImpl(route, aisleParameter);
		}

		// terminateがfalseならば経路検索は失敗している
		return terminate;
	}

	bool Voxel::AisleImpl(const std::vector<Route>& route, const AisleParameter& aisleParameter) noexcept
	{
		std::mutex pathResultsMutex;
		std::map<size_t, std::shared_ptr<PathFinder::Result>> pathResults;

#if defined(BUILD_TARGET_UNREAL_ENGINE)
		//constexpr EParallelForFlags ParallelForFlags = EParallelForFlags::ForceSingleThread;
		constexpr EParallelForFlags ParallelForFlags = EParallelForFlags::Unbalanced;
		ParallelForTemplate(route.size(), [this, &route, &aisleParameter, &pathResultsMutex, &pathResults](const int32 index)
#else
		for (size_t index = 0; index < route.size(); ++index)
#endif
			{
				const Route& currentRoute = route[index];

				/*
				 * 理想的なゴール位置がゴール条件に含まれていないなら
				 * 検索できないのでエラーを表示して中断する。
				 */
				if (aisleParameter.mGoalCondition.Contains(route[index].mIdealGoal) == false)
				{
					DUNGEON_GENERATOR_ERROR(TEXT("Voxel: Task %d: include the finish line in the goal range. (%d,%d,%d)")
						, index, currentRoute.mIdealGoal.X, currentRoute.mIdealGoal.Y, currentRoute.mIdealGoal.Z);
					mLastError = Error::GoalPointIsOutsideGoalRange;
					return;
				}

				// パス検索開始
				const auto pathResult = FindAisle(currentRoute, aisleParameter, index);
				if (pathResult == nullptr)
					return;

				size_t key = pathResult->GetPathLength();
				key <<= 8;
				key |= index & 0xFF;
				{
					std::lock_guard lock(pathResultsMutex);
					pathResults[key] = pathResult;
				}
#if defined(BUILD_TARGET_UNREAL_ENGINE)
			},
			ParallelForFlags
		);
#else
		}
#endif

		if (pathResults.empty() == true)
			return false;

		const std::shared_ptr<PathFinder::Result>& pathResult = pathResults.begin()->second;

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		const size_t index = pathResults.begin()->first & 0xFF;
		const FIntVector& startLocation = pathResult->GetStartLocation();
		const FIntVector& goalLocation = pathResult->GetGoalLocation();
		DUNGEON_GENERATOR_LOG(TEXT("Voxel: Task %d: Pathfinding succeeded. (%d,%d,%d)-(%d,%d,%d)")
			, index
			, startLocation.X, startLocation.Y, startLocation.Z
			, goalLocation.X, goalLocation.Y, goalLocation.Z
		);
#endif

		// パスをグリッドに書き込む
		WriteAisleToGrid(pathResult, aisleParameter);

		// 最も長い直線を計算します
		const FIntVector2& longestStraightPath = pathResult->ComputeLongestStraightPath();
		if (mLongestStraightPath.X < longestStraightPath.X)
			mLongestStraightPath.X = longestStraightPath.X;
		if (mLongestStraightPath.Y < longestStraightPath.Y)
			mLongestStraightPath.Y = longestStraightPath.Y;

		return true;
	}

	const FIntVector2& Voxel::GetLongestStraightPath() const noexcept
	{
		return mLongestStraightPath;
	}

	/**
	 * A*による通路検索
	 * 条件によってはマルチスレッド下で実行されるのでスレッドセーフを徹底して下さい
	 * @param route				通路の開始と終了位置
	 * @param aisleParameter	通路検索パラメーター
	 * @param index				通路配列番号
	 * @return 通路検索結果。nullptrなら検索失敗。
	 */
	std::shared_ptr<PathFinder::Result> Voxel::FindAisle(const Route& route, const AisleParameter& aisleParameter, const size_t index) const noexcept
	{
		static constexpr uint32_t PriorityConnectingCost = 1;
		static constexpr uint32_t NormalConnectingCost = 2;
		static constexpr uint32_t SlopeConnectingCost = 1;

		// 開始地点だけで成立する経路は通路として扱わない
		if (IsReachedGoal(route.mStart, route.mIdealGoal.Z, aisleParameter.mGoalCondition))
			return nullptr;

		// パス検索開始
		PathFinder pathFinder;
		pathFinder.Start(route.mStart, route.mIdealGoal, PathFinder::SearchDirection::Any);
#if 0
		// 検索する最大数（おおよその数）
		int32 maximumNumberToFinding;
		{
			const FIntVector delta = route.mStart - route.mIdealGoal;
			const int32 deltaX = std::max(1, std::abs(delta.X));
			const int32 deltaY = std::max(1, std::abs(delta.Y));
			const int32 deltaZ = std::max(1, std::abs(delta.Z)) * 7; // 一段上がる（下がる）のに7グリッド必要
			maximumNumberToFinding = deltaX * deltaY * deltaZ + 2;				// 出入口の2グリッド分加算
		}
#endif
		// 最も有望なポジションを得る
		uint64_t nextKey;
		PathFinder::NodeType nextNodeType;
		uint32_t nextCost;
		FIntVector nextLocation;
		Direction nextDirection;
		PathFinder::SearchDirection nextSearchDirection;
		while (pathFinder.Pop(nextKey, nextNodeType, nextCost, nextLocation, nextDirection, nextSearchDirection))
		{
			// ゴールに到達？
			if (IsReachedGoal(nextLocation, route.mIdealGoal.Z, aisleParameter.mGoalCondition))
			{
				if (nextNodeType == PathFinder::NodeType::Aisle)
				{
					if (nextSearchDirection == PathFinder::SearchDirection::Any)
						break;
				}

				// 別の経路を探索要求
				continue;
			}

			// 水平方向に検索する
			for (uint_fast8_t i = 0; i < 4; ++i)
			{
				if (
					nextSearchDirection == PathFinder::SearchDirection::Any ||
					nextSearchDirection == static_cast<PathFinder::SearchDirection>(i))
				{
					const FIntVector& offset = Direction::GetVector(static_cast<Direction::Index>(i));
					const FIntVector openLocation = nextLocation + offset;
					const Direction direction(static_cast<Direction::Index>(i));
					if (IsPassableForAisle(openLocation, aisleParameter.mGenerateIntersections, aisleParameter) || IsReachedGoalWithDirection(openLocation, route.mIdealGoal.Z, aisleParameter.mGoalCondition, direction))
					{
						if (pathFinder.IsUsingOpenNode(openLocation) == false)
						{
							const uint32_t connectingCost = (i == nextDirection.Get()) ? PriorityConnectingCost : NormalConnectingCost;
							// 接続しない部屋の壁際を通ると、その部屋の門の候補を奪うのでコストを上げます
							const uint32_t proximityCost = GetRoomProximityCost(openLocation, aisleParameter);
							pathFinder.Open(nextKey, PathFinder::NodeType::Aisle, nextCost + connectingCost + proximityCost, openLocation, route.mIdealGoal, direction, PathFinder::SearchDirection::Any);
						}
					}
				}
			}

			// 垂直方向に検索する
			if (nextNodeType == PathFinder::NodeType::Aisle)
			{
				if (Get(nextLocation).Is(Grid::Type::Empty))
				{
					// up
					const FIntVector upstairsOpenLocationU = nextLocation + FIntVector(0, 0, 1);
					const FIntVector upstairsOpenLocationF = nextLocation + nextDirection.GetVector();
					const FIntVector upstairsOpenLocationUF = upstairsOpenLocationF + FIntVector(0, 0, 1);
					if (
						IsPassableForAisle(upstairsOpenLocationU, false, aisleParameter) && IsReachedGoal(upstairsOpenLocationU, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationU) == false &&
						IsPassableForAisle(upstairsOpenLocationF, false, aisleParameter) && IsReachedGoal(upstairsOpenLocationF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationF) == false &&
						IsPassableForAisle(upstairsOpenLocationUF, false, aisleParameter) && IsReachedGoal(upstairsOpenLocationUF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false)
					{
						// スロープは4グリッドを占有するので、部屋の壁際を避けさせます
						pathFinder.Open(nextKey, PathFinder::NodeType::Upstairs, nextCost + SlopeConnectingCost, upstairsOpenLocationUF, route.mIdealGoal, nextDirection, PathFinder::Cast(nextDirection));

						const PathNodeSwitcher::Node useNode(
							PathFinder::Hash(upstairsOpenLocationU),
							PathFinder::Hash(upstairsOpenLocationF)
						);
						pathFinder.ReserveOpenNode(upstairsOpenLocationUF, useNode);
					}

					// down
					const FIntVector downstairsOpenLocationD = nextLocation + FIntVector(0, 0, -1);
					const FIntVector downstairsOpenLocationF = nextLocation + nextDirection.GetVector();
					const FIntVector downstairsOpenLocationDF = downstairsOpenLocationF + FIntVector(0, 0, -1);
					if (
						IsPassableForAisle(downstairsOpenLocationD, false, aisleParameter) && IsReachedGoal(downstairsOpenLocationD, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationD) == false &&
						IsPassableForAisle(downstairsOpenLocationF, false, aisleParameter) && IsReachedGoal(downstairsOpenLocationF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationF) == false &&
						IsPassableForAisle(downstairsOpenLocationDF, false, aisleParameter) && IsReachedGoal(downstairsOpenLocationDF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false)
					{
						// スロープは4グリッドを占有するので、部屋の壁際を避けさせます
						pathFinder.Open(nextKey, PathFinder::NodeType::Downstairs, nextCost + SlopeConnectingCost, downstairsOpenLocationDF, route.mIdealGoal, nextDirection, PathFinder::Cast(nextDirection));

						const PathNodeSwitcher::Node useNode(
							PathFinder::Hash(downstairsOpenLocationD),
							PathFinder::Hash(upstairsOpenLocationF)
						);
						pathFinder.ReserveOpenNode(downstairsOpenLocationDF, useNode);
					}
				}
			}

#if 0
			// 検索を断念
			if (pathFinder.CloseSize() >= maximumNumberToFinding)
			{
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
				DUNGEON_GENERATOR_LOG(TEXT("Voxel: Task %d: Suspend route search. (%d,%d,%d)-(%d,%d,%d) %d/%d/%d nodes, %lf seconds"), index
					, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z
					, pathFinder.OpenSize(), pathFinder.CloseSize(), maximumNumberToFinding
					, stopwatch.Lap());
#endif
				return nullptr;
			}
#endif
		}

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
		const size_t closeNodeSize = pathFinder.CloseSize();
		const size_t openNodeSize = pathFinder.OpenSize();
#endif

		// nextLocationが実際に到達した場所
		if (!aisleParameter.mGoalCondition.Contains(nextLocation))
		{
#if WITH_EDITOR & JENKINS_FOR_DEVELOP & 0
			DUNGEON_GENERATOR_WARNING(TEXT("Voxel: Task %d: The path does not meet the goal conditions. (%d,%d,%d)-(%d,%d,%d) %d/%d nodes, %lf seconds"), index
				, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z
				, openNodeSize, closeNodeSize
				, stopwatch.Lap());
#endif
			return nullptr;
		}

		// nextLocationが実際に到達した場所
		if (!pathFinder.Commit(nextLocation))
		{
			DUNGEON_GENERATOR_WARNING(TEXT("Voxel: Task %d: Failed to generate route. (%d,%d,%d)-(%d,%d,%d)"), index
				, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z);
			return nullptr;
		}

		const std::shared_ptr<PathFinder::Result>& pathResult = pathFinder.GetResult();
		if (!pathResult || pathResult->GetPathLength() < 2)
			return nullptr;

		/*
		 * 通路を共有する時に、門グリッド周辺に同じ方向の門と通路があるなら
		 * 同じ通路の門が並ぶので今回の門は生成する必要が無い
		 */
		if (aisleParameter.mGenerateIntersections)
		{
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			if (route.mStart != pathResult->GetStartLocation())
				return nullptr;
			if (nextLocation != pathResult->GetGoalLocation())
				return nullptr;
			if (nextDirection != pathResult->GetGoalDirection())
				return nullptr;
#endif
			check(route.mStart == pathResult->GetStartLocation());
			check(nextLocation == pathResult->GetGoalLocation());
			check(nextDirection == pathResult->GetGoalDirection());

			if (CheckDoorAligned(pathResult->GetStartLocation(), pathResult->GetStartDirection(), pathResult->GetNodeTypeFromStart(1)) == true)
				pathResult->InvalidateStartLocationType();

			if (CheckDoorAligned(pathResult->GetGoalLocation(), pathResult->GetGoalDirection(), pathResult->GetNodeTypeFromGoal(1)) == true)
				pathResult->InvalidateGoalLocationType();
		}
#if 0
		// 部屋を結合する場合ドアが不要なので、短すぎる通路を失敗扱いにする
		if (mergeRooms && pathFinder.GetPathLength() <= 2)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Voxel: 探索した経路が短すぎました (%d,%d,%d)-(%d,%d,%d)"), index
				, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z);
			return false;
		}
#endif

		return pathResult;
	}

// #define ___DUNGEON_DEBUG___

	bool Voxel::CheckDoorAligned(const FIntVector& location, const Direction& direction, const PathFinder::NodeType nodeType) const noexcept
	{
		if (nodeType != PathFinder::NodeType::Aisle)
			return false;

		if (direction.IsNorthSouth())
		{
			// 南北に向いた門（東西に並んだ門）
			const Grid& w = Get(location.X - 1, location.Y, location.Z);
			const Grid& e = Get(location.X + 1, location.Y, location.Z);

#if defined(___DUNGEON_DEBUG___)
			const auto wType = w.GetType();
			const auto eType = e.GetType();
			const auto wDirection = w.GetDirection();
			const auto eDirection = e.GetDirection();
#endif

			if (w.IsKindOfGateType() && w.GetDirection().IsNorthSouth() == true)
			{
				const Grid& n = Get(location.X - 1, location.Y - 1, location.Z);
				const Grid& s = Get(location.X - 1, location.Y + 1, location.Z);

#if defined(___DUNGEON_DEBUG___)
				const auto nType = n.GetType();
				const auto sType = s.GetType();
#endif

				if (n.IsKindOfAisleType() || s.IsKindOfAisleType())
				{
					return true;
				}
			}
			else if (e.IsKindOfGateType() && e.GetDirection().IsNorthSouth() == true)
			{
				const Grid& n = Get(location.X + 1, location.Y - 1, location.Z);
				const Grid& s = Get(location.X + 1, location.Y + 1, location.Z);

#if defined(___DUNGEON_DEBUG___)
				const auto nType = n.GetType();
				const auto sType = s.GetType();
#endif

				if (n.IsKindOfAisleType() || s.IsKindOfAisleType())
				{
					return true;
				}
			}
		}
		else
		{
			// 東西に向いた門（南北に並んだ門）
			const Grid& n = Get(location.X, location.Y - 1, location.Z);
			const Grid& s = Get(location.X, location.Y + 1, location.Z);

#if defined(___DUNGEON_DEBUG___)
			const auto nType = n.GetType();
			const auto sType = s.GetType();
			const auto nDirection = n.GetDirection();
			const auto sDirection = s.GetDirection();
#endif

			if (n.IsKindOfGateType() && n.GetDirection().IsNorthSouth() == false)
			{
				const Grid& w = Get(location.X - 1, location.Y - 1, location.Z);
				const Grid& e = Get(location.X + 1, location.Y - 1, location.Z);

#if defined(___DUNGEON_DEBUG___)
				const auto wType = w.GetType();
				const auto eType = e.GetType();
#endif

				if (w.IsKindOfAisleType() || e.IsKindOfAisleType())
				{
					return true;
				}
			}
			else if (s.IsKindOfGateType() && s.GetDirection().IsNorthSouth() == false)
			{
				const Grid& w = Get(location.X + 1, location.Y + 1, location.Z);
				const Grid& e = Get(location.X - 1, location.Y + 1, location.Z);

#if defined(___DUNGEON_DEBUG___)
				const auto wType = w.GetType();
				const auto eType = e.GetType();
#endif

				if (w.IsKindOfAisleType() || e.IsKindOfAisleType())
				{
					return true;
				}
			}
		}
		return false;
	}
#undef ___DUNGEON_DEBUG___

	void Voxel::WriteAisleToGrid(const std::shared_ptr<PathFinder::Result>& pathResult, const AisleParameter& aisleParameter) const
	{
		check(pathResult);

		// パスをグリッドに反映します
		pathResult->Path([this, &aisleParameter](const PathFinder::NodeType nodeType, const FIntVector& location, const Direction& direction)
			{
				const size_t index = Index(location);
				Grid& grid = mGrids.get()[index];

				Grid::Type cellType;
				switch (nodeType)
				{
				case PathFinder::NodeType::UpSpace:
					cellType = Grid::Type::UpSpace;
					grid.MergeAisle(aisleParameter.mGenerateIntersections);
					grid.SetDepthRatioFromStart(aisleParameter.mDepthRatioFromStart);
					break;

				case PathFinder::NodeType::DownSpace:
					cellType = Grid::Type::DownSpace;
					grid.MergeAisle(aisleParameter.mGenerateIntersections);
					grid.SetDepthRatioFromStart(aisleParameter.mDepthRatioFromStart);
					break;

				case PathFinder::NodeType::Downstairs:
				case PathFinder::NodeType::Upstairs:
					cellType = Grid::Type::Slope;
					grid.MergeAisle(aisleParameter.mGenerateIntersections);
					grid.SetDepthRatioFromStart(aisleParameter.mDepthRatioFromStart);
					break;

				case PathFinder::NodeType::Stairwell:
					cellType = Grid::Type::Stairwell;
					grid.MergeAisle(aisleParameter.mGenerateIntersections);
					grid.SetDepthRatioFromStart(aisleParameter.mDepthRatioFromStart);
					break;

				case PathFinder::NodeType::Gate:
					cellType = Grid::Type::Gate;
					break;

				case PathFinder::NodeType::Aisle:
					cellType = Grid::Type::Aisle;
					grid.MergeAisle(aisleParameter.mGenerateIntersections);
					grid.SetDepthRatioFromStart(aisleParameter.mDepthRatioFromStart);
					break;

				case PathFinder::NodeType::Invalid:
				default:
					return;
				}

				// 識別子が無効なら通路として設定する
				if (grid.IsInvalidIdentifier())
				{
					grid.SetIdentifier(aisleParameter.mIdentifier);
					grid.SetZoneIndex(aisleParameter.mZoneIndex);
				}

				grid.SetType(cellType);
				grid.SetDirection(direction);
			}
		);

		/*
		 * 必要であれば、奥の部屋の門に鍵をかける
		 * 施錠される通路は交差点を生成しないため、門は必ずこの通路専用になっている
		 */
		if (aisleParameter.mUniqueLocked || aisleParameter.mLocked)
		{
			const FIntVector& startLocation = pathResult->GetStartLocation();
			Grid grid = Get(startLocation.X, startLocation.Y, startLocation.Z);
			check(grid.GetProps() == Grid::Props::None);
			if (grid.GetProps() == Grid::Props::None)
			{
				if (aisleParameter.mUniqueLocked)
					grid.SetProps(Grid::Props::UniqueLock);
				else if (aisleParameter.mLocked)
					grid.SetProps(Grid::Props::Lock);
				Set(startLocation.X, startLocation.Y, startLocation.Z, grid);

				/*
				 * 鍵をかけた門を持つ部屋を覚えます
				 * この部屋から見ると通路は扉の向こう側にあり、反対側の部屋から見ると扉の手前側にあります
				 */
				mLockedAisleDoorRooms[aisleParameter.mIdentifier] = static_cast<Identifier::IdentifierType>(grid.GetIdentifier());
			}
		}
	}

	const Grid& Voxel::Get(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept
	{
		if (mWidth <= x || mDepth <= y || mHeight <= z)
		{
			static const Grid outOfBounds(Grid::Type::OutOfBounds);
			return outOfBounds;
		}

		const size_t index = Index(x, y, z);
		return mGrids.get()[index];
	}

	Grid& Voxel::GetRef(const size_t index) const noexcept
	{
		check(index < static_cast<size_t>(mWidth) * mDepth * mHeight);
		return mGrids.get()[index];
	}

	void Voxel::Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) const noexcept
	{
		if (x < mWidth && y < mDepth && z < mHeight)
		{
			const size_t index = Index(x, y, z);
			mGrids.get()[index] = grid;
		}
	}

	void Voxel::Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) const noexcept
	{
		FIntVector min_;
		min_.X = math::Clamp(min.X, 0, static_cast<int32_t>(mWidth));
		min_.Y = math::Clamp(min.Y, 0, static_cast<int32_t>(mDepth));
		min_.Z = math::Clamp(min.Z, 0, static_cast<int32_t>(mHeight));

		FIntVector max_;
		max_.X = math::Clamp(max.X, 0, static_cast<int32_t>(mWidth));
		max_.Y = math::Clamp(max.Y, 0, static_cast<int32_t>(mDepth));
		max_.Z = math::Clamp(max.Z, 0, static_cast<int32_t>(mHeight));

		if (min_.X > max_.X) std::swap(min_.X, max_.X);
		if (min_.Y > max_.Y) std::swap(min_.Y, max_.Y);
		if (min_.Z > max_.Z) std::swap(min_.Z, max_.Z);

		// 床を塗りつぶす
		for (int32_t y = min_.Y; y < max_.Y; ++y)
		{
			for (int32_t x = min_.X; x < max_.X; ++x)
			{
				const size_t minIndex = Index(x, y, min_.Z);
				mGrids.get()[minIndex] = floorGrid;
			}
		}

		// 中を塗りつぶす
		for (int32_t z = min_.Z + 1; z < max_.Z; ++z)
		{
			for (int32_t y = min_.Y; y < max_.Y; ++y)
			{
				for (int32_t x = min_.X; x < max_.X; ++x)
				{
					const size_t index = Index(x, y, z);
					mGrids.get()[index] = fillGrid;
				}
			}
		}
	}

	bool Voxel::IsPassable(const FIntVector& location, const bool includeAisle) const noexcept
	{
		// 範囲内？
		if (Contain(location) == false)
			return false;

		// 進入先グリッドを取得
		const size_t index = Index(location);
		const auto& grid = mGrids.get()[index];

		// 通路グリッドも通行可能なら判定に含める
		if (includeAisle && grid.Is(Grid::Type::Aisle))
			return true;

		// 空きグリッド？
		return grid.Is(Grid::Type::Empty);
	}

	bool Voxel::IsPassableForAisle(const FIntVector& location, const bool includeAisle, const AisleParameter& aisleParameter) const noexcept
	{
		if (IsPassable(location, includeAisle) == false)
			return false;

		/*
		 * 施錠される通路の廊下は通り抜けさせない
		 * 通り抜けられると、鍵付き扉を通らずに関門の向こう側へ出られてしまう
		 */
		if (includeAisle && IsLockedAisleGrid(Get(location)))
			return false;

		return IsGateApproachAvailable(location, aisleParameter);
	}

	bool Voxel::IsGateApproachAvailable(const FIntVector& location, const AisleParameter& aisleParameter) const noexcept
	{
		if (mGateApproachReservations.empty())
			return true;

		const auto reservation = mGateApproachReservations.find(Index(location));
		if (reservation == mGateApproachReservations.end())
			return true;

		// この通路が接続する部屋のために確保したグリッドならば通行できます
		for (const Identifier::IdentifierType owner : reservation->second)
		{
			if (owner == aisleParameter.mStartRoomIdentifier || owner == aisleParameter.mGoalRoomIdentifier)
				return true;
		}

		return false;
	}

	void Voxel::CollectGateApproachLocations(std::vector<FIntVector>& result, const FIntRect& rect, const int32 groundZ, const Identifier& identifier) const noexcept
	{
		result.clear();

		for (int32 y = rect.Min.Y; y < rect.Max.Y; ++y)
		{
			for (int32 x = rect.Min.X; x < rect.Max.X; ++x)
			{
				const FIntVector location(x, y, groundZ);
				if (Contain(location) == false)
					continue;

				// SearchGateLocationと同じ条件で門にできるグリッドを調べます
				const Grid& grid = mGrids.get()[Index(location)];
				if (grid.GetIdentifier() != identifier)
					continue;
				if (grid.Is(Grid::Type::Deck) == false)
					continue;

				for (std::uint_fast8_t i = 0; i < 4; ++i)
				{
					// 壁の生成が出来ない方向には門を置けません
					const Direction direction(static_cast<Direction::Index>(i));
					if (grid.IsNoWallMeshGeneration(direction))
						continue;

					// 門から通路が出ていく先が空白でなければ門を置けません
					const FIntVector approachLocation = location + direction.GetVector();
					if (Contain(approachLocation) == false)
						continue;
					if (Get(approachLocation).Is(Grid::Type::Empty) == false)
						continue;

					result.emplace_back(approachLocation);
				}
			}
		}
	}

	void Voxel::ReserveGateApproachLocations(const std::vector<FIntVector>& locations, const Identifier& identifier) noexcept
	{
		const Identifier::IdentifierType owner = static_cast<Identifier::IdentifierType>(identifier);
		for (const FIntVector& location : locations)
		{
			if (Contain(location) == false)
				continue;

			std::vector<Identifier::IdentifierType>& owners = mGateApproachReservations[Index(location)];
			if (std::find(owners.begin(), owners.end(), owner) == owners.end())
				owners.emplace_back(owner);
		}
	}

	void Voxel::ReleaseGateApproachLocations() noexcept
	{
		mGateApproachReservations.clear();
	}

	void Voxel::SetLockedAisleIdentifiers(std::unordered_set<Identifier::IdentifierType>&& identifiers) noexcept
	{
		mLockedAisleIdentifiers = std::move(identifiers);
	}

	void Voxel::AddFailedAisleEndpoints(const FIntVector& start, const FIntVector& goal) noexcept
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		mFailedAisleEndpoints.emplace_back(start, goal);
#else
		(void)start;
		(void)goal;
#endif
	}

	void Voxel::ClearLockedAisleIdentifiers() noexcept
	{
		mLockedAisleIdentifiers.clear();
		mLockedAisleDoorRooms.clear();
	}

	void Voxel::SetRoomGateScarcity(std::unordered_map<Identifier::IdentifierType, uint32_t>&& scarcity) noexcept
	{
		mRoomGateScarcity = std::move(scarcity);
	}

	void Voxel::ClearRoomGateScarcity() noexcept
	{
		mRoomGateScarcity.clear();
	}

	/**
	 * Returns the extra cost of running along the wall of a room this aisle does not connect to.
	 * A grid next to a room can become that room's gate, so occupying it takes a gate candidate away.
	 * The cost is a soft penalty: when there is no other way around, the search still takes the grid.
	 * この通路が接続しない部屋の壁際を通る事に対する追加コストを返します。
	 * 部屋に隣接するグリッドはその部屋の門になり得るため、占有すると門の候補を一つ奪います。
	 * これは重み付けであり通行禁止ではないので、他に道が無ければ経路探索はそのグリッドを選びます。
	 */
	uint32_t Voxel::GetRoomProximityCost(const FIntVector& location, const AisleParameter& aisleParameter) const noexcept
	{
		if (mRoomGateScarcity.empty())
			return 0;

		uint32_t cost = 0;
		for (uint_fast8_t i = 0; i < 4; ++i)
		{
			const FIntVector neighborLocation = location + Direction::GetVector(static_cast<Direction::Index>(i));
			if (Contain(neighborLocation) == false)
				continue;

			const Grid& neighborGrid = mGrids.get()[Index(neighborLocation)];
			if (neighborGrid.Is(Grid::Type::Deck) == false && neighborGrid.Is(Grid::Type::Floor) == false)
				continue;

			// この通路が接続する部屋へは近づく必要があるので対象外です
			const Identifier::IdentifierType identifier = static_cast<Identifier::IdentifierType>(neighborGrid.GetIdentifier());
			if (identifier == aisleParameter.mStartRoomIdentifier || identifier == aisleParameter.mGoalRoomIdentifier)
				continue;

			const auto scarcity = mRoomGateScarcity.find(identifier);
			if (scarcity == mRoomGateScarcity.end())
				continue;

			// 複数の部屋に接していても二重には数えません
			if (cost < scarcity->second)
				cost = scarcity->second;
		}
		return cost;
	}

	/**
	 * Returns whether the grid belongs to a locked aisle whose door is held by the given room.
	 * The lock is written on one gate only, so the corridor sits behind the door for the room that
	 * holds it and in front of the door for the room at the other end. Opening a second gate is only
	 * a bypass for the room that holds the door; the room at the other end already reaches the
	 * corridor through its own gate, so a second opening adds nothing a player could exploit.
	 * 指定した部屋が鍵をかけた門を持つ施錠通路のグリッドかを返します。
	 * 鍵は片方の門にしか書かれないため、通路は鍵を持つ部屋から見れば扉の向こう側、
	 * 反対側の部屋から見れば扉の手前側にあります。2つ目の門が迂回路になるのは鍵を持つ部屋だけで、
	 * 反対側の部屋は自身の門で既に通路とつながっているため、口を増やしても迂回にはなりません。
	 * @param[in]	grid				判定するグリッド
	 * @param[in]	roomIdentifier		門を開けようとしている部屋の識別子
	 * @return		扉の向こう側の通路ならばtrue
	 */
	bool Voxel::IsLockedAisleGridBehindDoorOf(const Grid& grid, const Identifier& roomIdentifier) const noexcept
	{
		if (IsLockedAisleGrid(grid) == false)
			return false;

		const auto doorRoom = mLockedAisleDoorRooms.find(static_cast<Identifier::IdentifierType>(grid.GetIdentifier()));
		if (doorRoom == mLockedAisleDoorRooms.end())
		{
			// まだ鍵を書いていない通路は、どちら側になるか分からないので拒否します
			return true;
		}

		return doorRoom->second == static_cast<Identifier::IdentifierType>(roomIdentifier);
	}

	bool Voxel::IsLockedAisleGrid(const Grid& grid) const noexcept
	{
		if (mLockedAisleIdentifiers.empty())
			return false;

		return mLockedAisleIdentifiers.find(static_cast<Identifier::IdentifierType>(grid.GetIdentifier())) != mLockedAisleIdentifiers.end();
	}

	bool Voxel::IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept
	{
		const bool reachTheGoal =
			(goalAltitude == location.Z) &&
			(goalCondition.Contains(location) == true);
		return reachTheGoal;
	}

	bool Voxel::IsReachedGoalWithDirection(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition, const Direction& enteringDirection) const noexcept
	{
		// ゴールの高さが違う？
		if (goalAltitude != location.Z)
			return false;

		// ゴールの範囲外？
		if (goalCondition.Contains(location) == false)
			return false;

		// 進入先グリッドを取得
		const Grid& grid = Get(location);

		// 進入先から現在位置への壁が生成禁止なら進入できない
		if (grid.IsNoWallMeshGeneration(enteringDirection.Inverse().Get()) == true)
		{
			return false;
		}

		// 部屋の一階部分の属性なら進入許可
		return grid.Is(Grid::Type::Deck);
	}


	void Voxel::SetFloor(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetFloor(enable);
		}
	}

	void Voxel::SetCeiling(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetCeiling(enable);
		}
	}

	void Voxel::SetNorthWall(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetNorthWall(enable);
		}
	}

	void Voxel::SetSouthWall(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetSouthWall(enable);
		}
	}

	void Voxel::SetEastWall(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetEastWall(enable);
		}
	}

	void Voxel::SetWestWall(const FIntVector& position, const bool enable) const noexcept
	{
		if (Contain(position))
		{
			const auto index = Index(position);
			GetRef(index).SetWestWall(enable);
		}
	}


	bool Voxel::HasFloor(const FIntVector& position) const noexcept
	{
		return Get(position).HasFloor();
	}

	bool Voxel::HasCeiling(const FIntVector& position) const noexcept
	{
		return Get(position).HasCeiling();
	}

	bool Voxel::HasNorthWall(const FIntVector& position) const noexcept
	{
		return Get(position).HasNorthWall();
	}

	bool Voxel::HasSouthWall(const FIntVector& position) const noexcept
	{
		return Get(position).HasSouthWall();
	}

	bool Voxel::HasEastWall(const FIntVector& position) const noexcept
	{
		return Get(position).HasEastWall();
	}

	bool Voxel::HasWestWall(const FIntVector& position) const noexcept
	{
		return Get(position).HasWestWall();
	}

	uint32_t Voxel::CalculateCRC32(const uint32_t hash) const noexcept
	{
		const size_t size = sizeof(Grid) * mWidth * mDepth * mHeight;
		const uint32_t crc32 = GenerateCrc32FromData(mGrids.get(), size, hash);
		return crc32;
	}

	void Voxel::DrawImageForDebug(const bmp::Canvas& canvas, const std::vector<int32_t>& floorHeights) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		const int32_t offsetZ = GetDepth() + 1;

		// XY平面を描画
		for (uint32_t z = 0; z < GetHeight(); ++z)
		{
			for (uint32_t y = 0; y < GetDepth(); ++y)
			{
				for (uint32_t x = 0; x < GetWidth(); ++x)
				{
					bmp::RGBCOLOR color;

					const Grid& grid = Get(x, y, z);
					if (grid.IsKindOfSpatialType() == true)
						continue;
					if (grid.IsKindOfRoomType() == false)
					{
						color = AisleColor;
					}
					else
					{
						float ratio = static_cast<float>(z) / static_cast<float>(GetHeight());
						if (ratio <= std::numeric_limits<float>::epsilon())
							ratio = std::numeric_limits<float>::epsilon();
						color.rgbRed = BaseDarkColor.rgbRed + (BaseLightColor.rgbRed - BaseDarkColor.rgbRed) * ratio;
						color.rgbGreen = BaseDarkColor.rgbGreen + (BaseLightColor.rgbGreen - BaseDarkColor.rgbGreen) * ratio;
						color.rgbBlue = BaseDarkColor.rgbBlue + (BaseLightColor.rgbBlue - BaseDarkColor.rgbBlue) * ratio;
					}

					canvas.Rectangle(
						Scale(x),
						Scale(y),
						Scale(x + 1),
						Scale(y + 1),
						color
					);
				}
			}
		}

		// XZ平面を描画
		for (uint32_t y = 0; y < GetDepth(); ++y)
		{
			for (uint32_t z = 0; z < GetHeight(); ++z)
			{
				for (uint32_t x = 0; x < GetWidth(); ++x)
				{
					bmp::RGBCOLOR color;

					const Grid& grid = Get(x, y, z);
					if (grid.IsKindOfSpatialType() == true)
						continue;
					if (grid.IsKindOfRoomType() == false)
					{
						color = AisleColor;
					}
					else
					{
						float ratio = static_cast<float>(y) / static_cast<float>(GetDepth());
						if (ratio <= std::numeric_limits<float>::epsilon())
							ratio = std::numeric_limits<float>::epsilon();
						color.rgbRed = BaseDarkColor.rgbRed + (BaseLightColor.rgbRed - BaseDarkColor.rgbRed) * ratio;
						color.rgbGreen = BaseDarkColor.rgbGreen + (BaseLightColor.rgbGreen - BaseDarkColor.rgbGreen) * ratio;
						color.rgbBlue = BaseDarkColor.rgbBlue + (BaseLightColor.rgbBlue - BaseDarkColor.rgbBlue) * ratio;
					}
					canvas.Rectangle(
						Scale(x),
						Scale(offsetZ + GetHeight() - z - 1),
						Scale(x + 1),
						Scale(offsetZ + GetHeight() - z),
						color
					);
				}
			}
		}

		// グリッドを描画
		{
			for (uint32_t x = 0; x < GetWidth(); ++x)
			{
				canvas.VerticalLine(
					Scale(x),
					Scale(0),
					Scale(GetDepth()),
					x == 0 ? OriginYColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);

				canvas.VerticalLine(
					Scale(x),
					Scale(offsetZ + GetHeight()),
					Scale(offsetZ),
					x == 0 ? OriginZColor : x % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (uint32_t y = 0; y < GetDepth(); ++y)
			{
				canvas.HorizontalLine(
					Scale(0),
					Scale(GetWidth()),
					Scale(y),
					y == 0 ? OriginXColor : y % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (uint32_t z = 0; z < GetHeight(); ++z)
			{
				canvas.HorizontalLine(
					Scale(0),
					Scale(GetWidth()),
					Scale(offsetZ + GetHeight() - z),
					z == 0 ? OriginXColor : z % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}
		}

		// XZ側面図に確定した階層位置を、1グリッド3点のゴールドの点線で描画
		{
			static constexpr int32_t DotCountPerGrid = 3;

			for (const int32_t floorHeight : floorHeights)
			{
				if (floorHeight < 0 || floorHeight > static_cast<int32_t>(GetHeight()))
					continue;

				const int32_t lineY = static_cast<int32_t>(Scale(offsetZ + GetHeight() - floorHeight));
				for (uint32_t gridX = 0; gridX < GetWidth(); ++gridX)
				{
					const int32_t cellLeft = static_cast<int32_t>(Scale(gridX));
					const int32_t cellWidth = static_cast<int32_t>(Scale(gridX + 1)) - cellLeft;
					for (int32_t dotIndex = 0; dotIndex < DotCountPerGrid; ++dotIndex)
					{
						const int32_t x = cellLeft + (dotIndex + 1) * cellWidth / (DotCountPerGrid + 1);
						canvas.Put(x, lineY, FloorLevelColor);
					}
				}
			}
		}

		// 各投影図の軸名を3x5ピクセルフォントの2倍サイズで描画
		{
			static constexpr int32_t AxisLabelScale = 2;
			static constexpr std::array<uint8_t, 5> XLabel = { 0b101, 0b101, 0b010, 0b101, 0b101 };
			static constexpr std::array<uint8_t, 5> YLabel = { 0b101, 0b101, 0b010, 0b010, 0b010 };
			static constexpr std::array<uint8_t, 5> ZLabel = { 0b111, 0b001, 0b010, 0b100, 0b111 };

			const auto drawAxisLabel = [&canvas](
				const int32_t left,
				const int32_t top,
				const std::array<uint8_t, 5>& label,
				const bmp::RGBCOLOR color)
			{
				for (int32_t y = 0; y < static_cast<int32_t>(label.size()); ++y)
				{
					for (int32_t x = 0; x < 3; ++x)
					{
						if ((label[y] & (1 << (2 - x))) != 0)
						{
							for (int32_t offsetY = 0; offsetY < AxisLabelScale; ++offsetY)
							{
								for (int32_t offsetX = 0; offsetX < AxisLabelScale; ++offsetX)
								{
									canvas.Put(
										left + x * AxisLabelScale + offsetX,
										top + y * AxisLabelScale + offsetY,
										color
									);
								}
							}
						}
					}
				}
			};

			const int32_t rightLabelX = std::max(2, static_cast<int32_t>(Scale(GetWidth())) - 8);
			drawAxisLabel(rightLabelX, 2, XLabel, OriginXColor);
			drawAxisLabel(2, std::max(2, static_cast<int32_t>(Scale(GetDepth())) - 12), YLabel, OriginYColor);
			drawAxisLabel(
				rightLabelX,
				std::max(2, static_cast<int32_t>(Scale(offsetZ + GetHeight())) - 12),
				XLabel,
				OriginXColor
			);
			drawAxisLabel(2, static_cast<int32_t>(Scale(offsetZ)) + 2, ZLabel, OriginZColor);
		}

		/*
		 * 生成できなかった通路の両端を、通常とは別の色で最後に描きます。
		 * 失敗した通路はグリッドを持たないため、結ぶはずだった位置を示す事しかできません。
		 * 外枠を二重に描いて、周囲のグリッドの色に埋もれないようにします。
		 */
		for (const auto& endpoints : mFailedAisleEndpoints)
		{
			for (const FIntVector& location : { endpoints.first, endpoints.second })
			{
				const int32_t left = static_cast<int32_t>(Scale(location.X));
				const int32_t top = static_cast<int32_t>(Scale(location.Y));
				const int32_t right = static_cast<int32_t>(Scale(location.X + 1));
				const int32_t bottom = static_cast<int32_t>(Scale(location.Y + 1));
				canvas.Rectangle(left, top, right, bottom, FailedAisleColor);
				canvas.Frame(left - 1, top - 1, right + 1, bottom + 1, FailedAisleColor);

				// XZ平面にも同じ位置を描きます
				const int32_t sectionTop = static_cast<int32_t>(Scale(offsetZ + GetHeight() - 1 - location.Z));
				const int32_t sectionBottom = static_cast<int32_t>(Scale(offsetZ + GetHeight() - location.Z));
				canvas.Rectangle(left, sectionTop, right, sectionBottom, FailedAisleColor);
			}
		}
#else
		(void)canvas;
		(void)floorHeights;
#endif
	}

	void Voxel::GenerateImageForDebug(const std::string& filename, const std::vector<int32_t>& floorHeights) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		// 空間のサイズを設定
		const bmp::Canvas canvas(Scale(GetWidth()), Scale(GetDepth() + 1 + GetHeight()));
		DrawImageForDebug(canvas, floorHeights);
		canvas.Write(dungeon::GetDebugDirectoryString() + filename);
#else
		(void)filename;
		(void)floorHeights;
#endif
	}

	void Voxel::GenerateImageForArtifact(const std::string& filePath, const std::vector<int32_t>& floorHeights) const
	{
#if defined(DEBUG_GENERATE_ARTIFACT_FILE)
		/*
		 * 成果物はデバッグ画像と同じ絵ですが、生成の度に削除されるデバッグディレクトリとは別の場所へ残します。
		 */
		const bmp::Canvas canvas(Scale(GetWidth()), Scale(GetDepth() + 1 + GetHeight()));
		DrawImageForDebug(canvas, floorHeights);
		canvas.Write(filePath);
#else
		(void)filePath;
		(void)floorHeights;
#endif
	}
}
