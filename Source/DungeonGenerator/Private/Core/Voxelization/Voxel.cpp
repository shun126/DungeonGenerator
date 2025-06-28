/**
グリッドに関するソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Voxel.h"
#include "../GenerateParameter.h"
#include "../Debug/Config.h"
#include "../Debug/Debug.h"
#include "../Helper/Crc.h"
#include "../Helper/Stopwatch.h"
#include "../Math/Math.h"
#include "../PathGeneration/PathFinder.h"

#if defined(BUILD_TARGET_UNREAL_ENGINE)
#include <Async/ParallelFor.h>
#endif

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

		std::sort(result.begin(), result.end(), [](const CandidateLocation& l, const CandidateLocation& r)
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
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		Stopwatch stopwatch;
#endif

		// 経路の一覧を準備
		bool terminate = false;
		std::vector<Route> route;
		if (aisleParameter.mGenerateIntersections == true)
		{
			// ドア～ドアを優先的に検索
			route.reserve(startToGoal.size() * goalToStart.size());
			for (const auto& startLocation : startToGoal)
			{
				if ((startLocation.mPriority & 0x80000000) == 0)
					continue;
				for (const auto& goalLocation : goalToStart)
				{
					if ((goalLocation.mPriority & 0x80000000) == 0)
						continue;
					route.emplace_back(startLocation.mLocation, goalLocation.mLocation);
				}
			}
			terminate = AisleImpl(route, aisleParameter);
		}

		if (terminate == false)
		{
			// 非ドア～非ドアを検索
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

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		const double lap = stopwatch.Lap();
		if (lap >= 1.0)
		{
			DUNGEON_GENERATOR_WARNING(TEXT("Voxel: Aisle ID=%d, %lf seconds"), static_cast<uint16_t>(aisleParameter.mIdentifier), lap);
		}
		else
		{
			DUNGEON_GENERATOR_LOG(TEXT("Voxel: Aisle ID=%d, %lf seconds"), static_cast<uint16_t>(aisleParameter.mIdentifier), lap);
		}
#endif

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
				理想的なゴール位置がゴール条件に含まれていないなら
				検索できないのでエラーを表示して中断する。
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

				size_t key = pathResult->GetPathLength() << 8;
				key |= index & 0xFF;
				std::lock_guard lock(pathResultsMutex);
				pathResults[key] = pathResult;
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

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
		Stopwatch stopwatch;
#endif
		PathFinder pathFinder;

		// パス検索開始
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
					if (IsPassable(openLocation, aisleParameter.mGenerateIntersections) || IsReachedGoalWithDirection(openLocation, route.mIdealGoal.Z, aisleParameter.mGoalCondition, direction))
					{
						if (pathFinder.IsUsingOpenNode(openLocation) == false)
						{
							const uint32_t connectingCost = (i == nextDirection.Get()) ? PriorityConnectingCost : NormalConnectingCost;
							pathFinder.Open(nextKey, PathFinder::NodeType::Aisle, nextCost + connectingCost, openLocation, route.mIdealGoal, direction, PathFinder::SearchDirection::Any);
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
						IsPassable(upstairsOpenLocationU, false) && IsReachedGoal(upstairsOpenLocationU, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationU) == false &&
						IsPassable(upstairsOpenLocationF, false) && IsReachedGoal(upstairsOpenLocationF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationF) == false &&
						IsPassable(upstairsOpenLocationUF, false) && IsReachedGoal(upstairsOpenLocationUF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false)
					{
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
						IsPassable(downstairsOpenLocationD, false) && IsReachedGoal(downstairsOpenLocationD, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationD) == false &&
						IsPassable(downstairsOpenLocationF, false) && IsReachedGoal(downstairsOpenLocationF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationF) == false &&
						IsPassable(downstairsOpenLocationDF, false) && IsReachedGoal(downstairsOpenLocationDF, route.mIdealGoal.Z, aisleParameter.mGoalCondition) == false)
					{
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
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
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
#if WITH_EDITOR & JENKINS_FOR_DEVELOP
			DUNGEON_GENERATOR_WARNING(TEXT("Voxel: Task %d: Failed to generate route. (%d,%d,%d)-(%d,%d,%d) %d/%d nodes, %lf seconds"), index
				, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z
				, openNodeSize, closeNodeSize
				, stopwatch.Lap());
#else
			DUNGEON_GENERATOR_WARNING(TEXT("Voxel: Task %d: Failed to generate route. (%d,%d,%d)-(%d,%d,%d)"), index
				, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z);
#endif
			return nullptr;
		}

		/*
		 * 通路を共有する時に、門グリッド周辺に同じ方向の門と通路があるなら
		 * 同じ通路の門が並ぶので今回の門は生成する必要が無い
		 */
		if (aisleParameter.mGenerateIntersections)
		{
			if (const auto& pathResult = pathFinder.GetResult())
			{
				check(route.mStart == pathResult->GetStartLocation());
				check(nextLocation == pathResult->GetGoalLocation());
				check(nextDirection == pathResult->GetGoalDirection());
				check(pathResult->GetPathLength() >= 2);

				if (CheckDoorAligned(pathResult->GetStartLocation(), pathResult->GetStartDirection(), pathResult->GetNodeTypeFromStart(1)) == true)
					pathResult->InvalidateStartLocationType();

				if (CheckDoorAligned(pathResult->GetGoalLocation(), pathResult->GetGoalDirection(), pathResult->GetNodeTypeFromGoal(1)) == true)
					pathResult->InvalidateGoalLocationType();
			}
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

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
		DUNGEON_GENERATOR_LOG(TEXT(" - Voxel: Task %d: Completed route search. (%d,%d,%d)-(%d,%d,%d) %d/%d nodes, %lf seconds"), index
			, route.mStart.X, route.mStart.Y, route.mStart.Z, route.mIdealGoal.X, route.mIdealGoal.Y, route.mIdealGoal.Z
			, openNodeSize, closeNodeSize
			, stopwatch.Lap());
#endif

		return pathFinder.GetResult();
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
				}

				grid.SetType(cellType);
				grid.SetDirection(direction);
			}
		);

		// 必要であれば、奥の部屋の門に鍵をかける
		if (aisleParameter.mGenerateIntersections == false)
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
		if (grid.IsNoWallMeshGeneration(enteringDirection.Inverse()) == true)
			return false;

		// 部屋の一階部分の属性なら進入許可
		return grid.Is(Grid::Type::Deck);
	}

	uint32_t Voxel::CalculateCRC32(const uint32_t hash) const noexcept
	{
		const size_t size = static_cast<size_t>(mWidth) * mDepth * mHeight;
		const uint32_t crc32 = GenerateCrc32FromData(mGrids.get(), size, hash);
		return crc32;
	}

	void Voxel::GenerateImageForDebug(const std::string& filename) const
	{
#if defined(DEBUG_GENERATE_BITMAP_FILE)
		const int32_t offsetZ = GetDepth() + 1;

		// 空間のサイズを設定
		const bmp::Canvas canvas(Scale(GetWidth()), Scale(GetDepth() + 1 + GetHeight()));

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
						float ratio = static_cast<float>(z) / static_cast<float>(GetHeight());
						if (ratio <= std::numeric_limits<float>::epsilon())
							ratio = std::numeric_limits<float>::epsilon();
						color.rgbRed = BaseDarkColor.rgbRed + (BaseLightColor.rgbRed - BaseDarkColor.rgbRed) * ratio;
						color.rgbGreen = BaseDarkColor.rgbGreen + (BaseLightColor.rgbGreen - BaseDarkColor.rgbGreen) * ratio;
						color.rgbBlue = BaseDarkColor.rgbBlue + (BaseLightColor.rgbBlue - BaseDarkColor.rgbBlue) * ratio;
					}
					canvas.Rectangle(
						Scale(x),
						Scale(offsetZ + GetHeight() - z),
						Scale(x + 1),
						Scale(offsetZ + GetHeight() - z + 1),
						color
					);
				}
			}
		}

		// グリッドを描画
		{
			for (uint32_t x = 0; x <= GetWidth(); ++x)
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

			for (uint32_t y = 0; y <= GetDepth(); ++y)
			{
				canvas.HorizontalLine(
					Scale(0),
					Scale(GetWidth()),
					Scale(y),
					y == 0 ? OriginXColor : y % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}

			for (uint32_t z = 0; z <= GetHeight(); ++z)
			{
				canvas.HorizontalLine(
					Scale(0),
					Scale(GetWidth()),
					Scale(offsetZ + GetHeight() - z),
					z == 0 ? OriginXColor : z % 10 == 0 ? LightGridColor : DarkGridColor
				);
			}
		}

		canvas.Write(dungeon::GetDebugDirectoryString() + filename);
#endif
	}
}
