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
#include "../Math/Math.h"
#include "../PathGeneration/PathFinder.h"
#include "../PathGeneration/PathGoalCondition.h"
#include <array>
#include <bitset>

namespace dungeon
{
	Voxel::Voxel(const GenerateParameter& parameter) noexcept
		: mGrids(std::make_unique<Grid[]>(static_cast<size_t>(parameter.GetWidth()) * parameter.GetDepth() * parameter.GetHeight()))
		, mWidth(parameter.GetWidth())
		, mDepth(parameter.GetDepth())
		, mHeight(parameter.GetHeight())
	{
	}

	void Voxel::NoRoofMeshGeneration(const FIntVector& location, const bool noRoofMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoRoofMeshGeneration(noRoofMeshGeneration);
		}
	}

	void Voxel::NoFloorMeshGeneration(const FIntVector& location, const bool noFloorMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoFloorMeshGeneration(noFloorMeshGeneration);
		}
	}

	void Voxel::NoNorthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoNorthWallMeshGeneration(noWallMeshGeneration);
		}
	}

	void Voxel::NoSouthWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoSouthWallMeshGeneration(noWallMeshGeneration);
		}
	}

	void Voxel::NoEastWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoEastWallMeshGeneration(noWallMeshGeneration);
		}
	}

	void Voxel::NoWestWallMeshGeneration(const FIntVector& location, const bool noWallMeshGeneration) noexcept
	{
		if (Contain(location))
		{
			const size_t index = Index(location);
			mGrids.get()[index].NoWestWallMeshGeneration(noWallMeshGeneration);
		}
	}

	static int32 SquareDistance(const FIntVector& a, const FIntVector& b)
	{
		const auto delta = a - b;
		const int32 squareDistance = delta.X * delta.X + delta.Y * delta.Y + delta.Z * delta.Z;
		return squareDistance;
	}

	/*
	部屋の内部から部屋の外郭を検索します。
	部屋の外のグリッドは Grid::Type::Empty である必要があります。
	*/
	bool Voxel::SearchGateLocation(std::map<size_t, FIntVector>& result, const FIntVector& start, const Identifier& identifier, const FIntVector& goal, const bool shared) noexcept
	{
		check(mGrids.get()[Index(start)].GetIdentifier() == identifier.Get());

		/*
		最も近い位置を調べる関数
		戻り値がfalseならば検索を終了する。
		*/
		auto checker = [this, &result, &goal, identifier, shared](const FIntVector& location) -> bool
			{
				const auto& grid = mGrids.get()[Index(location)];
				if (grid.GetIdentifier() != identifier.Get())
					return false;

				const Grid::Type currentGridType = grid.GetType();
				if (shared)
				{
					if (currentGridType != Grid::Type::Deck && currentGridType != Grid::Type::Gate)
						return true;
				}
				else
				{
					if (currentGridType != Grid::Type::Deck)
						return true;
				}

				/*
				四方向のいずれかにEmptyタイプがあれば合格
				四方向のいずれかにAisle/Slopeタイプがあれば失格
				*/
				bool validLocation = false;
				for (std::uint_fast8_t i = 0; i < 4; ++i)
				{
					// 壁の生成が出来ないならcontinue
					const Direction direction(static_cast<Direction::Index>(i));
					if (grid.IsNoWallMeshGeneration(direction))
						continue;

					const FIntVector& offset = direction.GetVector();
					const Grid& aroundGrid = Get(location + offset);
					const Grid::Type aroundGridType = aroundGrid.GetType();
					if (aroundGrid.GetIdentifier() == identifier.Get())
						continue;
					if (shared)
					{
						if (aroundGrid.IsKindOfSlopeType() && (aroundGrid.GetDirection().IsNorthSouth() == direction.IsNorthSouth()))
							return true;
						if (aroundGrid.GetType() == Grid::Type::Empty || aroundGrid.IsKindOfAisleType() /* || aroundGrid.IsKindOfGateType() || aroundGrid.Is(Grid::Type::Deck)*/)
							validLocation = true;
					}
					else
					{
						if (aroundGrid.IsKindOfAisleType())
							return true;
						if (aroundGrid.IsKindOfSlopeType() && (aroundGrid.GetDirection().IsNorthSouth() == direction.IsNorthSouth()))
							return true;
						if (aroundGrid.GetType() == Grid::Type::Empty)
							validLocation = true;
					}
				}
				if (validLocation)
				{
					const int32 squareDistance = SquareDistance(location, goal);
					result[squareDistance] = location;
				}

				return true;
			};

		// スタート位置周辺で門にできそうな場所を検索
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

		return result.empty() == false;
	}

	bool Voxel::Aisle(const FIntVector& start, const FIntVector& idealGoal, const PathGoalCondition& goalCondition, const Identifier& identifier, const bool mergeRooms, const bool generateIntersections) noexcept
	{
		constexpr uint32_t PriorityConnectingCost = 1;
		constexpr uint32_t NormalConnectingCost = 2;
		constexpr uint32_t SlopeConnectingCost = 3;

		if (!goalCondition.Contains(idealGoal))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Voxel: include the finish line in the goal range. (%d,%d,%d)"), idealGoal.X, idealGoal.Y, idealGoal.Z);
			mLastError = Error::GoalPointIsOutsideGoalRange;
			return false;
		}

		// Start of path search
		// パス検索開始
		PathFinder pathFinder;
		pathFinder.Start(start, idealGoal, PathFinder::SearchDirection::Any);

		// Get the most promising positions
		// 最も有望なポジションを得る
		uint64_t nextKey;
		PathFinder::NodeType nextNodeType;
		uint32_t nextCost;
		FIntVector nextLocation;
		Direction nextDirection;
		PathFinder::SearchDirection nextSearchDirection;
		while (pathFinder.Pop(nextKey, nextNodeType, nextCost, nextLocation, nextDirection, nextSearchDirection))
		{
			// Reaching the goal?
			// ゴールに到達？
			if (IsReachedGoal(nextLocation, idealGoal.Z, goalCondition))
			{
				if (nextNodeType == PathFinder::NodeType::Aisle && nextSearchDirection == PathFinder::SearchDirection::Any)
					break;
				else
					continue;
			}

			// Search horizontally
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
					const auto& openGrid = Get(openLocation);
					if (pathFinder.IsUsingOpenNode(openLocation) == false)
					{
						if (IsPassable(openLocation, generateIntersections) || IsReachedGoalWithDirection(openLocation, idealGoal.Z, goalCondition, direction))
						{
							const uint32_t connectingCost = (i == nextDirection.Get()) ? PriorityConnectingCost : NormalConnectingCost;
							pathFinder.Open(nextKey, PathFinder::NodeType::Aisle, nextCost + connectingCost, openLocation, idealGoal, direction, PathFinder::SearchDirection::Any);
						}
					}
				}
			}

			// Vertical search
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
						IsPassable(upstairsOpenLocationU, false) && IsReachedGoal(upstairsOpenLocationU, idealGoal.Z, goalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationU) == false &&
						IsPassable(upstairsOpenLocationF, false) && IsReachedGoal(upstairsOpenLocationF, idealGoal.Z, goalCondition) == false && pathFinder.IsUsingOpenNode(upstairsOpenLocationF) == false &&
						IsPassable(upstairsOpenLocationUF, false) && IsReachedGoal(upstairsOpenLocationUF, idealGoal.Z, goalCondition) == false)
					{
						pathFinder.Open(nextKey, PathFinder::NodeType::Upstairs, nextCost + SlopeConnectingCost, upstairsOpenLocationUF, idealGoal, nextDirection, PathFinder::Cast(nextDirection));

						auto useNode = std::make_shared<PathNodeSwitcher::Node>();
						useNode->Add(PathFinder::Hash(upstairsOpenLocationU));
						useNode->Add(PathFinder::Hash(upstairsOpenLocationF));
						pathFinder.ReserveOpenNode(upstairsOpenLocationUF, useNode);
					}

					// down
					const FIntVector downstairsOpenLocationD = nextLocation + FIntVector(0, 0, -1);
					const FIntVector downstairsOpenLocationF = nextLocation + nextDirection.GetVector();
					const FIntVector downstairsOpenLocationDF = downstairsOpenLocationF + FIntVector(0, 0, -1);
					if (
						IsPassable(downstairsOpenLocationD, false) && IsReachedGoal(downstairsOpenLocationD, idealGoal.Z, goalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationD) == false &&
						IsPassable(downstairsOpenLocationF, false) && IsReachedGoal(downstairsOpenLocationF, idealGoal.Z, goalCondition) == false && pathFinder.IsUsingOpenNode(downstairsOpenLocationF) == false &&
						IsPassable(downstairsOpenLocationDF, false) && IsReachedGoal(downstairsOpenLocationDF, idealGoal.Z, goalCondition) == false)
					{
						pathFinder.Open(nextKey, PathFinder::NodeType::Downstairs, nextCost + SlopeConnectingCost, downstairsOpenLocationDF, idealGoal, nextDirection, PathFinder::Cast(nextDirection));

						auto useNode = std::make_shared<PathNodeSwitcher::Node>();
						useNode->Add(PathFinder::Hash(downstairsOpenLocationD));
						useNode->Add(PathFinder::Hash(upstairsOpenLocationF));
						pathFinder.ReserveOpenNode(downstairsOpenLocationDF, useNode);
					}
				}
			}
		}

		if (!goalCondition.Contains(nextLocation))
		{
			DUNGEON_GENERATOR_LOG(TEXT("Voxel: Failed to search route. (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}

		// nextLocationが実際に到達した場所
		if (!pathFinder.Commit(nextLocation))
		{
			DUNGEON_GENERATOR_LOG(TEXT("Voxel: Failed to generate route. (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}

#if defined(DEBUG_ENABLE_SHOW_DEVELOP_LOG)
		DUNGEON_GENERATOR_LOG(TEXT("Voxel: Pathfinding succeeded. (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, nextLocation.X, nextLocation.Y, nextLocation.Z);
#endif
#if 0
		// 部屋を結合する場合ドアが不要なので、短すぎる通路を失敗扱いにする
		if (mergeRooms && pathFinder.GetPathLength() <= 2)
		{
			DUNGEON_GENERATOR_LOG(TEXT("Voxel: 探索した経路が短すぎました (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}
#endif
		// パスをグリッドに反映します
		pathFinder.Path([this, &identifier, generateIntersections](PathFinder::NodeType nodeType, const FIntVector& location, Direction direction)
			{
				const size_t index = Index(location);
				Grid& grid = mGrids.get()[index];

				Grid::Type cellType;
				switch (nodeType)
				{
				case PathFinder::NodeType::UpSpace:
					cellType = Grid::Type::UpSpace;
					grid.MergeAisle(generateIntersections);
					break;

				case PathFinder::NodeType::DownSpace:
					cellType = Grid::Type::DownSpace;
					grid.MergeAisle(generateIntersections);
					break;

				case PathFinder::NodeType::Downstairs:
				case PathFinder::NodeType::Upstairs:
					cellType = Grid::Type::Slope;
					grid.MergeAisle(generateIntersections);
					break;

				case PathFinder::NodeType::Stairwell:
					cellType = Grid::Type::Stairwell;
					grid.MergeAisle(generateIntersections);
					break;
						
				case PathFinder::NodeType::Gate:
					cellType = Grid::Type::Gate;
					break;

				case PathFinder::NodeType::Aisle:
					cellType = Grid::Type::Aisle;
					grid.MergeAisle(generateIntersections);
					break;

				default:
					return;
				}

				// 識別子が無効なら通路として設定する
				if (grid.IsInvalidIdentifier())
				{
					grid.SetIdentifier(identifier.Get());
				}

				grid.SetType(cellType);
				grid.SetDirection(direction);

#if defined(DEBUG_ENABLE_INFOMATION_FOR_REPRECATION)
				DUNGEON_GENERATOR_LOG(TEXT("pathFinder: Identifier=%d"), grid.GetIdentifier());
#endif
			}
		);

		return true;
	}

	size_t Voxel::Index(const uint32_t x, const uint32_t y, const uint32_t z) const noexcept
	{
		const size_t index
			= static_cast<size_t>(z) * mWidth * mDepth
			+ static_cast<size_t>(y) * mWidth
			+ static_cast<size_t>(x);
		return index;
	}

	size_t Voxel::Index(const FIntVector& location) const noexcept
	{
		const size_t index
			= static_cast<size_t>(location.Z) * mWidth * mDepth
			+ static_cast<size_t>(location.Y) * mWidth
			+ static_cast<size_t>(location.X);
		return index;
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

	const Grid& Voxel::Get(const FIntVector& location) const noexcept
	{
		return Get(location.X, location.Y, location.Z);
	}

	const Grid& Voxel::Get(const size_t index) const noexcept
	{
		check(index < static_cast<size_t>(mWidth) * mDepth * mHeight);
		return mGrids.get()[index];
	}

	void Voxel::Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) noexcept
	{
		if (x < mWidth && y < mDepth && z < mHeight)
		{
			const size_t index = Index(x, y, z);
			mGrids.get()[index] = grid;
		}
	}

	void Voxel::Rectangle(const FIntVector& min, const FIntVector& max, const Grid& fillGrid, const Grid& floorGrid) noexcept
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

	bool Voxel::Contain(const FIntVector& location) const noexcept
	{
		return
			(0 <= location.X && location.X < static_cast<int32_t>(mWidth)) &&
			(0 <= location.Y && location.Y < static_cast<int32_t>(mDepth)) &&
			(0 <= location.Z && location.Z < static_cast<int32_t>(mHeight));
	}

	Grid& Voxel::operator[](const size_t index) noexcept
	{
		check(index < static_cast<size_t>(mWidth)* mDepth* mHeight);
		return mGrids.get()[index];
	}

	const Grid& Voxel::operator[](const size_t index) const noexcept
	{
		check(index < static_cast<size_t>(mWidth)* mDepth* mHeight);
		return mGrids.get()[index];
	}

	void Voxel::Each(std::function<bool(const FIntVector& location, Grid& grid)> func) noexcept
	{
		for (uint32_t z = 0; z < mHeight; ++z)
		{
			for (uint32_t y = 0; y < mDepth; ++y)
			{
				for (uint32_t x = 0; x < mWidth; ++x)
				{
					const size_t index = Index(x, y, z);
					Grid& grid = mGrids.get()[index];
					if (!func(FIntVector(x, y, z), grid))
						return;
				}
			}
		}
	}

	void Voxel::Each(std::function<bool(const FIntVector& location, const Grid& grid)> func) const noexcept
	{
		for (uint32_t z = 0; z < mHeight; ++z)
		{
			for (uint32_t y = 0; y < mDepth; ++y)
			{
				for (uint32_t x = 0; x < mWidth; ++x)
				{
					const size_t index = Index(x, y, z);
					Grid& grid = mGrids.get()[index];
					if (!func(FIntVector(x, y, z), grid))
						return;
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

	bool Voxel::IsReachedGoalWithDirection(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition, const Direction& enteringDirection) noexcept
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

		// 部屋の一階部分の属性ではない？
		if (grid.Is(Grid::Type::Deck) == false)
			return false;

		// 進入許可
		return true;
	}

	uint32_t Voxel::GetWidth() const noexcept
	{
		return mWidth;
	}

	uint32_t Voxel::GetDepth() const noexcept
	{
		return mDepth;
	}

	uint32_t Voxel::GetHeight() const noexcept
	{
		return mHeight;
	}

	uint32_t Voxel::CalculateCRC32(uint32_t hash) const noexcept
	{
		const size_t size = static_cast<size_t>(mWidth) * mDepth * mHeight;
		const uint32_t crc32 = GenerateCrc32FromData(mGrids.get(), size, hash);
		return crc32;
	}
}
