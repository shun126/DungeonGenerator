/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonGeneratorAssetVersion.h"
#include "DungeonGridSize.h"
#include "Migration/DungeonAssetMigration.h"
#include "Mission/DungeonRoomProps.h"
#include "DungeonLayoutTypes.h"
#include "DungeonMeshSetDatabase.h"
#include <CoreMinimal.h>
#include <functional>
#include <memory>
#include "DungeonGenerateParameter.generated.h"

// forward declaration
class UDungeonAisleGridMap;
class UDungeonRandom;
class UDungeonRoomSensorDatabase;
class UDungeonPartsSelectorBase;
struct FPropertyChangedEvent;
struct FDungeonAisleGrid;

/**
 * Legacy dungeon expansion policy serialized by 1.x UDungeonGenerateParameter assets.
 * 1.xのUDungeonGenerateParameterアセットで保存されていた旧ダンジョン展開方針です。
 */
UENUM()
enum class EDungeonExpansionPolicy : uint8
{
	Flat UMETA(DisplayName = "Flat", ToolTip = "Legacy value migrated to Structure.FloorMode = Flat."),
	ExpandHorizontally UMETA(DisplayName = "Expand Horizontally", ToolTip = "Legacy value migrated to Structure.FloorMode = Free."),
	ExpandVertically UMETA(DisplayName = "Expand Vertically", ToolTip = "Legacy value migrated to Structure.FloorMode = Vertical."),
	ExpandAnyDirection UMETA(DisplayName = "Expand Any Direction", ToolTip = "Legacy value migrated to Structure.FloorMode = Free."),
};

