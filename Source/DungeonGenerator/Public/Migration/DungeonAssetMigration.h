/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonGeneratorAssetVersion.h"
#include <CoreMinimal.h>
#include <UObject/Interface.h>
#include <UObject/SoftObjectPath.h>
#include "DungeonAssetMigration.generated.h"

/**
 * Result state for an in-memory DungeonGenerator asset migration.
 * メモリ上で実行したDungeonGeneratorアセット移行の結果状態です。
 */
UENUM(BlueprintType)
enum class EDungeonAssetMigrationStatus : uint8
{
	NotRequired UMETA(DisplayName = "Not Required", ToolTip = "The asset is already in the latest DungeonGenerator format."),
	Succeeded UMETA(DisplayName = "Succeeded", ToolTip = "The asset was migrated in memory and should be reviewed before saving."),
	SucceededWithWarnings UMETA(DisplayName = "Succeeded With Warnings", ToolTip = "The asset was migrated in memory, but some items need review."),
	Failed UMETA(DisplayName = "Failed", ToolTip = "The asset could not be migrated safely and should not be saved until fixed."),
	RequiresSaveConfirmation UMETA(DisplayName = "Requires Save Confirmation", ToolTip = "The asset can be saved only after the user reviews migration messages."),
};

/**
 * Severity for a single migration message.
 * 個別の移行メッセージの重要度です。
 */
UENUM(BlueprintType)
enum class EDungeonMigrationMessageSeverity : uint8
{
	Info UMETA(DisplayName = "Info", ToolTip = "Informational migration message."),
	Warning UMETA(DisplayName = "Warning", ToolTip = "Migration warning that should be reviewed before saving."),
	Error UMETA(DisplayName = "Error", ToolTip = "Migration error that blocks normal saving."),
};

/**
 * Human-readable migration message shown to users and developers.
 * 利用者と開発者に表示する、人が読める移行メッセージです。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMigrationMessage
{
	GENERATED_BODY()

	/**
	 * Severity of this migration message.
	 * この移行メッセージの重要度です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Severity of this migration message: informational, warning, or error."))
	EDungeonMigrationMessageSeverity Severity = EDungeonMigrationMessageSeverity::Info;

	/**
	 * Stable identifier for grouping migration messages.
	 * 移行メッセージを分類するための安定した識別子です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Stable machine-readable code identifying this migration issue."))
	FName Code;

	/**
	 * Human-readable description of the migration result.
	 * 移行結果を説明する人が読める文章です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Human-readable description of the migration result or issue."))
	FText Message;

	/**
	 * Suggested manual action for the user.
	 * 利用者が手動で確認または修正するための案内です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Suggested action for resolving this migration issue."))
	FText FixHint;

	/**
	 * Related property path, when a specific setting caused this message.
	 * 特定の設定が原因の場合に使用する関連プロパティパスです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Property path associated with this migration issue, when available."))
	FName PropertyPath;

	/**
	 * Related asset reference, when the migration message concerns another asset.
	 * 他のアセットに関係する移行メッセージで使用する関連アセット参照です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Related asset associated with this migration issue, when available."))
	FSoftObjectPath RelatedAsset;
};

/**
 * Editor-only migration state retained by each migrated DungeonGenerator asset.
 * 移行された各DungeonGeneratorアセットが保持するEditor専用の移行状態です。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonAssetMigrationState
{
	GENERATED_BODY()

	/**
	 * Source asset format version read from the uasset.
	 * uassetから読み取った移行元の保存形式バージョンです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Serialized DungeonGenerator asset version detected before migration."))
	int32 SourceVersion = FDungeonGeneratorAssetVersion::LatestVersion;

	/**
	 * Target asset format version for the current plugin.
	 * 現在のプラグインが移行先とする保存形式バージョンです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "DungeonGenerator asset version targeted by this migration."))
	int32 TargetVersion = FDungeonGeneratorAssetVersion::LatestVersion;

	/**
	 * Overall migration status for this asset.
	 * このアセット全体の移行状態です。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Overall migration status for the loaded asset."))
	EDungeonAssetMigrationStatus Status = EDungeonAssetMigrationStatus::NotRequired;

	/**
	 * Messages produced while migrating this asset.
	 * このアセットの移行中に生成されたメッセージです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Messages produced while validating or migrating the asset."))
	TArray<FDungeonMigrationMessage> Messages;

	/**
	 * True when this asset was converted from an older saved format in memory.
	 * このアセットが古い保存形式からメモリ上で変換された場合にtrueになります。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Whether serialized values were migrated in memory during loading."))
	bool bWasMigrated = false;

	/**
	 * True when the asset should be saved manually to write the latest format.
	 * 最新形式を書き込むために利用者による手動保存が必要な場合にtrueになります。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Migration", meta = (ToolTip = "Whether the asset must be saved in v2.0.0 to persist migrated values before upgrading further."))
	bool bRequiresResave = false;
};

/**
 * C++ interface for DungeonGenerator assets that expose editor-only migration state.
 * Editor専用の移行状態を公開するDungeonGeneratorアセット用C++インターフェースです。
 */
UINTERFACE(MinimalAPI)
class UDungeonMigratableAsset : public UInterface
{
	GENERATED_BODY()
};

class DUNGEONGENERATOR_API IDungeonMigratableAsset
{
	GENERATED_BODY()

public:
	/**
	 * Returns LoadedDungeonAssetVersion.
	 * ロード中に取得した保存形式バージョンを返します。
	 */
	virtual int32 GetLoadedDungeonAssetVersion() const = 0;

#if WITH_EDITORONLY_DATA
	/**
	 * このアセットの変更可能なEditor専用移行状態を返します。
	 */
	virtual FDungeonAssetMigrationState& GetMutableDungeonAssetMigrationState() = 0;

	/**
	 * このアセットのEditor専用移行状態を返します。
	 */
	virtual const FDungeonAssetMigrationState& GetDungeonAssetMigrationState() const = 0;
#endif
};

/**
 * Runtime-owned dispatcher that lets the editor module run migration without a runtime dependency on editor code.
 * Runtime側が所有し、Editorコードへ依存せずにEditorモジュールへ移行処理を委譲するdispatcherです。
 */
class DUNGEONGENERATOR_API FDungeonAssetMigrationDelegates
{
public:
	DECLARE_DELEGATE_TwoParams(FOnDungeonAssetPostLoad, UObject*, int32);

	/**
	 * Delegate bound by the editor module to migrate loaded assets.
	 * ロード済みアセットを移行するためにEditorモジュールがバインドするデリゲートです。
	 */
	static FOnDungeonAssetPostLoad OnAssetPostLoad;

	/**
	 * Requests editor-side migration for a loaded asset if the editor module is available.
	 * Editorモジュールが利用可能な場合、ロード済みアセットのEditor側移行を要求します。
	 */
	static void RequestMigration(UObject* Asset, int32 LoadedAssetVersion);
};
