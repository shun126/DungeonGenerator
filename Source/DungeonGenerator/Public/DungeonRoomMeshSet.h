/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonMeshSet.h"
#include "DungeonRoomMeshSet.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomMeshSet : public FDungeonMeshSet
{
	GENERATED_BODY()

public:
	const FDungeonMeshPartsWithDirection* SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	void EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshParts* SelectWallParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshPartsWithDirection* SelectRoofParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	void EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

protected:
	//!	How to generate floor parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Floor parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> FloorParts;

	//!	How to generate wall parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Wall parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		TArray<FDungeonMeshParts> WallParts;

	//!	How to generate room parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
		EDungeonPartsSelectionMethod RoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Room Roof Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> RoofParts;
};

inline const FDungeonMeshPartsWithDirection* FDungeonRoomMeshSet::SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return SelectParts(gridIndex, grid, random, FloorParts, FloorPartsSelectionMethod);
}

inline void FDungeonRoomMeshSet::EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachParts(FloorParts, func);
}

inline const FDungeonMeshParts* FDungeonRoomMeshSet::SelectWallParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return SelectParts(gridIndex, grid, random, WallParts, WallPartsSelectionMethod);
}

inline void FDungeonRoomMeshSet::EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachParts(WallParts, func);
}

inline const FDungeonMeshPartsWithDirection* FDungeonRoomMeshSet::SelectRoofParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	return SelectParts(gridIndex, grid, random, RoofParts, RoofPartsSelectionMethod);
}

inline void FDungeonRoomMeshSet::EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachParts(RoofParts, func);
}
