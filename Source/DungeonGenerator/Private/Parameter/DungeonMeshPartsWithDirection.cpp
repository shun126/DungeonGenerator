/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonMeshPartsWithDirection.h"

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, transform, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, rotator, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, yaw, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, direction, PlacementDirection);
}
