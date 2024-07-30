/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonActorPartsWithDirection.h"
#include "Parameter/DungeonPartsTransform.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, transform, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, rotator, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, yaw, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, direction, PlacementDirection);
}

#if WITH_EDITOR
FString FDungeonActorPartsWithDirection::DumpToJson(const uint32 indent) const
{
	FString json = Super::DumpToJson(indent) + TEXT(", \n");
	json += dungeon::Indent(indent) + TEXT("\"PlacementDirection\":\"") + UEnum::GetValueAsString(PlacementDirection) + TEXT("\"");
	return json;
}
#endif
