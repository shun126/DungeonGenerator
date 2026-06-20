/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonLayoutTypes.h"
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
	UniqueDoor UMETA(DisplayName = "Unique Door", ToolTip = "Select unique lock door parts, usually used for goal or boss doors."),
};

/**
 * Lightweight query parameters for parts selection.
 * Hot path friendly POD-like data only.
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPartsQuery
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
	 * Local dungeon grid X coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドX座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid X coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridX = 0;

	/**
	 * Local dungeon grid Y coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドY座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid Y coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridY = 0;

	/**
	 * Local dungeon grid Z coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドZ座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid Z coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridZ = 0;

	/**
	 * Room identifier used to apply room-specific selection logic.
	 *
	 * 部屋固有の選択ロジックに使う部屋IDです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 *
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonRoomStructuralRole RoomStructuralRole = EDungeonRoomStructuralRole::Connector;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonRoomGameplayRole RoomGameplayRole = EDungeonRoomGameplayRole::None;

	/**
	 * Zone index assigned by progress and floor conditions.
	 *
	 * 進行度と階層条件で割り当てられたゾーン番号です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 ZoneIndex = INDEX_NONE;

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
struct DUNGEONGENERATOR_API FDungeonMeshSetQuery
{
	GENERATED_BODY()

	/**
	 * Local dungeon grid X coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドX座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid X coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridX = 0;

	/**
	 * Local dungeon grid Y coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドY座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid Y coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridY = 0;

	/**
	 * Local dungeon grid Z coordinate for the selection target.
	 *
	 * 選択対象のダンジョンローカルグリッドZ座標です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Grid", meta = (ToolTip = "Local dungeon grid Z coordinate for the selection target. This is a grid cell coordinate, not an Unreal world-space value."))
	int32 GridZ = 0;

	/**
	 * Room identifier used to apply room-specific selection logic.
	 *
	 * 部屋固有の選択ロジックに使う部屋IDです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 RoomId = INDEX_NONE;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 *
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonRoomStructuralRole RoomStructuralRole = EDungeonRoomStructuralRole::Connector;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonRoomGameplayRole RoomGameplayRole = EDungeonRoomGameplayRole::None;

	/**
	 * Zone index assigned by progress and floor conditions.
	 *
	 * 進行度と階層条件で割り当てられたゾーン番号です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	int32 ZoneIndex = INDEX_NONE;

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


