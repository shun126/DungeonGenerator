/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
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
	explicit UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonMeshSetDatabase() override = default;
	/*
	 * Serializes this asset and stamps the DungeonGenerator asset format version.
	 * このアセットをシリアライズし、DungeonGeneratorアセット形式バージョンを記録します。
	 */
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual int32 GetLoadedDungeonAssetVersion() const override;
#if WITH_EDITORONLY_DATA
	virtual FDungeonAssetMigrationState& GetMutableDungeonAssetMigrationState() override;
	virtual const FDungeonAssetMigrationState& GetDungeonAssetMigrationState() const override;
#endif
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/**
	 * FDungeonMeshSetを取得します
	 */
	virtual const FDungeonMeshSet* AtImplement(const size_t index) const;

	/**
	 * FDungeonMeshSetをランダムに抽選します
	 */
	virtual const FDungeonMeshSet* SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random, const FDungeonMeshSetQuery& query) const;

	template<typename Function>
	void Each(Function&& function) const
	{
		for (const FDungeonMeshSet& parts : Parts)
		{
			std::forward<Function>(function)(parts);
		}
	}

	void MigrateSelectionPolicies();

#if WITH_EDITOR
public:
	// Debug
	FString DumpToJson(const uint32 indent) const;

#endif

protected:
	/**
	 * Part Selection Method
	 * パーツを選択する方法
	 */
	/*
	 * Selector that chooses one mesh set from this database.
	 * このデータベースから 1 つのメッシュセットを選択するセレクターです。
	 */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "DungeonGenerator", meta = (
		DisplayName = "Mesh Set Selector",
		ToolTip = "Chooses which FDungeonMeshSet candidate to use from this database. Uniform Random is assigned automatically when empty."))
	TObjectPtr<UDungeonMeshSetSelectorBase> MeshSetSelector;

	UPROPERTY()
	EDungeonSelectionPolicy SelectionPolicy = EDungeonSelectionPolicy::Random;

	/**
	 * Legacy serialized selection method. Kept for backward compatibility and migrated to SelectionPolicy on load.
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
	 * Optional selector object used only when mesh-set selection policy is CustomSelector.
	 */
	UPROPERTY()
	TObjectPtr<UDungeonMeshSetSelectorBase> DungeonPartsSelector;

	/**
	 * Set the DungeonRoomMeshSet; multiple DungeonRoomMeshSets can be set.
	 * DungeonRoomMeshSetを設定して下さい。DungeonRoomMeshSetは複数設定する事ができます。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (DisplayName = "Mesh Set"))
	TArray<FDungeonMeshSet> Parts;

private:
	/*
	 * Runs version-specific migration for the serialized asset format.
	 * 保存形式バージョンごとの移行処理を実行します。
	 */
	void MigrateFromAssetVersion(const int32 assetVersion);

	/*
	 * Applies compatibility fixups that must remain valid for every asset version.
	 * 全てのアセット形式で有効に保つ必要がある互換補正を適用します。
	 */
	void ApplyPostLoadCompatibilityFixups();

#if WITH_EDITORONLY_DATA
	/*
	 * Editor-only state that reports the latest in-memory migration result for this asset.
	 * このアセットの最新のメモリ上移行結果を報告するEditor専用状態です。
	 */
	UPROPERTY(Transient)
	FDungeonAssetMigrationState MigrationState;
#endif

	/*
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

