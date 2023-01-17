/*!
グリッドに関するソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Voxel.h"
#include "GenerateParameter.h"
#include "PathFinder.h"
#include "PathGoalCondition.h"
#include "Debug/Debug.h"
#include "Math/Math.h"
#include <array>

namespace dungeon
{
	Voxel::Voxel(const GenerateParameter& parameter) noexcept
		: mGrids(std::make_unique<Grid[]>(static_cast<size_t>(parameter.GetWidth()) * parameter.GetDepth() * parameter.GetHeight()))
		, mWidth(parameter.GetWidth())
		, mDepth(parameter.GetDepth())
		, mHeight(parameter.GetHeight())
	{
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

	bool Voxel::Aisle(const FIntVector& start, const FIntVector& idealGoal, const PathGoalCondition& goalCondition, const Identifier& identifier) noexcept
	{
		if (!goalCondition.Contains(idealGoal))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("ゴール地点をゴール範囲に含めて下さい (%d,%d,%d)"), idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}

		// パス検索開始
		PathFinder pathFinder;
		pathFinder.Start(PathFinder::NodeType::Gate, start, idealGoal, PathFinder::SearchDirection::Any);

		// 最も有望な位置を取得します
		uint64_t nextKey;
		PathFinder::NodeType nextNodeType;
		uint32_t nextCost;
		FIntVector nextLocation;
		Direction nextDirection;
		PathFinder::SearchDirection nextSearchDirection;
		while (pathFinder.Pop(nextKey, nextNodeType, nextCost, nextLocation, nextDirection, nextSearchDirection))
		{
			// ゴールに到達？
			if (IsReachedGoal(nextLocation, idealGoal.Z, goalCondition))
			{
				if (nextNodeType == PathFinder::NodeType::Aisle && nextSearchDirection == PathFinder::SearchDirection::Any)
					break;
				else
					continue;
			}

			// 水平方向へ探索
			for (auto i = Direction::Begin(); i != Direction::End(); ++i)
			{
				if (
					nextSearchDirection == PathFinder::SearchDirection::Any ||
					nextSearchDirection == static_cast<PathFinder::SearchDirection>(std::distance(Direction::Begin(), i)))
				{
					const FIntVector openLocation = nextLocation + *i;
					if (IsHorizontallyPassable(openLocation) || IsReachedGoal(openLocation, idealGoal.Z, goalCondition))
					{
						const Direction direction(static_cast<Direction::Index>(std::distance(Direction::Begin(), i)));
						pathFinder.Open(nextKey, PathFinder::NodeType::Aisle, nextCost + 1, openLocation, idealGoal, direction, PathFinder::SearchDirection::Any);
					}
				}
			}

			// 垂直方向へ探索
			if (nextNodeType == PathFinder::NodeType::Aisle || nextNodeType == PathFinder::NodeType::Gate)
			{
				// 下
				const FIntVector downstairsOpenLocationD = nextLocation + FIntVector(0, 0, -1);
				const FIntVector downstairsOpenLocationF = nextLocation + nextDirection.GetVector();
				const FIntVector downstairsOpenLocationDF = downstairsOpenLocationF + FIntVector(0, 0, -1);
				if (
					IsReachedGoal(downstairsOpenLocationD, idealGoal.Z, goalCondition) == false && IsEmpty(downstairsOpenLocationD) &&
					IsReachedGoal(downstairsOpenLocationF, idealGoal.Z, goalCondition) == false && IsEmpty(downstairsOpenLocationF) &&
					IsReachedGoal(downstairsOpenLocationDF, idealGoal.Z, goalCondition) == false && IsEmpty(downstairsOpenLocationDF))
				{
					pathFinder.Open(nextKey, PathFinder::NodeType::Downstairs, nextCost + 1, downstairsOpenLocationDF, idealGoal, nextDirection, PathFinder::Cast(nextDirection));
				}

				// 上
				const FIntVector upstairsOpenLocationU = nextLocation + FIntVector(0, 0, 1);
				const FIntVector upstairsOpenLocationF = nextLocation + nextDirection.GetVector();
				const FIntVector upstairsOpenLocationUF = upstairsOpenLocationF + FIntVector(0, 0, 1);
				if (
					IsReachedGoal(upstairsOpenLocationU, idealGoal.Z, goalCondition) == false && IsEmpty(upstairsOpenLocationU) &&
					IsReachedGoal(upstairsOpenLocationF, idealGoal.Z, goalCondition) == false && IsEmpty(upstairsOpenLocationF) &&
					IsReachedGoal(upstairsOpenLocationUF, idealGoal.Z, goalCondition) == false && IsEmpty(upstairsOpenLocationUF))
				{
					pathFinder.Open(nextKey, PathFinder::NodeType::Upstairs, nextCost + 1, upstairsOpenLocationUF, idealGoal, nextDirection, PathFinder::Cast(nextDirection));
				}
			}
		}

		if (!goalCondition.Contains(nextLocation))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("経路探索に失敗しました (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}

		// nextLocationが実際に到達した場所
		if (!pathFinder.Commit(nextLocation))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("経路探索に失敗しました (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, idealGoal.X, idealGoal.Y, idealGoal.Z);
			return false;
		}

		DUNGEON_GENERATOR_LOG(TEXT("経路探索に成功しました (%d,%d,%d)-(%d,%d,%d)"), start.X, start.Y, start.Z, nextLocation.X, nextLocation.Y, nextLocation.Z);

		// パスをグリッドに反映します
		{
			pathFinder.Path([this, &identifier](PathFinder::NodeType nodeType, const FIntVector& location, Direction direction)
				{
					Grid::Type cellType;
					switch (nodeType)
					{
					case PathFinder::NodeType::Space:
						cellType = Grid::Type::Atrium;
						break;

					case PathFinder::NodeType::Downstairs:
						cellType = Grid::Type::Slope;
						direction.SetInverse();
						break;

					case PathFinder::NodeType::Upstairs:
						cellType = Grid::Type::Slope;
						break;

					case PathFinder::NodeType::Aisle:
					case PathFinder::NodeType::Gate:
					default:
						cellType = Grid::Type::Aisle;
						break;
					}
					const size_t index = Index(location);
					mGrids.get()[index].SetType(cellType);
					mGrids.get()[index].SetDirection(direction);
					mGrids.get()[index].SetIdentifier(identifier.Get());
				}
			);

			// 開始位置と終了位置
			{
#if 0
				const FIntVector delta = nextLocation - start;
				double yaw = std::atan2(delta.X, delta.Y);
				yaw *= 4.0 / (3.1415926535897932384626433832795 * 2.0);
				const Direction d(static_cast<Direction::Index>(static_cast<uint8_t>(yaw) & 3));

				Grid& startGrid = mGrids.get()[Index(start)];
				startGrid.SetType(Grid::Type::Gate);
				startGrid.SetDirection(d);

				Grid& goalGrid = mGrids.get()[Index(nextLocation)];
				goalGrid.SetType(Grid::Type::Gate);
				goalGrid.SetDirection(d);
#else
				Grid& startGrid = mGrids.get()[Index(start)];
				Grid& goalGrid = mGrids.get()[Index(nextLocation)];
				startGrid.SetType(Grid::Type::Gate);
				startGrid.SetDirection(goalGrid.GetDirection());
				goalGrid.SetType(Grid::Type::Gate);

				/*
				開始と終了位置は部屋の中なので識別子を書き換えない
				SetIdentifier(identifier);
				*/
