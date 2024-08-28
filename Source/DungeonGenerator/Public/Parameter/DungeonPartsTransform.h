/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonPartsPlacementDirection.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonPartsTransform.generated.h"

// forward declaration
class UClass;
class UStaticMesh;

namespace dungeon
{
	class Direction;
	class Random;
}

/**
Parts transform
パーツのトランスフォーム
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	/**
	Set the relative transform of the static mesh
	スタティックメッシュの相対トランスフォームを設定して下さい
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	FTransform RelativeTransform;

	FTransform CalculateWorldTransform(const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept;

protected:
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept;

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const;
#endif
};