/**
 * Top-level dungeon definition asset used to configure structure, route, progression, gameplay, and theme.
 * 構造、経路、攻略進行、ゲームプレイ、テーマを設定する最上位のダンジョン定義アセットです。
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject, public IDungeonMigratableAsset
{
	GENERATED_BODY()

public:

	/**
	 * Constructs a dungeon parameter asset with initialized grouped settings.
	 * グループ化された設定を初期化してダンジョンパラメータアセットを構築します。
	 */
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);

	/**
	 * Destroys the ~UDungeonGenerateParameter instance.
	 * ダンジョンパラメータアセットを破棄します。
	 */
	virtual ~UDungeonGenerateParameter() override = default;

	/**
	 * Returns RandomSeed.
	 * ダンジョン生成開始時に使用する設定済み乱数シードを返します。
	 */
	int32 GetRandomSeed() const;

	/**
	 * Returns GeneratedRandomSeed.
	 * 直近のダンジョン生成で実際に使用した乱数シードを返します。
	 */
	int32 GetGeneratedRandomSeed() const;

	/**
	 * Gets the maximum target room count from Structure.RoomCountRange.
	 * Structure.RoomCountRangeの最大値から目標部屋数を取得します。
	 */
	int32 GetNumberOfCandidateRooms() const;

	/**
	 * Returns RoomWidth.
	 * 水平グリッドセル単位で設定された部屋幅の範囲を返します。
	 */
	const FInt32Interval& GetRoomWidth() const noexcept;

	/**
	 * Returns RoomDepth.
	 * 水平グリッドセル単位で設定された部屋奥行きの範囲を返します。
	 */
	const FInt32Interval& GetRoomDepth() const noexcept;

	/**
	 * Returns RoomHeight.
	 * 垂直グリッドセル単位で設定された部屋高さの範囲を返します。
	 */
	const FInt32Interval& GetRoomHeight() const noexcept;

	/**
	 * Returns HorizontalRoomMargin.
	 * グリッドセル単位の水平方向の部屋間余白を返します。
	 */
	int32 GetHorizontalRoomMargin() const noexcept;

	/**
	 * Returns VerticalRoomMargin.
	 * グリッドセル単位の垂直方向の部屋間余白を返します。
	 */
	int32 GetVerticalRoomMargin() const noexcept;

	/**
	 * Unrealワールド単位の水平・垂直グリッドサイズを返します。
	 */
	FDungeonGridSize GetGridSize() const;

	/**
	 * PlayerStartアクターを生成された開始部屋へ移動するかを返します。
	 */
	bool IsMovePlayerStartToStartingPoint() const noexcept;

	/**
	 * Returns whether UseMissionGraph.
	 * UseMissionGraph かどうかを返します。
	 */
	bool IsUseMissionGraph() const noexcept;

	/**
	 * Gets the number of layout candidates evaluated before generation is committed.
	 * 生成確定前に評価するレイアウト候補数を取得します。
	 */
	int32 GetLayoutCandidateCount() const noexcept;

	/**
	 * Gets additional corridor complexity, or 0 while Keys And Locks progression is active.
	 * AisleComplexity を返します。
	 */
	uint8 GetAisleComplexity() const noexcept;

	/**
	 * Returns whether AisleComplexity.
	 * 現在の進行方針で追加通路の複雑度が有効かを返します。
	 */
	bool IsAisleComplexity() const noexcept;

	/**
	 * Returns AisleCeilingHeightPolicy.
	 * 通路の天井高を選択するポリシーを返します。
	 */
	EDungeonAisleCeilingHeightPolicy GetAisleCeilingHeightPolicy() const noexcept;

	/**
	 * Returns the default-theme frequency used to generate torch actors in rooms.
	 * 部屋にたいまつアクターを生成する既定テーマの頻度を返します。
	 */
	EDungeonFrequencyOfGeneration GetRoomTorchFrequency() const noexcept;

	/**
	 * Returns the default-theme frequency used to generate torch actors in aisles.
	 * 通路にたいまつアクターを生成する既定テーマの頻度を返します。
	 */
	EDungeonFrequencyOfGeneration GetAisleTorchFrequency() const noexcept;

	/**
	 * Returns the grouped structure settings.
	 * グループ化された構造設定を返します。
	 */
	const FDungeonStructureSettings& GetStructureSettings() const noexcept;

	/**
	 * Returns the grouped route and progression settings.
	 * グループ化された経路・攻略進行設定を返します。
	 */
	const FDungeonPathSettings& GetPathSettings() const noexcept;

	/**
	 * Returns the room-role profile settings.
	 * 部屋役割プロファイル設定を返します。
	 */
	const FDungeonRoomRoleSettings& GetRoomRoleSettings() const noexcept;

	/**
	 * Returns the ordered zone definitions.
	 * 順序付けされたゾーン定義を返します。
	 */
	const FDungeonZoneSettings& GetZoneSettings() const noexcept;

	/**
	 * Returns gameplay spawning and reserved-sublevel settings.
	 * ゲームプレイ用スポーンと予約サブレベルの設定を返します。
	 */
	const FDungeonGameplaySpawnSettings& GetGameplaySpawnSettings() const noexcept;
	
	/**
	 * Returns mesh, interior, and fixture theme settings.
	 * メッシュ、内装、設置物のテーマ設定を返します。
	 */
	const FDungeonThemeSettings& GetThemeSettings() const noexcept;

	/**
	 * Resolves fixture settings in room-role, zone, then default-theme priority order.
	 * Room grids can use role and zone overrides; aisle grids use zone and default settings.
	 * 部屋役割、ゾーン、既定テーマの優先順で設置物設定を解決します。
	 * 部屋グリッドでは役割とゾーンの上書きを使用でき、通路グリッドではゾーンと既定設定を使用します。
	 */
	const FDungeonFixtureSettings& ResolveFixtureSettings(EDungeonRoomGameplayRole gameplayRole, int32 zoneIndex, bool bRoomGrid) const noexcept;

	/**
	 * Resolves aisle-slope base-light settings in zone, then default-theme priority order.
	 * 通路スロープ用ベースライト設定をZone、既定Themeの優先順で解決します。
	 */
	const FDungeonAisleSlopeBaseLightSettings& ResolveAisleSlopeBaseLightSettings(int32 zoneIndex) const noexcept;


	/**
	 * Represents for.
	 * for を表します。
	 */
	UClass* GetRoomSensorClass() const;

	/**
	 * Resolves the room sensor class for a generated room.
	 * 生成された部屋に使用する Room Sensor クラスを解決します。
	 */
	UClass* ResolveRoomSensorClass(EDungeonRoomGameplayRole gameplayRole, int32 zoneIndex) const;

	/**
	 * Deprecated v1 room sensor database accessor kept only for v2.0.0 migration checks.
	 * v2.0.0 の移行確認専用に残されている旧 Room Sensor Database 取得関数です。
	 */
	UE_DEPRECATED(5.0, "Use GetGameplaySpawnSettings().DungeonRoomSensorClass and GetGameplaySpawnSettings().SpawnActorInAisle instead. v1 room sensor database migration support may be removed in v2.1 or later.")
	UDungeonRoomSensorDatabase* GetRoomSensorDatabase() const;

	/**
	 * Converts from a grid coordinate system to a world coordinate system
	 *
	 * グリッド座標系からワールド座標系への変換
	 */
	FVector ToWorld(const FIntVector& location) const;

	/**
	 * Converts from a grid coordinate system to a world coordinate system
	 *
	 * グリッド座標系からワールド座標系への変換
	 */
	FVector ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const;

	/**
	 * Converts from a grid coordinate system to a world coordinate system
	 *
	 * グリッド座標系からワールド座標系への変換
	 */
	FIntVector ToGrid(const FVector& location) const;

	/**
	 * コピー元アセットから再利用可能なコンテンツ設定をコピーし、構造設定をランダム化したパラメータを生成します。
	 * @param sourceParameter Source asset whose mesh and related content settings are copied. メッシュなどのコンテンツ設定をコピーする元アセットです。
	 * @return Newly created randomized parameter asset. 新しく生成されたランダムパラメータアセットです。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Create randomized structural settings while copying reusable mesh and content settings from the source parameter asset."))
	static UDungeonGenerateParameter* GenerateRandomParameter(const UDungeonGenerateParameter* sourceParameter) noexcept;

	/**
	 * Randomizes the editable structural settings on this asset.
	 * このアセットの編集可能な構造設定をランダム化します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Randomize the editable structural settings on this parameter asset."))
	void SetRandomParameter() noexcept;

#if WITH_EDITOR

#endif

private:
#if WITH_EDITORONLY_DATA
	/**
	 * Saved asset used as the content source for this temporary generated parameter.
	 * この一時生成ParameterのContent作成元として使用した保存済みAssetです。
	 */
	UPROPERTY(Transient)
	TObjectPtr<UDungeonGenerateParameter> GenerationSourceParameterAsset;
