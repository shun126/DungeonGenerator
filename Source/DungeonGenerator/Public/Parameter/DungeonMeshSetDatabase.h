/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonGeneratorAssetVersion.h"
#include "Migration/DungeonAssetMigration.h"
#include "DungeonMeshSetSelectionMethod.h"
#include "Parameter/DungeonMeshSet.h"
#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "Parameter/DungeonSelectionPolicy.h"
#include "Parameter/DungeonSelectionQuery.h"
#include <memory>
#include "DungeonMeshSetDatabase.generated.h"

class UDungeonAisleMeshSetDatabase;
struct FPropertyChangedEvent;
namespace dungeon
{
	class Random;
}

/**
 * Database of dungeon mesh sets
 * ダンジョンのメッシュセットのデータベース
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonMeshSetDatabase : public UObject, public IDungeonMigratableAsset
{
	GENERATED_BODY()

public:
	/** Constructs an empty mesh-set database and initializes migration state. 空のメッシュセットデータベースを構築し、移行状態を初期化します。 */
	explicit UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer);
	/** Destroys the mesh-set database. メッシュセットデータベースを破棄します。 */
	virtual ~UDungeonMeshSetDatabase() override = default;
	/**
	 * Serializes this asset and stamps the DungeonGenerator asset format version.
	 * このアセットをシリアライズし、DungeonGeneratorアセット形式バージョンを記録します。
	 */
	virtual void Serialize(FArchive& Ar) override;
	/** Migrates legacy selection settings and ensures an active selector exists after loading. 読み込み後に旧選択設定を移行し、有効なセレクターが存在する状態にします。 */
	virtual void PostLoad() override;
	/** Returns the asset format version captured during serialization. シリアライズ時に取得したアセット形式バージョンを返します。 */
	virtual int32 GetLoadedDungeonAssetVersion() const override;
#if WITH_EDITORONLY_DATA
	virtual FDungeonAssetMigrationState& GetMutableDungeonAssetMigrationState() override;
	virtual const FDungeonAssetMigrationState& GetDungeonAssetMigrationState() const override;
#endif
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/**
	 * 指定された0始まりのインデックスにあるメッシュセットを返し、範囲外の場合はnullptrを返します。
	 */
	virtual const FDungeonMeshSet* AtImplement(const size_t index) const;

	/**
	 * Selects Implement.
	 * 有効なセレクターと指定された生成コンテキストを使ってメッシュセットを選択します。
	 */
	virtual const FDungeonMeshSet* SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random, const FDungeonMeshSetQuery& query) const;

	/** Visits every mesh set stored in this database. このデータベースに格納された全メッシュセットを列挙します。 */
	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonMeshSet& parts : Parts)
		{
			std::forward<Function>(function)(parts);
		}
	}

	/** Converts legacy database and mesh-set selection methods into selector objects. 旧データベース・メッシュセット選択方式をセレクターオブジェクトへ変換します。 */
	void MigrateSelectionPolicies();

protected:
	/**
	 * Selector that chooses one mesh set from this database.
	 * このデータベースから 1 つのメッシュセットを選択するセレクターです。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "DungeonGenerator", meta = (
		DisplayName = "Mesh Set Selector",
		ToolTip = "Chooses which FDungeonMeshSet candidate to use from this database. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonMeshSetSelectorBase> MeshSetSelector;

	/** Internal migration value used while converting legacy database selection methods. 旧データベース選択方式を変換するときに使う内部移行値です。 */
	UPROPERTY()
	EDungeonSelectionPolicy SelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy serialized selection method retained for v1 asset migration and converted into MeshSetSelector on load.
	 * v1アセット移行用に保持され、読み込み時にMeshSetSelectorへ変換される旧選択方式です。
	 */
	UPROPERTY()
	EDungeonMeshSetSelectionMethod SelectionMethod = EDungeonMeshSetSelectionMethod::Random;

	/**
	 * Migration flag indicating old database-level selection policy has been converted.
	 *
	 * 旧データベース選択ポリシーが移行済みであることを示すフラグです。
	 */
	UPROPERTY()
	bool bSelectionPolicyMigrated = false;

	/**
	 * Legacy custom selector retained only as an input to v1 asset migration.
	 * v1アセット移行の入力としてのみ保持される旧カスタムセレクターです。
	 */
	UPROPERTY()
	TObjectPtr<UDungeonMeshSetSelectorBase> DungeonPartsSelector;

	/**
	 * Set the DungeonRoomMeshSet; multiple DungeonRoomMeshSets can be set.
	 * DungeonRoomMeshSetを設定して下さい。DungeonRoomMeshSetは複数設定する事ができます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (DisplayName = "Mesh Set", ToolTip = "Mesh-set candidates available to Mesh Set Selector. Candidate order matters for index-based selectors."))
	TArray<FDungeonMeshSet> Parts;

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

inline int32 UDungeonMeshSetDatabase::GetLoadedDungeonAssetVersion() const
{
	return LoadedAssetVersion;
}

#if WITH_EDITORONLY_DATA
inline FDungeonAssetMigrationState& UDungeonMeshSetDatabase::GetMutableDungeonAssetMigrationState()
{
	return MigrationState;
}

inline const FDungeonAssetMigrationState& UDungeonMeshSetDatabase::GetDungeonAssetMigrationState() const
{
	return MigrationState;
}
#endif
