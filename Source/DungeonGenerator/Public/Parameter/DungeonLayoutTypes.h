/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonDoorActorParts.h"
#include "Parameter/DungeonMeshParts.h"
#include "Parameter/DungeonMeshSetSelectionMethod.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonSelectionPolicy.h"
#include <CoreMinimal.h>
#include <Curves/CurveFloat.h>
#include "DungeonLayoutTypes.generated.h"

class UDungeonInteriorDatabase;
class UDungeonMeshSetDatabase;
class UDungeonPartsSelectorBase;
class UDungeonRoomSensorDatabase;
class UDungeonSubLevelDatabase;

/*
 * Defines how generated rooms may use horizontal and vertical placement.
 * 生成される部屋を水平・垂直方向にどのように配置できるかを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonFloorMode : uint8
{
	Free UMETA(DisplayName = "Free", ToolTip = "Place rooms without direction limits. Rooms may spread horizontally and vertically."),
	Flat UMETA(DisplayName = "Flat", ToolTip = "Place every room on the same floor height. Use this for a single-level dungeon."),
	Vertical UMETA(DisplayName = "Vertical", ToolTip = "Place rooms mainly upward and downward across floors."),
};

/*
 * Defines how the generated start room is selected.
 * 生成される開始部屋をどの基準で選ぶかを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonStartLocationPolicy : uint8
{
	UseNorthernMost UMETA(DisplayName = "Use Northernmost", ToolTip = "Use the northernmost candidate as the start room."),
	UseEasternMost UMETA(DisplayName = "Use Easternmost", ToolTip = "Use the easternmost candidate as the start room."),
	UseWesternMost UMETA(DisplayName = "Use Westernmost", ToolTip = "Use the westernmost candidate as the start room."),
	UseSouthernMost UMETA(DisplayName = "Use Southernmost", ToolTip = "Use the southernmost candidate as the start room."),
	UseHighestPoint UMETA(DisplayName = "Use Highest Point", ToolTip = "Use the highest candidate as the start room."),
	UseLowestPoint UMETA(DisplayName = "Use Lowest Point", ToolTip = "Use the lowest candidate as the start room."),
	UseCentralPoint UMETA(DisplayName = "Use Central Point", ToolTip = "Use the central candidate as the start room."),
	UseMultiStart UMETA(DisplayName = "Use Multi Start", ToolTip = "Use multiple start rooms matched to PlayerStart actors. Keys And Locks progression does not support this option."),
};

/*
 * Defines how the generated goal room is selected.
 * 生成されるゴール部屋をどの基準で選ぶかを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonGoalLocationPolicy : uint8
{
	UseNorthernMost UMETA(DisplayName = "Use Northernmost", ToolTip = "Use the northernmost candidate as the goal room."),
	UseEasternMost UMETA(DisplayName = "Use Easternmost", ToolTip = "Use the easternmost candidate as the goal room."),
	UseWesternMost UMETA(DisplayName = "Use Westernmost", ToolTip = "Use the westernmost candidate as the goal room."),
	UseSouthernMost UMETA(DisplayName = "Use Southernmost", ToolTip = "Use the southernmost candidate as the goal room."),
	UseHighestPoint UMETA(DisplayName = "Use Highest Point", ToolTip = "Use the highest candidate as the goal room."),
	UseLowestPoint UMETA(DisplayName = "Use Lowest Point", ToolTip = "Use the lowest candidate as the goal room."),
	UseCentralPoint UMETA(DisplayName = "Use Central Point", ToolTip = "Use the central candidate as the goal room."),
};

/*
 * Defines the intended progression model for generated routes.
 * 生成される経路の攻略進行モデルを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonProgressionPolicy : uint8
{
	FreeExploration UMETA(DisplayName = "Free Exploration", ToolTip = "Create an open layout with loops, alternate routes, and optional side rooms."),
	StartToGoal UMETA(DisplayName = "Start To Goal", ToolTip = "Create a readable main route from the start room to the goal room."),
	KeysAndLocks UMETA(DisplayName = "Keys And Locks", ToolTip = "Create a solvable key-and-lock route where locked doors cannot be bypassed."),
	BossRoute UMETA(DisplayName = "Boss Route", ToolTip = "Create a main route that builds toward a boss room near the goal."),
	HubQuest UMETA(DisplayName = "Hub Quest", ToolTip = "Create a hub-centered layout with quest-like branches around an early hub room."),
};

/*
 * Defines how aisle ceiling height is selected.
 * 通路の天井高の選び方を定義します。
 */
UENUM(BlueprintType)
enum class EDungeonAisleCeilingHeightPolicy : uint8
{
	TwoGrids UMETA(DisplayName = "2 Grids", ToolTip = "Always use a two-grid ceiling height for aisles."),
	OneGrid UMETA(DisplayName = "Always 1 Grid", ToolTip = "Always use a one-grid ceiling height for aisles."),
	Random UMETA(DisplayName = "Random", ToolTip = "Randomly choose one-grid or two-grid ceiling height for aisles."),
	SIZE UMETA(Hidden, ToolTip = "Internal sentinel value."),
};