#endif

	/**
	 * Sets RandomSeed.
	 * ダンジョン生成に使用する乱数の種を設定します
	 */
	void SetRandomSeed(const int32 generateRandomSeed);

	/**
	 * Sets GeneratedRandomSeed.
	 * ダンジョン生成に使用した乱数の種を設定します
	 */
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

	const FDungeonMeshSet* SelectMeshSet(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;

	const UDungeonMeshSetDatabase* GetDungeonRoomMeshPartsDatabase() const noexcept;
	const UDungeonMeshSetDatabase* GetDungeonAisleMeshPartsDatabase() const noexcept;
	const UDungeonMeshSetDatabase* GetDungeonMeshPartsDatabase(const FIntVector& gridLocation, const dungeon::Grid& grid) const noexcept;
	const FDungeonZoneThemeOverride* ResolveThemeOverride(EDungeonRoomGameplayRole gameplayRole, int32 zoneIndex, bool bRoomGrid) const noexcept;
	static FDungeonMeshSetQuery MakeMeshSetQuery(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid);
	const FDungeonMeshPartsWithDirection* SelectFloorParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;
	const FDungeonMeshParts* SelectCatwalkParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;
	const FDungeonMeshParts* SelectWallPartsByGrid(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;
	static const FDungeonMeshParts* SelectWallPartsByFace(const FDungeonMeshSet* dungeonMeshSet, const FIntVector& gridLocation, const dungeon::Direction& direction);
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;
	const FDungeonMeshParts* SelectSlopeParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;

	const FDungeonMeshParts* SelectPillarParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonRandomActorParts* SelectTorchParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonRandomActorParts* SelectChandelierParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const;
	const FDungeonDoorActorParts* SelectDoorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, EDungeonRoomProps props, const std::shared_ptr<dungeon::Random>& random) const;

	void EachFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachPillarParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	void EachRoomFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachRoomWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoomRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachRoomSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoomCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	void EachAisleFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachAisleWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachAisleRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachAisleSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachAisleCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	int32 GetGeneratedDungeonCRC32() const noexcept;
	void SetGeneratedDungeonCRC32(const int32 generatedDungeonCRC32) noexcept;

	/**
	 * ダンジョン生成終了時に通知されるイベント
	 * @param synchronizedRandom クライアント・サーバー間で同期される乱数
	 * @param aisleGridMap 通路のグリッドを記録したコンテナ
	 * @param spawnActor アクターをスポーンする関数
	 */
	void OnEndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap, const std::function<void(const FSoftObjectPath&, const FTransform&)>& spawnActor) const;
	const TArray<FSoftObjectPath>& ResolveSpawnActorInAisle(const FDungeonAisleGrid& aisleGrid) const;

