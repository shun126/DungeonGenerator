/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomMeshSet.h"
#include "DungeonAisleMeshSet.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonAisleMeshSet : public FDungeonRoomMeshSet
{
	GENERATED_BODY()

public:
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;

protected:
	// How to generate parts for stairs and ramps
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Sloop", BlueprintReadWrite)
		EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// Stairs and ramp parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Sloop", BlueprintReadWrite)
		TArray<FDungeonMeshParts> SlopeParts;
};
