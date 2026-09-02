/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonPartsTransform.h"
#include "Core/Helper/Direction.h"

namespace
{
	FTransform CalculateWorldTransform_(const FTransform& toWorldTransform, const FTransform& relativeTransform)
	{
		return relativeTransform * toWorldTransform;
	}
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FTransform& transform) const noexcept
{
	return CalculateWorldTransform_(transform, RelativeTransform);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept
{
	const FTransform transform(rotator, position);
	return CalculateWorldTransform(transform);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const float yaw) const noexcept
{
	const FRotator rotator(0, yaw, 0);
	return CalculateWorldTransform(position, rotator);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return CalculateWorldTransform(position, direction.ToDegree());
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	if (static_cast<uint8>(placementDirection) < static_cast<uint8>(EDungeonPartsPlacementDirection::West))
	{
		const FRotator rotator(0., static_cast<double>(placementDirection) * 90., 0.);
		FTransform result(transform);
		result.SetRotation(rotator.Quaternion());
		return CalculateWorldTransform_(result, RelativeTransform);
	}
	if (placementDirection == EDungeonPartsPlacementDirection::RandomDirection)
	{
		const FRotator rotator(0., static_cast<double>(random->Get<uint8_t>(4)) * 90., 0.);
		FTransform result(transform);
		result.SetRotation(rotator.Quaternion());
		return CalculateWorldTransform_(result, RelativeTransform);
	}
	if (placementDirection == EDungeonPartsPlacementDirection::FollowGridDirection)
	{
		return CalculateWorldTransform_(transform, RelativeTransform);
	}

	// Error if you have come here
	return transform;
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	const FTransform transform(rotator, position);
	return CalculateWorldTransform(random, transform, placementDirection);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	const FRotator rotator(0, yaw, 0);
	return CalculateWorldTransform(random, position, rotator, placementDirection);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	return CalculateWorldTransform(random, position, direction.ToDegree(), placementDirection);
}