public:
	// overrides
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual int32 GetLoadedDungeonAssetVersion() const override;
#if WITH_EDITOR
	virtual FDungeonAssetMigrationState& GetMutableDungeonAssetMigrationState() override;
	virtual const FDungeonAssetMigrationState& GetDungeonAssetMigrationState() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/**
	 * Seed of random number (if 0, auto-generated)
	 *
	 * 乱数のシード（0なら自動生成）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0", ToolTip = "Seed used to begin deterministic dungeon generation. Set to 0 to generate a seed automatically."))
	int32 RandomSeed = 0;

	/**
	 * Seed of random numbers used for the last generation
	 *
	 * 最後の生成に使用された乱数のシード
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator", meta = (ToolTip = "Actual random seed used by the most recent dungeon generation."))
	int32 GeneratedRandomSeed = 0;

	/**
	 * CRC32 of the last dungeon generated
	 *
	 * 最後に生成されたダンジョンのCRC32
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator", meta = (ToolTip = "CRC32 of the last dungeon generated"))
	int32 GeneratedDungeonCRC32 = 0;

	/**
	 * Theme settings that control meshes, interiors, fixtures, and visual selection.
	 * メッシュ、内装、設置物、見た目の選択を制御するテーマ設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ShowOnlyInnerProperties, ToolTip = "Theme settings that control meshes, interiors, fixtures, and visual selection."))
	FDungeonThemeSettings Theme;

	/**
	 * Structure settings that control size, grid, floors, and room spacing.
	 * サイズ、グリッド、階層、部屋間隔を制御する構造設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ShowOnlyInnerProperties, ToolTip = "Structure settings that control size, grid, floors, and room spacing."))
	FDungeonStructureSettings Structure;

	/**
	 * Path settings that control route shape, start and goal rooms, and progression gates.
	 * 経路形状、開始部屋、ゴール部屋、進行ゲートをまとめて制御する設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ShowOnlyInnerProperties, ToolTip = "Path settings that control route shape, start and goal rooms, and progression gates."))
	FDungeonPathSettings Path;

	/**
	 * Zone settings that define biome-like areas.
	 * バイオームのような領域を定義するゾーン設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ShowOnlyInnerProperties, ToolTip = "Zone settings that define biome-like areas."))
	FDungeonZoneSettings Zones;

	/**
	 * Gameplay settings for room roles, sensors, and reserved sublevels.
	 * 部屋役割、センサー、予約サブレベルを制御するゲームプレイ設定です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ShowOnlyInnerProperties, ToolTip = "Gameplay settings for room roles, room visuals, special-room selection, sensors, and reserved sublevels. RoomRoles does not control room shape or room count."))
	FDungeonGameplaySpawnSettings Gameplay;

	/**
	 * Migration flag that prevents re-running legacy fixture selection-policy conversion.
	 * 旧設置物選択ポリシー変換を再実行しないための移行フラグです。
	 */
	UPROPERTY()
	bool bFixtureSelectionPoliciesMigrated = false;

	/**
	 * PluginVersion
	 *
	 * プラグインバージョン
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "PluginVersion"))
	uint8 PluginVersion;

private:

	/**
	 * Runs version-specific migration for the serialized asset format.
	 * 保存形式バージョンごとの移行処理を実行します。
	 */
	void MigrateFromAssetVersion(const int32 assetVersion);

	/**
	 * Applies compatibility fixups that must remain valid for every asset version.
	 * 全てのアセット形式で有効に保つ必要がある互換補正を適用します。
	 */
	void ApplyPostLoadCompatibilityFixups();

	/**
	 * Migrates legacy fixture selection fields to the current policy fields.
	 * 旧設置物選択フィールドを現在のポリシーフィールドへ移行します。
	 */
	void MigrateLegacyFixtureSelectionPolicies();

	/**
	 * Migrates legacy top-level 1.x properties into the current grouped settings.
	 * bForceLegacyDefaults copies default-valued v1 fields while loading pre-v2 assets.
	 * 旧1.xのトップレベルプロパティを現在のグループ化された設定へ移行します。
	 * bForceLegacyDefaultsはv2より前のアセット読み込み時に、v1のデフォルト値もコピーします。
	 */
	void MigrateLegacyTopLevelProperties(bool bForceLegacyDefaults = false);
	void MigrateLegacyRoomSensorSettings();

	/**
	 * 現在の設定へコピーすべき旧トップレベルプロパティのデータがある場合にtrueを返します。
	 */
	bool HasLegacyTopLevelPropertyData() const;

