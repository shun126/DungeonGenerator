/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonLayoutTypes.h"
#include <CoreMinimal.h>
#include "DungeonSelectionQuery.generated.h"

/**
 * Identifies the kind of dungeon part requested from a custom selector.
 * カスタムセレクターへ要求するダンジョンパーツの種類を識別します。
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
 * Lightweight, read-only context supplied when selecting a dungeon part.
 * Frequently executed selection paths use only POD-like values to avoid object lookups.
 * ダンジョンパーツ選択時に渡される軽量な読み取り専用コンテキストです。
 * 頻繁に実行される選択処理でオブジェクト参照を避けるため、PODに近い値だけを保持します。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPartsQuery
{
	GENERATED_BODY()

	/**
	 * Kind of part being selected, such as a floor, wall, fixture, or door.
	 * 選択対象となる床、壁、設置物、ドアなどのパーツ種別です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Kind of part currently being selected, such as a floor, wall, fixture, or door."))
	EDungeonPartsSelectorTarget Target = EDungeonPartsSelectorTarget::Floor;

	/**
	 * Internal voxel grid type at the selection location.
	 * Values correspond to the generator's internal grid representation and are intended for advanced selectors.
	 * 選択位置にある内部ボクセルグリッドの種類です。
	 * 値は生成器内部のグリッド表現に対応し、上級者向けセレクターで使用します。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Internal voxel grid type at the selection location. This advanced value corresponds to the generator's internal grid representation."))
	uint8 PieceType = 0;

	/**
	 * Rotation index used when matching directional part candidates.
	 *
	 * 方向付きパーツ候補との照合に使う回転インデックスです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Rotation index of the selection location, used by selectors that distinguish directional candidates."))
	uint8 Rotation = 0;

	/**
	 * Bit mask of the six neighboring cells used for adjacency-based filtering.
	 *
	 * 隣接判定に使う6方向セルのビットマスクです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Six-bit adjacency mask for neighboring cells. Use this with topology-aware selectors."))
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
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Identifier of the room that owns this grid cell, or INDEX_NONE when the cell is not owned by a room."))
	int32 RoomId = INDEX_NONE;

	/**
	 * Structural route role assigned to the room that owns this grid.
	 *
	 * このグリッドを所有する部屋に割り当てられた経路構造上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Structural route role of the room that owns this grid cell, such as Start, Goal, Branch, or Dead End."))
	EDungeonRoomStructuralRole RoomStructuralRole = EDungeonRoomStructuralRole::Connector;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Gameplay role of the room that owns this grid cell, such as Combat, Treasure, Puzzle, or Secret."))
	EDungeonRoomGameplayRole RoomGameplayRole = EDungeonRoomGameplayRole::None;

	/**
	 * Zone index assigned by progress and floor conditions.
	 *
	 * 進行度と階層条件で割り当てられたゾーン番号です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Zero-based index of the zone selected for this location, or INDEX_NONE when no zone matches."))
	int32 ZoneIndex = INDEX_NONE;

	/**
	 * Normalized progress from the start room, in the range 0 to 1.
	 *
	 * 進行度を考慮した選択に使う、0から1に正規化されたスタート部屋からの進行度です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Normalized progress from the start room. 0 is near the start and 1 is at the deepest generated progress."))
	float DepthFromStart = 0;

	/**
	 * Inverse normalized progress used as an approximate distance to the goal.
	 *
	 * ゴールまでのおおよその距離として使う、正規化進行度の反転値です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Inverse of Depth From Start. Values near 0 are closer to the deepest generated progress; this is not a world-space distance."))
	float DistanceToGoal = 0;

	/**
	 * Stable seed key used to keep random selection deterministic per location.
	 *
	 * 場所ごとのランダム選択を決定論的にするための固定シードキーです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Stable per-location seed key for deterministic custom selection. Do not treat this value as a sequential index."))
	int32 SeedKey = 0;
};

/**
 * Lightweight, read-only context supplied when selecting a mesh set.
 * Frequently executed selection paths use only POD-like values to avoid object lookups.
 * メッシュセット選択時に渡される軽量な読み取り専用コンテキストです。
 * 頻繁に実行される選択処理でオブジェクト参照を避けるため、PODに近い値だけを保持します。
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
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Identifier of the room that owns this grid cell, or INDEX_NONE when the cell is not owned by a room."))
	int32 RoomId = INDEX_NONE;

	/**
	 * Structural route role assigned to the room that owns this grid.
	 *
	 * このグリッドを所有する部屋に割り当てられた経路構造上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Structural route role of the room that owns this grid cell, such as Start, Goal, Branch, or Dead End."))
	EDungeonRoomStructuralRole RoomStructuralRole = EDungeonRoomStructuralRole::Connector;

	/**
	 * Gameplay role assigned to the room that owns this grid.
	 * このグリッドを所有する部屋に割り当てられたゲームプレイ上の役割です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Gameplay role of the room that owns this grid cell, such as Combat, Treasure, Puzzle, or Secret."))
	EDungeonRoomGameplayRole RoomGameplayRole = EDungeonRoomGameplayRole::None;

	/**
	 * Zone index assigned by progress and floor conditions.
	 *
	 * 進行度と階層条件で割り当てられたゾーン番号です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Zero-based index of the zone selected for this location, or INDEX_NONE when no zone matches."))
	int32 ZoneIndex = INDEX_NONE;

	/**
	 * Normalized progress from the start room, in the range 0 to 1.
	 *
	 * 進行度を考慮した選択に使う、0から1に正規化されたスタート部屋からの進行度です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Normalized progress from the start room. 0 is near the start and 1 is at the deepest generated progress."))
	float DepthFromStart = 0;

	/**
	 * Inverse normalized progress used as an approximate distance to the goal.
	 *
	 * ゴールまでのおおよその距離として使う、正規化進行度の反転値です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Inverse of Depth From Start. Values near 0 are closer to the deepest generated progress; this is not a world-space distance."))
	float DistanceToGoal = 0;

	/**
	 * Stable seed key used to keep random selection deterministic per location.
	 *
	 * 場所ごとのランダム選択を決定論的にするための固定シードキーです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Stable per-location seed key for deterministic custom selection. Do not treat this value as a sequential index."))
	int32 SeedKey = 0;
};