/*
 * Defines how often generated actors should appear.
 * 生成アクターの出現頻度を定義します。
 */
UENUM(BlueprintType)
enum class EDungeonFrequencyOfGeneration : uint8
{
	Normally UMETA(DisplayName = "Normally", ToolTip = "Generate with normal frequency."),
	Sometime UMETA(DisplayName = "Sometimes", ToolTip = "Generate occasionally but less than normal."),
	Occasionally UMETA(DisplayName = "Occasionally", ToolTip = "Generate with a moderate low frequency."),
	Rarely UMETA(DisplayName = "Rarely", ToolTip = "Generate infrequently."),
	AlmostNever UMETA(DisplayName = "Almost Never", ToolTip = "Generate only in exceptional cases."),
	Never UMETA(DisplayName = "Never", ToolTip = "Never generate."),
};

/*
 * Defines the route-structure role assigned to generated rooms.
 * 生成された部屋が経路構造上でどの役割を持つかを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonRoomStructuralRole : uint8
{
	Start UMETA(DisplayName = "Start", ToolTip = "Starting room selected by progression settings."),
	Goal UMETA(DisplayName = "Goal", ToolTip = "Goal room selected by progression settings."),
	Hub UMETA(DisplayName = "Hub", ToolTip = "High-connectivity room used as a route hub."),
	Connector UMETA(DisplayName = "Connector", ToolTip = "Main-route room that connects larger route beats."),
	Branch UMETA(DisplayName = "Branch", ToolTip = "Optional side-route room that is not the start or goal."),
	DeadEnd UMETA(DisplayName = "Dead End", ToolTip = "Room with only one route connection, excluding start and goal rooms."),
};

/*
 * Defines the gameplay role assigned to generated rooms.
 * 生成された部屋でプレイヤーにどのような体験をさせるかを定義します。
 */
UENUM(BlueprintType)
enum class EDungeonRoomGameplayRole : uint8
{
	None UMETA(DisplayName = "None", ToolTip = "No special gameplay role is assigned to this room."),
	Combat UMETA(DisplayName = "Combat", ToolTip = "Use this room for enemy encounters or combat-focused events."),
	Treasure UMETA(DisplayName = "Treasure", ToolTip = "Use this room for rewards, loot, keys, or other pickups."),
	Puzzle UMETA(DisplayName = "Puzzle", ToolTip = "Use this room for switches, mechanisms, puzzles, or interaction challenges."),
	Rest UMETA(DisplayName = "Rest", ToolTip = "Use this room as a safe, low-pressure break between stronger encounters."),
	Boss UMETA(DisplayName = "Boss", ToolTip = "Use this room for a major encounter, usually assigned by Boss Route progression."),
	Secret UMETA(DisplayName = "Secret", ToolTip = "Use this room for hidden discoveries, optional rewards, or secret events."),
};

/*
 * Defines gameplay purposes assigned to generated aisles.
 * 生成された通路に割り当てるゲームプレイ上の目的を定義します。
 */
UENUM(BlueprintType)
enum class EDungeonAislePurpose : uint8
{
	MainPath UMETA(DisplayName = "Main Path", ToolTip = "Aisle on the main route from start to goal."),
	Branch UMETA(DisplayName = "Branch", ToolTip = "Aisle leading to an optional branch."),
	Loop UMETA(DisplayName = "Loop", ToolTip = "Aisle that creates an alternate route or loop."),
	Shortcut UMETA(DisplayName = "Shortcut", ToolTip = "Aisle that reconnects distant parts of the route."),
	Locked UMETA(DisplayName = "Locked", ToolTip = "Aisle intended for locked-door progression."),
	VerticalTransition UMETA(DisplayName = "Vertical Transition", ToolTip = "Aisle intended to move between floors."),
};

