/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonMeshPartsWithDirection.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include <CoreMinimal.h>
#include <functional>
#include <memory>
#include "DungeonRoomMeshSet.generated.h"

/*
Dungeon room mesh set
ダンジョン部屋のメッシュセット
*/
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

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif

protected:
	/*
	How to select floor parts
	床のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Specify the floor parts. Multiple parts can be set and will be selected based on FloorPartsSelectionMethod.
	床のパーツを指定して下さい。パーツは複数設定する事ができ、FloorPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
	TArray<FDungeonMeshPartsWithDirection> FloorParts;

	/*
	How to select wall parts
	壁のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Specify the wall parts. Multiple parts can be set and will be selected based on WallPartsSelectionMethod.
	壁のパーツを指定して下さい。パーツは複数設定する事ができ、WallPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
	TArray<FDungeonMeshParts> WallParts;

	/*
	How to select roof parts
	天井のパーツを選択する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Roof", BlueprintReadWrite)
	EDungeonPartsSelectionMethod RoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Specify the roof parts. Multiple parts can be set and will be selected based on RoofPartsSelectionMethod.
	天井のパーツを指定して下さい。パーツは複数設定する事ができ、RoofPartsSelectionMethodを元に選択されます。
	*/
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
