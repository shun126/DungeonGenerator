/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
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
 * Parts transform
 * パーツのトランスフォーム
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	/**
	 * Relative location, rotation, and scale applied after the placement transform.
	 * 配置トランスフォームの後に適用する相対位置、回転、スケールです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Relative location, rotation, and scale applied after the part placement transform."))
	FTransform RelativeTransform;

	/** Applies the relative transform to a base world transform. 基準ワールドトランスフォームへ相対トランスフォームを適用します。 */
	FTransform CalculateWorldTransform(const FTransform& transform) const noexcept;
	/** Builds a world transform from position and rotation, then applies the relative transform. 位置と回転からワールドトランスフォームを構築し、相対トランスフォームを適用します。 */
	FTransform CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept;
	/** Builds a world transform from position and yaw, then applies the relative transform. 位置とヨー角からワールドトランスフォームを構築し、相対トランスフォームを適用します。 */
	FTransform CalculateWorldTransform(const FVector& position, const float yaw) const noexcept;
	/** Builds a world transform from position and grid direction, then applies the relative transform. 位置とグリッド方向からワールドトランスフォームを構築し、相対トランスフォームを適用します。 */
	FTransform CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept;

protected:
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept;

};