/*
 * Core structure settings that control dungeon size and room spacing.
 * ダンジョンの規模と部屋の間隔を制御する構造設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonStructureSettings
{
	GENERATED_BODY()

	/*
	 * Target range for the initial generated room count.
	 * 初期生成する部屋数の目標範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (ClampMin = "3", ClampMax = "100", ToolTip = "Target range for the initial generated room count."))
	FInt32Interval RoomCountRange = { 10, 10 };

	/*
	 * Width range for generated rooms.
	 * 生成される部屋の幅の範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (UIMin = "1", ClampMin = "1", ToolTip = "Width range for generated rooms."))
	FInt32Interval RoomWidth = { 3, 8 };

	/*
	 * Depth range for generated rooms.
	 * 生成される部屋の奥行きの範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (UIMin = "1", ClampMin = "1", ToolTip = "Depth range for generated rooms."))
	FInt32Interval RoomDepth = { 3, 8 };

	/*
	 * Height range for generated rooms.
	 * 生成される部屋の高さの範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (UIMin = "1", ClampMin = "1", ToolTip = "Height range for generated rooms."))
	FInt32Interval RoomHeight = { 2, 4 };

	/*
	 * Controls whether rooms can spread freely, stay flat, or stack vertically.
	 * 部屋を自由に広げるか、同じ床高さにそろえるか、垂直方向に並べるかを制御します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (ToolTip = "Controls whether rooms can spread freely, stay on one floor height, or stack vertically. Free allows both horizontal and vertical placement. Flat keeps all rooms at the same floor height. Vertical favors upward and downward layouts."))
	EDungeonFloorMode FloorMode = EDungeonFloorMode::Free;

	/*
	 * Horizontal room-to-room margin.
	 * 水平方向の部屋間隔です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (ClampMin = "0", ToolTip = "Horizontal room-to-room margin. Set this to 0 or a small value when you want rooms to feel clustered without merging walls."))
	uint8 HorizontalRoomMargin = 2;

	/*
	 * Vertical room-to-room margin used when rooms are separated on multi-floor layouts.
	 * 複数階層で部屋を分けるときに使う、垂直方向の部屋間隔です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Structure", meta = (ClampMin = "0", EditCondition = "FloorMode != EDungeonFloorMode::Flat", ToolTip = "Vertical room-to-room margin used by Free and Vertical layouts. Flat floor mode ignores this value."))
	uint8 VerticalRoomMargin = 0;
};

/*
 * Path settings that control route shape, start and goal rooms, and progression gates.
 * 経路形状、開始部屋、ゴール部屋、進行ゲートをまとめて制御する設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPathSettings
{
	GENERATED_BODY()

	/**
	 * Number of layout candidates evaluated before selecting the best dungeon.
	 * 最良のダンジョンを選ぶ前に評価するレイアウト候補数です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ClampMin = "3", ClampMax = "16", ToolTip = "Number of layout candidates evaluated before the best dungeon is selected. Higher values improve selection quality but increase generation cost."))
	uint8 LayoutCandidateCount = 3;

	/**
	 * Primary progression style used to shape routes, branches, loops, and gates.
	 * 経路、分岐、ループ、ゲートの形を決める主な進行スタイルです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ToolTip = "Primary progression style for the dungeon structure. Choose this first: Free Exploration, Start To Goal, Keys And Locks, Boss Route, or Hub Quest. Route ratio, loop density, and corridor complexity are advanced fine-tuning controls inside this style."))
	EDungeonProgressionPolicy ProgressionPolicy = EDungeonProgressionPolicy::StartToGoal;

	/**
	 * Advanced bias toward branch-heavy or main-route-heavy layouts.
	 * 分岐多めまたは主経路重視へ寄せる上級者向け調整です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ClampMin = "-1.00", ClampMax = "1.00", ToolTip = "Advanced tuning for main-route emphasis. 0 uses the selected ProgressionPolicy baseline. Negative values create more branches. Positive values emphasize the main route."))
	float MainRouteBias = 0.0f;

	/**
	 * Advanced adjustment for loops and alternate routes.
	 * ループ経路と代替経路を調整する上級者向け設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ClampMin = "0.00", ClampMax = "1.00", EditCondition = "ProgressionPolicy != EDungeonProgressionPolicy::KeysAndLocks", ToolTip = "Advanced tuning for loops and alternate routes. 0 uses the selected ProgressionPolicy baseline. Higher values add more loops where the policy allows them. Keys And Locks disables unsafe loops so locked doors cannot be bypassed."))
	float LoopRouteDensity = 0.0f;

	/**
	 * Advanced corridor complexity applied after the minimum route network is built.
	 * 最小経路ネットワーク構築後に通路の複雑さを調整する上級者向け設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ClampMin = "0", ClampMax = "10", EditCondition = "ProgressionPolicy != EDungeonProgressionPolicy::KeysAndLocks", ToolTip = "Advanced tuning for extra corridor complexity after the ProgressionPolicy route network is built. 0 adds no extra corridor complexity beyond the policy baseline. Keys And Locks treats this as 0 to keep locked-door routes solvable."))
	uint8 ExtraCorridorComplexity = 0;

	/**
	 * Selection policy for corridor ceiling height.
	 * 通路の天井高を選ぶ方針です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ToolTip = "Corridor ceiling height selection policy. Random varies between one-grid and two-grid corridor ceilings."))
	EDungeonAisleCeilingHeightPolicy CorridorCeilingHeightPolicy = EDungeonAisleCeilingHeightPolicy::Random;

	/**
	 * Policy used to choose the start room.
	 * 開始部屋を選ぶための方針です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ToolTip = "Policy used to choose the generated start room. Use Multi Start creates one start room per PlayerStart actor when supported."))
	EDungeonStartLocationPolicy StartRoomPolicy = EDungeonStartLocationPolicy::UseSouthernMost;

	/**
	 * Policy used to choose the goal room.
	 * ゴール部屋を選ぶための方針です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ToolTip = "Policy used to choose which generated room becomes the goal. ProgressionPolicy controls how strictly the goal is kept as a route endpoint."))
	EDungeonGoalLocationPolicy GoalRoomPolicy = EDungeonGoalLocationPolicy::UseNorthernMost;

	/**
	 * Moves PlayerStart actors to generated start rooms after generation.
	 * 生成完了後にPlayerStartアクターを生成された開始部屋へ移動します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Path", meta = (ToolTip = "Move PlayerStart actors to generated start rooms when generation completes. Disable this when you keep PlayerStart actors in a hand-authored lobby."))
	bool bMovePlayerStartToStartRoom = true;
};

/*
 * Fixture settings that control fixed props such as pillars, torches, and doors.
 * 柱、たいまつ、ドアなどの固定装飾を制御する設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonFixtureSettings
{
	GENERATED_BODY()

	/**
	 * Selection policy used when choosing pillar parts.
	 * 柱パーツを選ぶときに使用する選択方法です。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Pillar", meta = (DisplayName = "Pillar Parts Selector", ToolTip = "Selects one pillar part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> PillarPartsSelector;

	UPROPERTY()
	EDungeonSelectionPolicy PillarPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy pillar selection method retained for asset migration.
	 * アセット移行のために保持している旧式の柱選択方法です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Candidate pillar mesh parts.
	 * 柱として使用する候補メッシュパーツです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Pillar", meta = (ToolTip = "Pillar mesh parts."))
	TArray<FDungeonMeshParts> PillarParts;

	/**
	 * Selection policy used when choosing torch actors.
	 * たいまつアクターを選ぶときに使用する選択方法です。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Torch", meta = (DisplayName = "Torch Parts Selector", ToolTip = "Selects one torch actor part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> TorchPartsSelector;

	UPROPERTY()
	EDungeonSelectionPolicy TorchPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy torch selection method retained for asset migration.
	 * アセット移行のために保持している旧式のたいまつ選択方法です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Frequency used when generating torch lights.
	 * たいまつを生成するときに使用する頻度です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Torch", meta = (ToolTip = "Frequency of torchlight generation."))
	EDungeonFrequencyOfGeneration FrequencyOfTorchlightGeneration = EDungeonFrequencyOfGeneration::Rarely;

	/**
	 * Candidate torch actor parts.
	 * たいまつとして使用する候補アクターパーツです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Torch", meta = (ToolTip = "Torch actor parts."))
	TArray<FDungeonRandomActorParts> TorchParts;

	/**
	 * Selection policy used when choosing door actors.
	 * ドアアクターを選ぶときに使用する選択方法です。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Door", meta = (DisplayName = "Door Parts Selector", ToolTip = "Selects one door actor part from the candidates. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> DoorPartsSelector;

	UPROPERTY()
	EDungeonSelectionPolicy DoorPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy door selection method retained for asset migration.
	 * アセット移行のために保持している旧式のドア選択方法です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Candidate door actor parts.
	 * ドアとして使用する候補アクターパーツです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Door", meta = (ToolTip = "Door actor parts."))
	TArray<FDungeonDoorActorParts> DoorParts;

	/*
	 * Selection policy used when choosing unique lock door actors.
	 * Unique Lock のドアアクターを選ぶときに使用する選択ポリシーです。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Door", meta = (DisplayName = "Unique Door Parts Selector", ToolTip = "Selects one Unique Lock door actor part, usually used for goal or boss doors. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonPartsSelectorBase> UniqueDoorPartsSelector;

	UPROPERTY()
	EDungeonSelectionPolicy UniqueDoorPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/*
	 * Legacy unique door selection method retained for asset migration.
	 * アセット移行のために保持している旧式の Unique Door 選択方式です。
	 */
	UPROPERTY()
	EDungeonPartsSelectionMethod UniqueDoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	 * Candidate actor parts used for Unique Lock doors.
	 * Unique Lock のドアとして使用する候補アクターパーツです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|Fixtures|Door", meta = (ToolTip = "Door actor parts used only for Unique Lock doors. Leave empty to use Door Parts as a fallback."))
	TArray<FDungeonDoorActorParts> UniqueDoorParts;

	/**
	 * Custom parts selector used by custom selection policies.
	 * カスタム選択方法で使用するパーツ選択オブジェクトです。
	 */
	UPROPERTY()
	TObjectPtr<UDungeonPartsSelectorBase> DungeonPartsSelector;
};

