/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "DungeonMeshPartsWithDirection.generated.h"

/**
 * Mesh parts with direction specification
 * 方向付きメッシュのパーツ
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshPartsWithDirection : public FDungeonMeshParts
{
	GENERATED_BODY()

public:
	/**
	 * Direction of placement
	 * 配置する方向
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Direction rule applied when placing this mesh part. Random Direction uses the synchronized generation random stream."))
	EDungeonPartsPlacementDirection PlacementDirection = EDungeonPartsPlacementDirection::RandomDirection;

	/** Resolves placement direction and applies the relative transform to a base transform. 配置方向を解決し、基準トランスフォームへ相対トランスフォームを適用します。 */
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept;
	/** Resolves placement direction from position and rotation. 位置と回転から配置方向を解決してワールドトランスフォームを返します。 */
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept;
	/** Resolves placement direction from position and yaw. 位置とヨー角から配置方向を解決してワールドトランスフォームを返します。 */
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept;
	/** Resolves placement direction from position and grid direction. 位置とグリッド方向から配置方向を解決してワールドトランスフォームを返します。 */
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept;

};
