/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Migration/DungeonAssetMigration.h"
#include <CoreMinimal.h>
#include <UObject/ObjectSaveContext.h>
#include <UObject/UObjectGlobals.h>

class FOutputDevice;
class UPackage;

/**
 * Editor service that migrates legacy DungeonGenerator assets in memory and validates saving.
 * 旧DungeonGeneratorアセットをメモリ上で移行し、保存を検証するEditorサービスです。
 */
class FDungeonAssetMigrationService final
{
public:
	FDungeonAssetMigrationService() = default;
	~FDungeonAssetMigrationService() = default;

	/**
	 * Registers migration, message log, and save validation hooks.
	 * 移行、Message Log、保存検証のフックを登録します。
	 */
	void Startup();

	/**
	 * Unregisters hooks owned by this service.
	 * このサービスが所有するフックを解除します。
	 */
	void Shutdown();

private:
	/**
	 * Handles a DungeonGenerator asset after Runtime-side PostLoad compatibility fixups.
	 * Runtime側のPostLoad互換補正後にDungeonGeneratorアセットを処理します。
	 */
	void OnAssetPostLoad(UObject* Asset, int32 LoadedAssetVersion);

	/**
	 * Validates packages before Unreal writes them to disk.
	 * Unrealがディスクへ書き込む前にパッケージを検証します。
	 */
	bool IsPackageOKToSave(UPackage* Package, const FString& Filename, FOutputDevice* Error) const;

	/**
	 * Clears transient migration state after a successful save.
	 * 保存成功後に一時的な移行状態をクリアします。
	 */
	void OnPackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext);

	/**
	 * Writes migration messages to the DungeonGenerator migration message log.
	 * DungeonGeneratorの移行Message Logへ移行メッセージを書き込みます。
	 */
	static void ReportMigrationMessages(const UObject* Asset, const FDungeonAssetMigrationState& State);

	/**
	 * パッケージに含まれる移行可能なDungeonGeneratorアセットを返します。
	 */
	static void GetMigratableAssetsInPackage(const UPackage* Package, TArray<UObject*>& OutAssets);

	/**
	 * Adds Message.
	 * 移行状態へ移行メッセージを追加します。
	 */
	static void AddMessage(FDungeonAssetMigrationState& State, EDungeonMigrationMessageSeverity Severity, FName Code, const FText& Message, const FText& FixHint, FName PropertyPath = NAME_None, const FSoftObjectPath& RelatedAsset = FSoftObjectPath());

	/**
	 * 状態にErrorが1件以上含まれる場合にtrueを返します。
	 */
	static bool HasErrors(const FDungeonAssetMigrationState& State);

	/**
	 * 状態にWarningが1件以上含まれる場合にtrueを返します。
	 */
	static bool HasWarnings(const FDungeonAssetMigrationState& State);

	/**
	 * Builds the default state for an asset that does not need migration.
	 * 移行不要なアセットの既定状態を作成します。
	 */
	static FDungeonAssetMigrationState MakeNotRequiredState(int32 LoadedAssetVersion);

private:
	FCoreUObjectDelegates::FIsPackageOKToSaveDelegate PreviousIsPackageOKToSaveDelegate;
	FDelegateHandle PackageSavedDelegateHandle;
	bool bStarted = false;
};