#if WITH_EDITORONLY_DATA
	/**
	 * Editor-only state that reports the latest in-memory migration result for this asset.
	 * このアセットの最新のメモリ上移行結果を報告するEditor専用状態です。
	 */
	UPROPERTY(Transient)
	FDungeonAssetMigrationState MigrationState;
#endif

	/**
	 * Serialized asset format version captured during Serialize for PostLoad migration.
	 * PostLoad移行で使用するためにSerialize中に取得した保存形式バージョンです。
	 */
	int32 LoadedAssetVersion = FDungeonGeneratorAssetVersion::LatestVersion;

	/**
	 * Legacy horizontal grid size serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧水平グリッドサイズです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.HorizontalGridSize instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	float GridSize = 400.f;

	/**
	 * Legacy vertical grid size serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧垂直グリッドサイズです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.VerticalGridSize instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	float VerticalGridSize = 400.f;

	/**
	 * Legacy initial room count serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧初期部屋数です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.RoomCountRange instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 NumberOfCandidateRooms = 10;

	/**
	 * Legacy room width range serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧部屋幅範囲です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.RoomWidth instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	FInt32Interval RoomWidth = { 3, 8 };

	/**
	 * Legacy room depth range serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧部屋奥行き範囲です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.RoomDepth instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	FInt32Interval RoomDepth = { 3, 8 };

	/**
	 * Legacy room height range serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧部屋高さ範囲です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.RoomHeight instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	FInt32Interval RoomHeight = { 2, 4 };

	/**
	 * Legacy horizontal room margin serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧水平部屋間隔です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.HorizontalRoomMargin instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 RoomMargin = 2;

	/**
	 * Legacy vertical room margin serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧垂直部屋間隔です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.VerticalRoomMargin instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 VerticalRoomMargin = 0;

	/**
	 * Legacy room merge toggle serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧部屋結合フラグです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Room merging has been removed. This value is ignored. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	bool MergeRooms = false;

	/**
	 * Legacy expansion policy serialized before settings were grouped into Structure.
	 * Structureへ設定を集約する前に保存されていた旧展開方針です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.FloorMode instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonExpansionPolicy ExpansionPolicy = EDungeonExpansionPolicy::ExpandHorizontally;

	/**
	 * Legacy flat toggle serialized before ExpansionPolicy was introduced.
	 * ExpansionPolicy導入前に保存されていた旧平面生成フラグです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Structure.FloorMode instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	bool Flat = false;

	/**
	 * Legacy floor count serialized by 1.x assets. Current 2.0 layout derives floor count automatically from FloorMode.
	 * 1.xアセットで保存されていた旧階層数です。現在の2.0レイアウトではFloorModeから階層数を自動決定します。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Floor count is now derived automatically from Structure.FloorMode. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 NumberOfCandidateFloors = 3;

	/**
	 * Legacy layout candidate count serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧レイアウト候補数です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.LayoutCandidateCount instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 LayoutCandidateCount = 0;

	/**
	 * Legacy mission graph toggle serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧MissionGraphフラグです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.ProgressionPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	bool UseMissionGraph = false;

	/**
	 * Legacy aisle complexity serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧通路複雑度です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.ExtraCorridorComplexity instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	uint8 AisleComplexity = 5;

	/**
	 * Legacy aisle ceiling policy serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧通路天井高方針です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.CorridorCeilingHeightPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonAisleCeilingHeightPolicy AisleCeilingHeightPolicy = EDungeonAisleCeilingHeightPolicy::Random;

	/**
	 * Legacy player-start movement flag serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧PlayerStart移動フラグです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.bMovePlayerStartToStartRoom instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	bool MovePlayerStartToStartingPoint = true;

	/**
	 * Legacy start-location policy serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧開始部屋選択方針です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.StartRoomPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonStartLocationPolicy StartLocationPolicy = EDungeonStartLocationPolicy::UseSouthernMost;

	/**
	 * Legacy goal-location policy serialized before settings were grouped into Path.
	 * Pathへ設定を集約する前に保存されていた旧ゴール部屋選択方針です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Path.GoalRoomPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonStartLocationPolicy GoalLocationPolicy = EDungeonStartLocationPolicy::UseSouthernMost;

	/**
	 * Legacy room mesh database reference serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧部屋メッシュDB参照です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.DungeonRoomMeshPartsDatabase instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UDungeonMeshSetDatabase> DungeonRoomMeshPartsDatabase;

	/**
	 * Legacy aisle mesh database reference serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧通路メッシュDB参照です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.DungeonAisleMeshPartsDatabase instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UDungeonMeshSetDatabase> DungeonAisleMeshPartsDatabase;

	/**
	 * Legacy pillar selection policy serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧柱選択ポリシーです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.PillarPartsSelectionPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonSelectionPolicy PillarPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy pillar selection method serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧柱選択方式です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.PillarPartsSelectionMethod instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Legacy pillar parts serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧柱パーツです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.PillarParts instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TArray<FDungeonMeshParts> PillarParts;

	/**
	 * Legacy torch selection policy serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧たいまつ選択ポリシーです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.TorchPartsSelectionPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonSelectionPolicy TorchPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy torch selection method serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧たいまつ選択方式です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.TorchPartsSelectionMethod instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Legacy torch generation frequency serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧たいまつ生成頻度です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.RoomTorchFrequency and Theme.Fixtures.AisleTorchFrequency instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonFrequencyOfGeneration FrequencyOfTorchlightGeneration = EDungeonFrequencyOfGeneration::Rarely;

	/**
	 * Legacy torch parts serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧たいまつパーツです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.TorchParts instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TArray<FDungeonRandomActorParts> TorchParts;

	/**
	 * Legacy door selection policy serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧ドア選択ポリシーです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.DoorPartsSelectionPolicy instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonSelectionPolicy DoorPartsSelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy door selection method serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧ドア選択方式です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.DoorPartsSelectionMethod instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	 * Legacy door parts serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧ドアパーツです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.DoorParts instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TArray<FDungeonDoorActorParts> DoorParts;

	/**
	 * Legacy custom parts selector serialized before settings were grouped into Theme.
	 * Themeへ設定を集約する前に保存されていた旧カスタムパーツセレクターです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Theme.Fixtures.DungeonPartsSelector instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UDungeonPartsSelectorBase> DungeonPartsSelector;


	/**
	 * Legacy room sensor class serialized before room sensor databases were introduced.
	 * RoomSensorDatabase導入前に保存されていた旧ルームセンサークラスです。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Gameplay.DungeonRoomSensorClass instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UClass> DungeonRoomSensorClass;

	/**
	 * Legacy room sensor database reference serialized before settings were grouped into Gameplay.
	 * Gameplayへ設定を集約する前に保存されていた旧ルームセンサーDB参照です。
	 */
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "Use Gameplay.DungeonRoomSensorClass and Gameplay.SpawnActorInAisle instead. This legacy v1 migration field may be removed in v2.1 or later; open and save v1 assets in v2.0.0 before upgrading."))
	TObjectPtr<UDungeonRoomSensorDatabase> DungeonRoomSensorDatabase;