#endif
			}
		}
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

	void Voxel::Set(const uint32_t x, const uint32_t y, const uint32_t z, const Grid& grid) noexcept
	{
		if (x < mWidth && y < mDepth && z < mHeight)
		{
			const size_t index = Index(x, y, z);
			mGrids.get()[index] = grid;
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

	bool Voxel::IsEmpty(const FIntVector& location) const noexcept
	{
		// 範囲内？
		if (!Contain(location))
			return false;

		// 侵入できる？
		const size_t index = Index(location);
		const auto& grid = mGrids.get()[index];
		return grid.GetType() == Grid::Type::Empty;
	}

	bool Voxel::IsHorizontallyPassable(const FIntVector& location) const noexcept
	{
		// 範囲内？
		if (!Contain(location))
			return false;

		// 水平方向に侵入できる？
		const size_t index = Index(location);
		const auto& grid = mGrids.get()[index];
		return grid.GetType() == Grid::Type::Empty || grid.GetType() == Grid::Type::Aisle;
	}

	bool Voxel::IsHorizontallyPassable(const Grid& baseGrid, const FIntVector& location) const noexcept
	{
		// 範囲内？
		if (!Contain(location))
			return false;

		// 水平方向に侵入できる？
		const size_t index = Index(location);
		const auto& grid = mGrids.get()[index];
		if (grid.GetType() == Grid::Type::Deck && baseGrid.GetType() == Grid::Type::Deck)
			return grid.GetIdentifier() != baseGrid.GetIdentifier();

		return grid.GetType() == Grid::Type::Empty || grid.GetType() == Grid::Type::Aisle;
	}

	bool Voxel::IsReachedGoal(const FIntVector& location, const int32_t goalAltitude, const PathGoalCondition& goalCondition) noexcept
	{
		const bool reachTheGoal = (location.Z == goalAltitude) && (goalCondition.Contains(location) == true);
		return reachTheGoal;
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
}
