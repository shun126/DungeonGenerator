/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>

class FArchive;

/**
 * Custom version used to identify serialized DungeonGenerator asset data.
 * DungeonGeneratorアセットデータの保存形式を識別するためのカスタムバージョンです。
 */
struct DUNGEONGENERATOR_API FDungeonGeneratorAssetVersion
{
	enum Type
	{
		/*
		 * Asset saved before DungeonGenerator Custom Version was introduced.
		 * DungeonGenerator Custom Version導入前に保存されたアセットです。
		 */
		BeforeCustomVersionWasAdded = 0,

		/*
		 * DungeonGenerator 2.0 asset format.
		 * DungeonGenerator 2.0のアセット形式です。
		 */
		Version2_0 = 1,

		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	/**
	 * GUID registered with Unreal Engine's Custom Version system.
	 * Unreal EngineのCustom Versionシステムに登録するGUIDです。
	 */
	static const FGuid GUID;

	/**
	 * DungeonGeneratorアセットの保存形式バージョンを返します。Custom Versionが無い場合は導入前形式として扱います。
	 */
	static int32 Get(const FArchive& Ar);

private:
	FDungeonGeneratorAssetVersion() = delete;
};