protected:
	friend class ADungeonGenerateBase;
	friend class ADungeonGenerateActor;
	friend class FDungeonParameterValidator;
};

inline int32 UDungeonGenerateParameter::GetRandomSeed() const
{
	return RandomSeed;
}

inline int32 UDungeonGenerateParameter::GetGeneratedRandomSeed() const
{
	return GeneratedRandomSeed;
}

inline int32 UDungeonGenerateParameter::GetNumberOfCandidateRooms() const
{
	return Structure.RoomCountRange.Max;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomWidth() const noexcept
{
	return Structure.RoomWidth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomDepth() const noexcept
{
	return Structure.RoomDepth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomHeight() const noexcept
{
	return Structure.RoomHeight;
}

inline int32 UDungeonGenerateParameter::GetHorizontalRoomMargin() const noexcept
{
	return Structure.HorizontalRoomMargin;
}

inline int32 UDungeonGenerateParameter::GetVerticalRoomMargin() const noexcept
{
	return Structure.FloorMode == EDungeonFloorMode::Flat ? 0 : Structure.VerticalRoomMargin;
}

inline FDungeonGridSize UDungeonGenerateParameter::GetGridSize() const
{
	return FDungeonGridSize(Theme.HorizontalGridSize, Theme.VerticalGridSize);
}

inline bool UDungeonGenerateParameter::IsMovePlayerStartToStartingPoint() const noexcept
{
	return Path.bMovePlayerStartToStartRoom;
}

inline bool UDungeonGenerateParameter::IsUseMissionGraph() const noexcept
{
	return Path.ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks;
}

inline int32 UDungeonGenerateParameter::GetLayoutCandidateCount() const noexcept
{
	return Path.LayoutCandidateCount;
}

inline uint8 UDungeonGenerateParameter::GetAisleComplexity() const noexcept
{
	return Path.ExtraCorridorComplexity;
}

inline bool UDungeonGenerateParameter::IsAisleComplexity() const noexcept
{
	return GetAisleComplexity() > 0;
}

inline EDungeonAisleCeilingHeightPolicy UDungeonGenerateParameter::GetAisleCeilingHeightPolicy() const noexcept
{
	return Path.CorridorCeilingHeightPolicy;
}

inline EDungeonFrequencyOfGeneration UDungeonGenerateParameter::GetRoomTorchFrequency() const noexcept
{
	return Theme.Fixtures.RoomTorchFrequency;
}

inline EDungeonFrequencyOfGeneration UDungeonGenerateParameter::GetAisleTorchFrequency() const noexcept
{
	return Theme.Fixtures.AisleTorchFrequency;
}

inline const FDungeonStructureSettings& UDungeonGenerateParameter::GetStructureSettings() const noexcept
{
	return Structure;
}

inline const FDungeonPathSettings& UDungeonGenerateParameter::GetPathSettings() const noexcept
{
	return Path;
}

inline const FDungeonRoomRoleSettings& UDungeonGenerateParameter::GetRoomRoleSettings() const noexcept
{
	return Gameplay.RoomRoles;
}

inline const FDungeonZoneSettings& UDungeonGenerateParameter::GetZoneSettings() const noexcept
{
	return Zones;
}

inline const FDungeonGameplaySpawnSettings& UDungeonGenerateParameter::GetGameplaySpawnSettings() const noexcept
{
	return Gameplay;
}

inline const FDungeonThemeSettings& UDungeonGenerateParameter::GetThemeSettings() const noexcept
{
	return Theme;
}

inline int32 UDungeonGenerateParameter::GetLoadedDungeonAssetVersion() const
{
	return LoadedAssetVersion;
}

#if WITH_EDITORONLY_DATA
inline FDungeonAssetMigrationState& UDungeonGenerateParameter::GetMutableDungeonAssetMigrationState()
{
	return MigrationState;
}

inline const FDungeonAssetMigrationState& UDungeonGenerateParameter::GetDungeonAssetMigrationState() const
{
	return MigrationState;
}
#endif

inline const UDungeonMeshSetDatabase* UDungeonGenerateParameter::GetDungeonRoomMeshPartsDatabase() const noexcept
{
	return Theme.DungeonRoomMeshPartsDatabase;
}

inline const UDungeonMeshSetDatabase* UDungeonGenerateParameter::GetDungeonAisleMeshPartsDatabase() const noexcept
{
	return Theme.DungeonAisleMeshPartsDatabase;
}

inline UClass* UDungeonGenerateParameter::GetRoomSensorClass() const
{
	return Gameplay.DungeonRoomSensorClass;
}

inline UDungeonRoomSensorDatabase* UDungeonGenerateParameter::GetRoomSensorDatabase() const
{
	return nullptr;
}

inline void UDungeonGenerateParameter::EachFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	EachRoomFloorParts(function);
	EachAisleFloorParts(function);
}

inline void UDungeonGenerateParameter::EachWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomWallParts(function);
	EachAisleWallParts(function);
}

inline void UDungeonGenerateParameter::EachRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	EachRoomRoofParts(function);
	EachAisleRoofParts(function);
}

inline void UDungeonGenerateParameter::EachSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomSlopeParts(function);
	EachAisleSlopeParts(function);
}

inline void UDungeonGenerateParameter::EachCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomCatwalkParts(function);
	EachAisleCatwalkParts(function);
}
