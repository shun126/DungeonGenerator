/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonMeshParts.h"
#include "DungeonMeshPartsWithDirection.generated.h"

/**
Mesh parts with direction specification
方向付きメッシュのパーツ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshPartsWithDirection : public FDungeonMeshParts
{
	GENERATED_BODY()

public:
	/*
	Direction of placement
	配置する方向
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	EDungeonPartsPlacementDirection PlacementDirection = EDungeonPartsPlacementDirection::RandomDirection;

	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept;

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif
};