/*
 * Theme override used by zones.
 * ゾーンで使用する見た目の上書き設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonZoneThemeOverride
{
	GENERATED_BODY()

	/**
	 * Optional room mesh database override used while this zone is active.
	 * このゾーンが有効な間に使用する任意の部屋用メッシュデータベース上書きです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Optional room mesh database override for this zone."))
	TObjectPtr<UDungeonMeshSetDatabase> RoomMeshSetDatabase;

	/**
	 * Optional aisle mesh database override used while this zone is active.
	 * このゾーンが有効な間に使用する任意の通路用メッシュデータベース上書きです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Optional aisle mesh database override for this zone."))
	TObjectPtr<UDungeonMeshSetDatabase> AisleMeshSetDatabase;

	/*
	 * Enables the interior database override even when the database is empty.
	 * データベースが未設定の場合でも、内装データベースの上書きを有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (InlineEditConditionToggle, ToolTip = "Enable this to replace the inherited interior database. Leave the database empty while enabled to intentionally disable interior decoration."))
	bool bOverrideDungeonInteriorDatabase = false;


	/*
	 * Enables fixture overrides even when all candidate lists are empty.
	 * 候補リストが空の場合でも、Fixture の上書きを有効にします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (InlineEditConditionToggle, ToolTip = "Enable this to replace inherited pillar, torch, and door fixture settings. Empty lists intentionally disable those fixtures."))
	bool bOverrideFixtures = false;

	/**
	 * Optional fixture override used by matching rooms or zones.
	 * 条件に一致する部屋またはゾーンで使用する任意の Fixture 上書きです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (EditCondition = "bOverrideFixtures", ToolTip = "Fixture override for this role or zone. This replaces inherited pillar, torch, door, unique door, frequency, and custom selector settings."))
	FDungeonFixtureSettings Fixtures;
};

/*
 * Gameplay override used by room-role profiles.
 * RoomRole profiles can replace the default room sensor class.
 * RoomRole プロファイルでデフォルトの Room Sensor クラスを上書きします。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomRoleGameplayOverride
{
	GENERATED_BODY()

	/*
	 * Optional room sensor class override used by rooms with this gameplay role.
	 * Rooms with this gameplay role can use this Room Sensor class instead of the zone or default class.
	 * この Gameplay Role の部屋では Zone またはデフォルトの代わりにこの Room Sensor クラスを使えます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (AllowedClasses = "/Script/DungeonGenerator.DungeonRoomSensorBase", ToolTip = "Optional room sensor class override for rooms with this gameplay role. Leave empty to use the zone or default gameplay room sensor."))
	TObjectPtr<UClass> DungeonRoomSensorClass;
};

/*
 * Gameplay override used by zones.
 * Zones can replace the default room sensor and aisle actor settings.
 * Zone でデフォルトの Room Sensor と通路 Actor 設定を上書きします。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonZoneGameplayOverride
{
	GENERATED_BODY()

	/*
	 * Optional room sensor class override used while this zone is active.
	 * Rooms in this zone can use this Room Sensor class unless their RoomRole overrides it.
	 * この Zone の部屋では RoomRole 上書きがない場合にこの Room Sensor クラスを使えます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (AllowedClasses = "/Script/DungeonGenerator.DungeonRoomSensorBase", ToolTip = "Optional room sensor class override for rooms in this zone. RoomRole overrides take priority over this value."))
	TObjectPtr<UClass> DungeonRoomSensorClass;

	/*
	 * Optional actor classes spawned inside aisles while this zone is active.
	 * Aisles in this zone can spawn these actor Blueprints instead of the default gameplay aisle actors.
	 * この Zone の通路ではデフォルトの代わりにこれらの Actor Blueprint をスポーンできます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (AllowedClasses = "/Script/Engine.Blueprint", ToolTip = "Optional actor Blueprints spawned inside aisles in this zone. Leave empty to use Gameplay.SpawnActorInAisle."))
	TArray<FSoftObjectPath> SpawnActorInAisle;
};

/**
 * Profile that controls one gameplay room role.
 * 1つのゲームプレイ部屋役割を制御するプロファイルです。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomRoleProfile
{
	GENERATED_BODY()

	/**
	 * Gameplay role affected by this profile.
	 * このプロファイルが対象にするゲームプレイ上の部屋役割です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (ToolTip = "Gameplay room role affected by this profile. None, Combat, Treasure, Puzzle, Rest, and Secret can be sampled for branch rooms. Boss is assigned by Boss Route progression and uses this profile only for role-specific theme overrides."))
	EDungeonRoomGameplayRole Role = EDungeonRoomGameplayRole::Combat;

	/**
	 * Relative weight used when assigning roles to generated branch rooms.
	 * 生成された分岐部屋へ役割を割り当てるときに使う相対重みです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (ClampMin = "0.00", ToolTip = "Relative weight used when assigning this gameplay role to generated branch rooms. None, Combat, Treasure, Puzzle, Rest, and Secret are sampled. Boss ignores this value."))
	float BranchSelectionWeight = 1.f;

	/**
	 * Theme override used by rooms with this role.
	 * この役割の部屋に使用するテーマ上書きです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (ToolTip = "Optional theme override used by rooms with this role. Room mesh overrides affect room mesh selection only. Aisle meshes still use zone or default theme settings."))
	FDungeonZoneThemeOverride ThemeOverride;

	/*
	 * Gameplay override used by rooms with this role.
	 * Rooms with this role can replace the default or zone room sensor class.
	 * この役割の部屋でデフォルトまたは Zone の Room Sensor クラスを上書きします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (ToolTip = "Optional gameplay override used by rooms with this role. Room sensor class overrides affect room sensor selection only."))
	FDungeonRoomRoleGameplayOverride GameplayOverride;
};

/**
 * Settings that control room roles, role visuals, and special room selection.
 * 部屋の役割、見た目、特殊部屋選択を制御する設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomRoleSettings
{
	GENERATED_BODY()

	/**
	 * Role profiles used for branch role sampling and role-specific room mesh overrides.
	 * 分岐部屋の役割抽選と役割別の部屋メッシュ上書きに使うプロファイル一覧です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay|RoomRoles", meta = (ToolTip = "Profiles that tune gameplay roles, room visuals, and random special-room selection. Branch room selection uses positive weights from None, Combat, Treasure, Puzzle, Rest, and Secret profiles."))
	TArray<FDungeonRoomRoleProfile> Roles;
};

/*
 * Definition of a dungeon zone or biome.
 * ダンジョン内のゾーンまたはバイオーム定義です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonZoneDefinition
{
	GENERATED_BODY()

	/**
	 * Display name used to identify this zone.
	 * このゾーンを識別するための表示名です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Display name used to identify this zone."))
	FName Name;

	/**
	 * Start-to-goal progress range covered by this zone.
	 * このゾーンが適用されるスタートからゴールまでの進行度範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ClampMin = "0.00", ClampMax = "1.00", ToolTip = "Progress range covered by this zone."))
	FFloatInterval ProgressRange = { 0.f, 1.f };

	/**
	 * Floor range covered by this zone.
	 * このゾーンが適用される階層範囲です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ClampMin = "0", ToolTip = "Floor range covered by this zone. The default range covers all generated floors."))
	FInt32Interval FloorRange = { 0, MAX_int32 };

	/**
	 * Relative weight used when multiple zones match the same progress and floor.
	 * 同じ進行度と階層に複数の Zone が一致したときに使う相対重みです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ClampMin = "0.00", ToolTip = "Relative weight used only among zones whose ProgressRange and FloorRange both match. Set to 0 to keep this zone from being randomly selected."))
	float SelectionWeight = 1.f;

	/**
	 * Theme override used by this zone when matching conditions are met.
	 * 条件に一致したときにこのゾーンで使用するテーマ上書きです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Theme overrides used while generating this zone."))
	FDungeonZoneThemeOverride ThemeOverride;

	/*
	 * Gameplay override used by this zone when matching conditions are met.
	 * This zone can replace the default room sensor and aisle actor settings.
	 * 条件に一致したとき、この Zone でデフォルトの Room Sensor と通路 Actor 設定を上書きします。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Gameplay overrides used while generating this zone. RoomRole sensor overrides take priority for rooms."))
	FDungeonZoneGameplayOverride GameplayOverride;
};

/*
 * Zone settings that define biome-like ranges across the dungeon.
 * ダンジョン内のバイオーム的な範囲を定義するゾーン設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonZoneSettings
{
	GENERATED_BODY()

	/**
	 * Ordered zones matched by progress and floor during generation.
	 * 生成時に進行度と階層で順番に照合されるゾーン一覧です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Zones", meta = (ToolTip = "Ordered list of zones used by progress and floor."))
	TArray<FDungeonZoneDefinition> Zones;
};

/*
 * Gameplay settings for room roles, room sensors, and reserved sublevels.
 * 部屋役割、ルームセンサー、予約サブレベルを制御するゲームプレイ設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGameplaySpawnSettings
{
	GENERATED_BODY()

	/**
	 * Room role settings used for branch roles, room visuals, and special room selection.
	 * 分岐部屋の役割、部屋の見た目、特殊部屋選択に使う設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay", meta = (ShowOnlyInnerProperties, ToolTip = "Settings for room roles, room visuals, and special-room selection. This does not control room shape or room count."))
	FDungeonRoomRoleSettings RoomRoles;

	/*
	 * Default room sensor class spawned for generated rooms.
	 * Generated rooms use this Room Sensor class unless a Zone or RoomRole override replaces it.
	 * Zone または RoomRole の上書きがない生成部屋では、この Room Sensor クラスを使います。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay", meta = (AllowedClasses = "/Script/DungeonGenerator.DungeonRoomSensorBase", ToolTip = "Default Room Sensor Blueprint used for generated rooms. RoomRole overrides take priority, then Zone overrides, then this default value."))
	TObjectPtr<UClass> DungeonRoomSensorClass;

	/*
	 * Default actor classes spawned inside generated aisles.
	 * Generated aisles can spawn these actor Blueprints unless a Zone override replaces them.
	 * Zone の上書きがない生成通路では、これらの Actor Blueprint をスポーンできます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Gameplay", meta = (AllowedClasses = "/Script/Engine.Blueprint", ToolTip = "Default actor Blueprints spawned inside generated aisles. Zone overrides can replace this list for matching aisle zones."))
	TArray<FSoftObjectPath> SpawnActorInAisle;

	/**
	 * Deprecated v1 room sensor database retained only for v2.0.0 migration.
	 * v2.0.0 の移行専用に残されている旧ルームセンサーデータベースです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Gameplay.DungeonRoomSensorClass and Gameplay.SpawnActorInAisle instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UDungeonRoomSensorDatabase> DungeonRoomSensorDatabase;

};

/*
 * Theme settings that control meshes, interiors, fixtures, and visual selection.
 * メッシュ、内装、設置物、見た目の選択を制御するテーマ設定です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonThemeSettings
{
	GENERATED_BODY()

	/*
	 * Horizontal grid size in world units used to align meshes, sublevels, interiors, and generated room placement.
	 * メッシュ、サブレベル、内装、生成部屋の配置を揃えるためのワールド単位の水平グリッドサイズです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme", meta = (ClampMin = "1", DisplayName = "Horizontal Grid Size", ToolTip = "Horizontal grid size in world units used to align meshes, sublevels, interiors, and generated room placement. Match this to the horizontal size of your room and aisle assets."))
	float HorizontalGridSize = 400.f;

	/*
	 * Vertical grid size in world units used to align floors, sublevels, interiors, and generated room height.
	 * 床高さ、サブレベル、内装、生成部屋の高さを揃えるためのワールド単位の垂直グリッドサイズです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme", meta = (ClampMin = "1", DisplayName = "Vertical Grid Size", ToolTip = "Vertical grid size in world units used to align floors, sublevels, interiors, and generated room height. Match this to the vertical size of your room and aisle assets."))
	float VerticalGridSize = 400.f;

	/**
	 * Mesh parts database used for generated rooms.
	 * 生成された部屋に使用するメッシュパーツデータベースです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme", meta = (ToolTip = "Room mesh parts database."))
	TObjectPtr<UDungeonMeshSetDatabase> DungeonRoomMeshPartsDatabase;

	/**
	 * Mesh parts database used for generated aisles.
	 * 生成された通路に使用するメッシュパーツデータベースです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme", meta = (ToolTip = "Aisle mesh parts database."))
	TObjectPtr<UDungeonMeshSetDatabase> DungeonAisleMeshPartsDatabase;


	/**
	 * Default fixture settings used when no role or zone override replaces them.
	 * Role または Zone の上書きがない場合に使用する既定の Fixture 設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme", meta = (ShowOnlyInnerProperties, ToolTip = "Default fixture settings for pillars, torches, doors, and custom fixture selection."))
	FDungeonFixtureSettings Fixtures;

	/*
	 * Spawn vegetation instances over multiple frames to reduce Play start stalls.
	 * Play開始時の停止を抑えるため、植生インスタンス生成を複数フレームに分散します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|VegetationPerformance", meta = (ToolTip = "Spawn vegetation instances over multiple frames. This keeps generation completion timing unchanged while foliage appears over the next frames."))
	bool bDeferredVegetationSpawn = true;

	/*
	 * Maximum vegetation candidates processed per frame while deferred spawning is enabled.
	 * 遅延植生生成が有効なとき、1フレームで処理する植生候補数の上限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|VegetationPerformance", meta = (ClampMin = "1", ToolTip = "Maximum vegetation candidates processed per frame while deferred spawning is enabled."))
	int32 MaxVegetationSpawnsPerFrame = 128;

	/*
	 * Maximum milliseconds spent placing vegetation instances per frame.
	 * 1フレームで植生インスタンス配置に使う最大ミリ秒です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|VegetationPerformance", meta = (ClampMin = "0.0", ToolTip = "Maximum milliseconds spent placing vegetation instances per frame. Lower values reduce hitches but make foliage appear over more frames."))
	float MaxVegetationSpawnTimeMs = 2.0f;

	/*
	 * Maximum foliage component tree builds processed per frame.
	 * 1フレームで処理するフォリッジコンポーネントのツリー構築数の上限です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|VegetationPerformance", meta = (ClampMin = "1", ToolTip = "Maximum foliage component tree builds processed per frame after deferred vegetation placement finishes."))
	int32 MaxVegetationTreeBuildsPerFrame = 2;

	/*
	 * Maximum milliseconds spent rebuilding foliage trees per frame.
	 * 1フレームでフォリッジツリー再構築に使う最大ミリ秒です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Theme|VegetationPerformance", meta = (ClampMin = "0.0", ToolTip = "Maximum milliseconds spent rebuilding foliage trees per frame after deferred vegetation placement finishes."))
	float MaxVegetationTreeBuildTimeMs = 1.0f;


};

/*
 * Metrics measured from a generated dungeon layout.
 * 生成されたダンジョンレイアウトから計測した指標です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonLayoutMetrics
{
	GENERATED_BODY()

	/**
	 * Number of rooms in the selected layout.
	 * 選択されたレイアウトに含まれる部屋数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 RoomCount = 0;

	/**
	 * Number of aisles in the selected layout.
	 * 選択されたレイアウトに含まれる通路数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 AisleCount = 0;

	/**
	 * Number of rooms or segments on the critical path.
	 * 主経路に含まれる部屋または区間の数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 CriticalPathLength = 0;

	/**
	 * Number of branch aisles in the selected layout.
	 * 選択されたレイアウトに含まれる分岐通路数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 BranchCount = 0;

	/**
	 * Number of loop or shortcut aisles in the selected layout.
	 * 選択されたレイアウトに含まれるループまたはショートカット通路数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 LoopCount = 0;

	/**
	 * Number of dead-end rooms in the selected layout.
	 * 選択されたレイアウトに含まれる行き止まり部屋数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 DeadEndCount = 0;

	/**
	 * Number of dead-end rooms assigned a special room archetype.
	 * 特殊な部屋役割が割り当てられた行き止まり部屋数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 SpecialDeadEndCount = 0;

	/**
	 * Ratio of dead-end rooms assigned a special room archetype.
	 * 行き止まり部屋のうち特殊な部屋役割が割り当てられた割合です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float SpecialDeadEndCoverage = 0.f;

	/**
	 * Number of vertical transition aisles in the selected layout.
	 * 選択されたレイアウトに含まれる上下移動通路数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 VerticalTransitionCount = 0;

	/**
	 * Distance between the selected start and goal rooms.
	 * 選択された開始部屋とゴール部屋の距離です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float StartGoalDistance = 0.f;

	/**
	 * Whether the generated mission route can reach the goal.
	 * 生成されたミッション経路でゴールへ到達できるかどうかです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	bool bMissionSolvable = false;

	/**
	 * Number of locked-route aisles in the selected layout.
	 * 選択されたレイアウトに含まれる鍵付き経路の通路数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 LockedRouteCount = 0;

	/**
	 * Number of rooms assigned the secret archetype.
	 * Secret役割が割り当てられた部屋数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 SecretRoomCount = 0;

	/**
	 * Number of matched zones in the selected layout.
	 * 選択されたレイアウトで一致したゾーン数です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 ZoneCount = 0;

	/**
	 * Average room intensity measured across the selected layout.
	 * 選択されたレイアウト全体で計測した部屋強度の平均値です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float AverageIntensity = 0.f;

	/**
	 * Sum of weighted room-to-room gaps across all aisles.
	 * 全通路における重み付き部屋間距離の合計です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float TotalAisleDistance = 0.f;

	/**
	 * Average weighted room-to-room gap across all aisles.
	 * 全通路における重み付き部屋間距離の平均です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float AverageAisleDistance = 0.f;

	/**
	 * Longest weighted room-to-room gap across all aisles.
	 * 全通路における最長の重み付き部屋間距離です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float MaxAisleDistance = 0.f;

	/**
	 * Sum of weighted room-to-room gaps on main-path aisles.
	 * 主経路通路における重み付き部屋間距離の合計です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float MainPathAisleDistance = 0.f;
};

/*
 * Score assigned to a generated dungeon layout candidate.
 * 生成されたダンジョンレイアウト候補に付与するスコアです。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonLayoutScore
{
	GENERATED_BODY()

	/**
	 * Candidate index that produced this score.
	 * このスコアを生成した候補番号です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	int32 CandidateIndex = INDEX_NONE;

	/**
	 * Total score assigned to the layout candidate.
	 * レイアウト候補に割り当てられた合計スコアです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	float TotalScore = 0.f;

	/**
	 * Whether the candidate passed the minimum layout checks.
	 * 候補が最低限のレイアウト検査を通過したかどうかです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	bool bAccepted = false;

	/**
	 * Human-readable reason for accepting or rejecting the candidate.
	 * 候補を採用または却下した理由を表す人が読める文章です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Layout")
	FString Reason;
};
