/**
ボクセルなどに利用するグリッド情報のソースファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Grid.h"

namespace dungeon
{
	/**
	自身からtoGridを見た時に床が生成されるか判定します
	*/
	bool Grid::CanBuildFloor(const bool checkNoMeshGeneration) const noexcept
	{
		if (checkNoMeshGeneration && IsNoFloorMeshGeneration())
			return false;

		return Is(Type::Deck) || Is(Type::Gate) || Is(Type::Aisle);
	}

	/**
	斜面が生成されるか判定します
	*/
	bool Grid::CanBuildSlope() const noexcept
	{
		return Is(Type::Slope);
	}

	/**
	自身からtoGridを見た時に屋根が生成されるか判定します
	*/
	bool Grid::CanBuildRoof(const Grid& toUpperGrid, const bool checkNoMeshGeneration) const noexcept
	{
		if (checkNoMeshGeneration && IsNoRoofMeshGeneration())
			return false;

		// 範囲外なら天井を生成
		if (IsKindOfSpatialType())
			return false;

		// 通路・スロープなら天井を生成
		if (Is(Type::Aisle) || Is(Type::Stairwell) || Is(Type::UpSpace))
		{


			if (GetIdentifier() == toUpperGrid.GetIdentifier())
			{
				return false;
			}


			return true;
		}

		// 識別子が違うなら天井を生成
		if (toUpperGrid.GetIdentifier() != GetIdentifier())
			return true;

		return false;
	}

	/**
	自身からtoGridを見た時に扉が生成されるか判定します
	*/
	bool Grid::CanBuildGate(const Grid& toGrid, const Direction::Index direction, const bool mergeRooms) const noexcept
	{
		if (!Is(Type::Gate))
			return false;

		/*
		門と門の間に通路が無い場合は、
		ゴールと反対方向のグリッドのみ門を生成する
		*/
		if (toGrid.IsKindOfGateType())
		{
			return
				mergeRooms == false &&
				GetIdentifier() != toGrid.GetIdentifier() &&
				GetDirection() == toGrid.GetDirection() &&
				GetDirection().Inverse() == Direction(direction);
		}
		/*
		スロープの正面が門と同じ方向なら門を生成する
		*/
		else if (toGrid.IsKindOfSlopeType())
		{

			if (GetIdentifier() == toGrid.GetIdentifier())
			{
				return false;
			}


			// スロープの正面が門と同じ方向なら門
			return
				GetDirection().IsNorthSouth() == toGrid.GetDirection().IsNorthSouth() &&
				GetDirection().IsNorthSouth() == Direction(direction).IsNorthSouth();
		}
		/*
		通路が門と同じ方向なら門を生成する
		*/
		else if (toGrid.IsKindOfAisleType())
		{
			// 通路の正面が門と同じ方向なら門
			return
				GetIdentifier() != toGrid.GetIdentifier() &&
				GetDirection().IsNorthSouth() == Direction(direction).IsNorthSouth();
		}

		return false;
	}

	/**
	自身からtoGridを見た時に壁が生成されるか判定します
	*/
	bool Grid::CanBuildWall(const Grid& toGrid, const Direction::Index direction, const bool mergeRooms) const noexcept
	{
		// TODO: NoWallMeshGenerationフラグは進入禁止に使用されている別途生成禁止フラグが必要
		if (IsNoWallMeshGeneration(direction))
			return false;
		/*
		*/
		// 門以外の部屋
		if (IsKindOfRoomTypeWithoutGate())
		{
			/*
			部屋と部屋を結合する場合
			部屋と部屋が隣接していてグリッドの識別番号（＝部屋の識別番号）が不一致なら壁がある
			*/
			if (mergeRooms == false && toGrid.IsKindOfRoomTypeWithoutGate())
				return GetIdentifier() != toGrid.GetIdentifier();

			// 部屋対門
			if (toGrid.IsKindOfGateType())
			{
				// 識別子が異なっていて、かつ方向が交差していたら壁
				return
					toGrid.GetIdentifier() != GetIdentifier() &&
					toGrid.GetDirection().IsNorthSouth() != Direction::IsNorthSouth(direction);
			}

			if (GetIdentifier() == toGrid.GetIdentifier())
			{
				return false;
			}

			// 以下の条件なら壁を生成
			return
				toGrid.IsKindOfAisleType() ||	// 通路
				toGrid.IsKindOfSlopeType() ||	// スロープ
				toGrid.IsKindOfSpatialType();	// 範囲外
		}
		// 門
		else if (IsKindOfGateType())
		{
			// 門対部屋、または通路、またはスロープ
			if (toGrid.IsKindOfRoomType() || toGrid.IsKindOfAisleType() || toGrid.IsKindOfSlopeType())
			{
				// 識別子が異なっていて、かつ方向が交差していたら壁
				return
					GetIdentifier() != toGrid.GetIdentifier() &&
					GetDirection().IsNorthSouth() != Direction::IsNorthSouth(direction);
			}

			// 範囲外なら壁
			return toGrid.IsKindOfSpatialType();
		}
		// 通路
		else if (IsKindOfAisleType())
		{
			// 通路対門以外の部屋
			if (toGrid.IsKindOfRoomTypeWithoutGate())
			{



				if (GetIdentifier() == toGrid.GetIdentifier())
				{
					return false;
				}



				return true;
			}
			// 通路対門
			else if (toGrid.IsKindOfGateType())
			{
				// 門の正面なら壁を作らない
				return toGrid.GetDirection().IsNorthSouth() != Direction::IsNorthSouth(direction);
			}
			// 通路対スロープ
			else if (toGrid.IsKindOfSlopeType())
			{
				// 通路の結合が可能ならば壁の生成を禁止する
				if (toGrid.CanMergeAisle() && CanMergeAisle())
				{
					// スロープの正面なら壁を作らない
					if (toGrid.GetDirection().IsNorthSouth() == Direction::IsNorthSouth(direction))
						return toGrid.Is(Type::DownSpace) || toGrid.Is(Type::Stairwell);
				}

				return toGrid.GetIdentifier() != GetIdentifier();
			}
			// 通路対通路
			else if (toGrid.IsKindOfAisleType())
			{
				// 通路の結合が可能ならば壁の生成を禁止する
				if (toGrid.CanMergeAisle() && CanMergeAisle())
					return false;

				// 通路の識別子が違うなら壁
				return toGrid.GetIdentifier() != GetIdentifier();
			}
			// 範囲外なら壁
			return toGrid.IsKindOfSpatialType();
		}
		// スロープとスロープの空間
		else if (IsKindOfSlopeType())
		{
			// スロープ対門以外の部屋
			if (toGrid.IsKindOfRoomTypeWithoutGate())
			{
				if (GetIdentifier() == toGrid.GetIdentifier())
				{
					return false;
				}

				return CanBuildWall_SlopeVsRoom();
			}
			// スロープ対門
			else if (toGrid.IsKindOfGateType())
			{

				if (GetIdentifier() == toGrid.GetIdentifier())
				{
					return false;
				}


				return CanBuildWall_SlopeVsGate(toGrid, direction);
			}
			// スロープ対通路
			else if (toGrid.IsKindOfAisleType())
			{

				if (GetIdentifier() == toGrid.GetIdentifier())
				{
					return false;
				}


				return CanBuildWall_SlopeVsAisle(toGrid, direction);
			}
			// スロープ対スロープ
			else if (toGrid.IsKindOfSlopeType())
			{
				return CanBuildWall_SlopeVsSlope(toGrid, direction);
			}
			// 範囲外なら壁
			return toGrid.IsKindOfSpatialType();
		}

		return false;
	}

	bool Grid::CanBuildWall_SlopeVsRoom() noexcept
	{
		// 横方向では常に壁（スロープの正面に部屋が来ることは無い）
		return true;
	}

	bool Grid::CanBuildWall_SlopeVsGate(const Grid& toGrid, const Direction::Index direction) noexcept
	{
		// 門の横から接続しているなら壁
		return toGrid.GetDirection().IsNorthSouth() != Direction::IsNorthSouth(direction);
	}

	bool Grid::CanBuildWall_SlopeVsAisle(const Grid& toGrid, const Direction::Index direction) const noexcept
	{
		// 通路の結合が可能
		if (toGrid.CanMergeAisle() && CanMergeAisle())
		{
			// Type::Slopeのスペースの上側空白(吹き抜け）以外
			if (Is(Type::Stairwell) == false)
			{
				// 調べる方向がスロープと同じ方向なら壁を作らない
				if (GetDirection().IsNorthSouth() == Direction::IsNorthSouth(direction))
					return false;
			}
		}

		// グリッドの識別番号が不一致なら壁がある
		return toGrid.GetIdentifier() != GetIdentifier();
	}

	bool Grid::CanBuildWall_SlopeVsSlope(const Grid& toGrid, const Direction::Index direction) const noexcept
	{
		// 通路の結合が可能
		if (toGrid.CanMergeAisle() && CanMergeAisle())
		{
			// 調べる方向がスロープの方向と交差していたら壁
			if (GetDirection().IsNorthSouth() != Direction::IsNorthSouth(direction))
			{
				// スロープが交差しているなら壁
				if (toGrid.GetDirection() != GetDirection())
					return true;

				// スロープの違うタイプなら壁
				return toGrid.GetType() != GetType();
			}

			// 違う方向を向くスロープなら壁
			return toGrid.GetDirection().IsNorthSouth() != GetDirection().IsNorthSouth();
		}

		// グリッドの識別番号が不一致なら壁がある
		return toGrid.GetIdentifier() != GetIdentifier();
	}

	const FColor& Grid::GetTypeColor() const noexcept
	{
		return GetTypeColor(GetType());
	}

	const FColor& Grid::GetTypeColor(Grid::Type gridType) noexcept
	{
		static const FColor colors[] = {
			FColor::Yellow,		// Floor
			FColor(128,128,0),	// Deck
			FColor::Red,		// Gate
			FColor::Green,		// Aisle
			FColor(128,0,128),	// Slope
			FColor::Magenta,	// Stairwell
			FColor(0,128,128),	// DownSpace
			FColor::Cyan,		// UpSpace
			FColor::Black,		// Empty
			FColor::Black,		// OutOfBounds
		};
		static constexpr size_t ColorSize = sizeof(colors) / sizeof(colors[0]);
		static_assert(ColorSize == static_cast<size_t>(TypeSize));
		const size_t index = static_cast<size_t>(gridType);
		return colors[index];
	}

	const FString& Grid::GetTypeName() const noexcept
	{
		static const FString names[] = {
			TEXT("Floor"),
			TEXT("Deck"),
			TEXT("Gate"),
			TEXT("Aisle"),
			TEXT("Slope"),
			TEXT("Stairwell"),
			TEXT("DownSpace"),
			TEXT("UpSpace"),
			TEXT("Empty"),
			TEXT("OutOfBounds")
		};
		static constexpr size_t NameSize = sizeof(names) / sizeof(names[0]);
		static_assert(NameSize == static_cast<size_t>(TypeSize));
		const size_t index = static_cast<size_t>(GetType());
		return names[index];
	}

	const FString& Grid::GetPropsName() const noexcept
	{
		static const FString names[] = {
			"None",
			"Lock",
			"UniqueLock"
		};
		static constexpr size_t NameSize = sizeof(names) / sizeof(names[0]);
		static_assert(NameSize == static_cast<size_t>(PropsSize));
		const size_t index = static_cast<size_t>(GetProps());
		return names[index];
	}

	FString Grid::GetNoMeshGenerationName() const noexcept
	{
		FString noMeshGenerationName;

		if (mPack.IsAttributeEnabled(Attribute::NoNorthWallMeshGeneration))
		{
			noMeshGenerationName += TEXT("NorthWall");
		}
		if (mPack.IsAttributeEnabled(Attribute::NoSouthWallMeshGeneration))
		{
			if (!noMeshGenerationName.IsEmpty())
				noMeshGenerationName += TEXT(",");
			noMeshGenerationName += TEXT("SouthWall");
		}
		if (mPack.IsAttributeEnabled(Attribute::NoEastWallMeshGeneration))
		{
			if (!noMeshGenerationName.IsEmpty())
				noMeshGenerationName += TEXT(",");
			noMeshGenerationName += TEXT("EastWall");
		}
		if (mPack.IsAttributeEnabled(Attribute::NoWestWallMeshGeneration))
		{
			if (!noMeshGenerationName.IsEmpty())
				noMeshGenerationName += TEXT(",");
			noMeshGenerationName += TEXT("WestWall");
		}
		if (mPack.IsAttributeEnabled(Attribute::NoFloorMeshGeneration))
		{
			if (!noMeshGenerationName.IsEmpty())
				noMeshGenerationName += TEXT(",");
			noMeshGenerationName += TEXT("Floor");
		}
		if (mPack.IsAttributeEnabled(Attribute::NoRoofMeshGeneration))
		{
			if (!noMeshGenerationName.IsEmpty())
				noMeshGenerationName += TEXT(",");
			noMeshGenerationName += TEXT("Roof");
		}

		return FString(TEXT("NoGeneration: ")) + noMeshGenerationName;
	}
}
