/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonRoomMeshSet.h"
#include <CoreMinimal.h>
#include <functional>
#include "DungeonAisleMeshSet.generated.h"

/*
Dungeon aisle mesh set
ダンジョン通路のメッシュセット
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonAisleMeshSet : public FDungeonRoomMeshSet
{
	GENERATED_BODY()

public:
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;

#if WITH_EDITOR
public:
	// Debug
	FString DumpToJson(const uint32 indent) const;
#endif

protected:
	/*
	How to generate parts for stairs and ramps
	階段やスロープの部品を生成する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Specify parts for stairs and ramps. Multiple parts can be set and will be selected based on the SloopPartsSelectionMethod.
	階段やスロープのパーツを指定して下さい。パーツは複数設定する事ができ、SloopPartsSelectionMethodを元に選択されます。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
	TArray<FDungeonMeshParts> SlopeParts;
};
