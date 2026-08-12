/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Migration/DungeonAssetMigration.h"

FDungeonAssetMigrationDelegates::FOnDungeonAssetPostLoad FDungeonAssetMigrationDelegates::OnAssetPostLoad;

/*
 * Dispatches loaded assets to the editor migration service without forcing the runtime module to link editor code.
 * RuntimeモジュールがEditorコードへリンクせずに、ロード済みアセットをEditor移行サービスへ通知します。
 */
void FDungeonAssetMigrationDelegates::RequestMigration(UObject* Asset, const int32 LoadedAssetVersion)
{
	if (OnAssetPostLoad.IsBound())
	{
		OnAssetPostLoad.Execute(Asset, LoadedAssetVersion);
	}
}
