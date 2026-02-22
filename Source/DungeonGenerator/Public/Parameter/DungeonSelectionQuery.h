/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "DungeonSelectionQuery.generated.h"

/**
 * Kind of dungeon parts requested by custom selector.
 */
UENUM(BlueprintType)
enum class EDungeonPartsSelectorTarget : uint8
{
	Floor UMETA(DisplayName = "Floor", ToolTip = "Select floor parts."),
	Wall UMETA(DisplayName = "Wall", ToolTip = "Select wall parts."),
	Roof UMETA(DisplayName = "Roof", ToolTip = "Select roof parts."),
	Slope UMETA(DisplayName = "Slope", ToolTip = "Select slope parts."),
	Catwalk UMETA(DisplayName = "Catwalk", ToolTip = "Select catwalk parts."),
	Pillar UMETA(DisplayName = "Pillar", ToolTip = "Select pillar parts."),
	Torch UMETA(DisplayName = "Torch", ToolTip = "Select torch parts."),
	Chandelier UMETA(DisplayName = "Chandelier", ToolTip = "Select chandelier parts."),
	Door UMETA(DisplayName = "Door", ToolTip = "Select door parts."),
};

/**
 * Lightweight query parameters for parts selection.
 * Hot path friendly POD-like data only.
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FPartsQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonPartsSelectorTarget Target = EDungeonPartsSelectorTarget::Floor;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 PieceType = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 Rotation = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 NeighborMask6 = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DepthFromStart = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DistanceToGoal = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 SeedKey = 0;
};

/**
 * Lightweight query parameters for mesh-set selection.
 * Hot path friendly POD-like data only.
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FMeshSetQuery
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DepthFromStart = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DistanceToGoal = 0;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 SeedKey = 0;
};
