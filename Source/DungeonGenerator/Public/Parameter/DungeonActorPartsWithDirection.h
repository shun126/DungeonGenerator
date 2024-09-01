/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonActorParts.h"
#include "DungeonActorPartsWithDirection.generated.h"

/**
Actor parts with direction specification
方向付きアクターのパーツ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorPartsWithDirection : public FDungeonActorParts
{
	GENERATED_BODY()

public:
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept;

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif

public:
	/**
	Direction of placement
	配置する方向
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	EDungeonPartsPlacementDirection PlacementDirection = EDungeonPartsPlacementDirection::RandomDirection;
};
