/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "DungeonGeneratorAssetVersion.h"
#include <Serialization/Archive.h>
#include <Serialization/CustomVersion.h>

const FGuid FDungeonGeneratorAssetVersion::GUID(0x1E4B48C5, 0xA5C64B95, 0x8E6B278E, 0xD9C3F1A2);

FCustomVersionRegistration GRegisterDungeonGeneratorAssetVersion(FDungeonGeneratorAssetVersion::GUID, FDungeonGeneratorAssetVersion::LatestVersion, TEXT("DungeonGeneratorAssetVer"));

/*
 * Reads the DungeonGenerator custom version from the archive and maps missing versions to the legacy format.
 * アーカイブからDungeonGeneratorのCustom Versionを読み取り、未設定の場合は旧形式へ対応付けます。
 */
int32 FDungeonGeneratorAssetVersion::Get(const FArchive& Ar)
{
	const auto version = Ar.CustomVer(GUID);
	return version == -1 ? BeforeCustomVersionWasAdded : version;
}
