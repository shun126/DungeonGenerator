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

	/**
	 * Target area where the queried part is being selected (room or aisle).
	 *
	 * パーツ選択を行う対象領域（部屋/通路）です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonPartsSelectorTarget Target = EDungeonPartsSelectorTarget::Floor;

	/**
	 * Requested piece type to select, such as floor, wall, or roof.
	 *
	 * 選択対象となるピース種別（床・壁・屋根など）です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 PieceType = 0;

	/**
	 * Rotation index used when matching directional part candidates.
	 *
	 * 方向付きパーツ候補との照合に使う回転インデックスです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 Rotation = 0;

	/**
	 * Bit mask of the six neighboring cells used for adjacency-based filtering.
	 *
	 * 隣接判定に使う6方向セルのビットマスクです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 NeighborMask6 = 0;

	/**
	 * Room identifier used to apply room-specific selection logic.
	 *
	 * 部屋固有の選択ロジックに使う部屋IDです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	/**
	 * Graph depth from the start room used for progression-aware selection.
	 *
	 * 進行度を考慮した選択に使うスタート部屋からの深さです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DepthFromStart = 0;

	/**
	 * Distance to the goal room used for progression-aware selection.
	 *
	 * 進行度を考慮した選択に使うゴール部屋までの距離です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DistanceToGoal = 0;

	/**
	 * Stable seed key used to keep random selection deterministic per location.
	 *
	 * 場所ごとのランダム選択を決定論的にするための固定シードキーです。
	 */
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

	/**
	 * Room identifier used to apply room-specific selection logic.
	 *
	 * 部屋固有の選択ロジックに使う部屋IDです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	/**
	 * Graph depth from the start room used for progression-aware selection.
	 *
	 * 進行度を考慮した選択に使うスタート部屋からの深さです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DepthFromStart = 0;

	/**
	 * Distance to the goal room used for progression-aware selection.
	 *
	 * 進行度を考慮した選択に使うゴール部屋までの距離です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	float DistanceToGoal = 0;

	/**
	 * Stable seed key used to keep random selection deterministic per location.
	 *
	 * 場所ごとのランダム選択を決定論的にするための固定シードキーです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 SeedKey = 0;
};


