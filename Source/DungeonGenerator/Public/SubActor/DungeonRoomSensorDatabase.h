/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonGeneratorAssetVersion.h"
#include "Migration/DungeonAssetMigration.h"
#include "Parameter/DungeonMeshSetSelectionMethod.h"
#include <functional>
#include <memory>
#include "DungeonRoomSensorDatabase.generated.h"

class UDungeonAisleGridMap;
class UDungeonRandom;

namespace dungeon
{
	class Random;
}

/**
 * Database class that manages dungeon room intrusion detection sensors
 *
 * Deprecated v1 room sensor database kept only for v2.0.0 migration.
 * Open and save v1 assets in v2.0.0 before upgrading to v2.1 or later.
 * New projects should set UDungeonGenerateParameter.Gameplay.DungeonRoomSensorClass and
 * UDungeonGenerateParameter.Gameplay.SpawnActorInAisle directly.
 *
 * ダンジョンの部屋侵入検知センサーを管理するデータベースクラス
 *
 * v2.0.0 の移行専用に残されている v1 ルームセンサーデータベースです。
 * v2.1 以降へ更新する前に、v1 アセットを v2.0.0 で開いて保存してください。
 * 新規プロジェクトでは UDungeonGenerateParameter.Gameplay.DungeonRoomSensorClass と
 * UDungeonGenerateParameter.Gameplay.SpawnActorInAisle を直接設定してください。
 */
UCLASS(ClassGroup = "DungeonGenerator", meta = (
	DisplayName = "Dungeon Room Sensor Database (Deprecated)",
	ToolTip = "Deprecated v1 migration asset. Open and save v1 assets in v2.0.0 before upgrading to v2.1 or later. Use UDungeonGenerateParameter.Gameplay.DungeonRoomSensorClass and Gameplay.SpawnActorInAisle for new setup.",
	DeprecationMessage = "UDungeonRoomSensorDatabase is kept only for v1-to-v2 migration in v2.0.0 and may be removed in v2.1 or later."))
class DUNGEONGENERATOR_API UDungeonRoomSensorDatabase : public UObject, public IDungeonMigratableAsset
{
	GENERATED_BODY()

public:
	/**
	 * constructor
	 * UDungeonRoomSensorDatabase を表します。
	 */
	explicit UDungeonRoomSensorDatabase(const FObjectInitializer& ObjectInitializer);

	/**
	 * destructor
	 * ~U Du ng eo nR oo mS en so rD at ab as e インスタンスを破棄します。
	 */
	virtual ~UDungeonRoomSensorDatabase() override = default;

	/**
	 * Serializes this asset and stamps the DungeonGenerator asset format version.
	 * このアセットをシリアライズし、DungeonGeneratorアセット形式バージョンを記録します。
	 */
	virtual void Serialize(FArchive& Ar) override;

	/**
	 * Applies version-based compatibility handling after this asset is loaded.
	 * このアセットのロード後にバージョンに基づく互換処理を適用します。
	 */
	virtual void PostLoad() override;
	virtual int32 GetLoadedDungeonAssetVersion() const override;
#if WITH_EDITORONLY_DATA
	virtual FDungeonAssetMigrationState& GetMutableDungeonAssetMigrationState() override;
	virtual const FDungeonAssetMigrationState& GetDungeonAssetMigrationState() const override;
#endif

	/**
	 * Select the DungeonRoomSensor class that matches the condition of the argument
	 *
	 * 引数の条件にあったDungeonRoomSensorのクラスを選択します
	 */
	UClass* Select(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const;

	/**
	 * Returns SpawnActorInAisle.
	 * SpawnActorInAisle を返します。
	 */
	UClass* GetFirstValidRoomSensorClass() const;

	/**
	 * UDungeonGenerateParameter へ移行するため、旧通路 Actor 設定を返します。
	 */
	const TArray<FSoftObjectPath>& GetSpawnActorInAisle() const;

	/**
	 * ダンジョン生成終了時に通知されるイベント
	 * @param synchronizedRandom クライアント・サーバー間で同期される乱数
	 * @param aisleGridMap 通路のグリッドを記録したコンテナ
	 * @param verticalGridSize グリッドの垂直方向の大きさ
	 * @param spawnActor アクターをスポーンする関数
	 */
	void OnEndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap, const float verticalGridSize, const std::function<void(const FSoftObjectPath&, const FTransform&)>& spawnActor) const;

protected:
	/**
	 * Room Sensor Generation Rules
	 * ルームセンサーの生成ルール
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomSensor", meta = (ToolTip = "Room Sensor Generation Rules"))
	EDungeonMeshSetSelectionMethod SelectionMethod = EDungeonMeshSetSelectionMethod::DepthFromStart;

	/**
	 * Register the RoomSensor to be placed.
	 *
	 * 配置するRoomSensorを登録して下さい。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomSensor", meta = (ToolTip = "Register the RoomSensor to be placed.", AllowedClasses = "/Script/DungeonGenerator.DungeonRoomSensorBase"))
	TArray<TObjectPtr<UClass>> DungeonRoomSensorClass;

	/**
	 * Actor spawning in an aisle
	 *
	 * 通路にスポーンするアクター
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Aisle", meta = (ToolTip = "Actor spawning in an aisle", AllowedClasses = "/Script/Engine.Blueprint"))
	TArray<FSoftObjectPath> SpawnActorInAisle;

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
};

inline int32 UDungeonRoomSensorDatabase::GetLoadedDungeonAssetVersion() const
{
	return LoadedAssetVersion;
}

#if WITH_EDITORONLY_DATA
inline FDungeonAssetMigrationState& UDungeonRoomSensorDatabase::GetMutableDungeonAssetMigrationState()
{
	return MigrationState;
}

inline const FDungeonAssetMigrationState& UDungeonRoomSensorDatabase::GetDungeonAssetMigrationState() const
{
	return MigrationState;
}
#endif
