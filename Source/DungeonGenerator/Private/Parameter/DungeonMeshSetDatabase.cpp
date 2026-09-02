/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonMeshSetDatabase.h"
#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "Parameter/DungeonSelectionPolicyUtility.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include <Serialization/Archive.h>
#if WITH_EDITOR
#include <UObject/UnrealType.h>
#endif
#include <cmath>

UDungeonMeshSetDatabase::UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

void UDungeonMeshSetDatabase::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FDungeonGeneratorAssetVersion::GUID);
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		LoadedAssetVersion = FDungeonGeneratorAssetVersion::Get(Ar);
	}
}

void UDungeonMeshSetDatabase::PostLoad()
{
	Super::PostLoad();
	MigrateFromAssetVersion(LoadedAssetVersion);
	ApplyPostLoadCompatibilityFixups();
#if WITH_EDITOR
	FDungeonAssetMigrationDelegates::RequestMigration(this, LoadedAssetVersion);
#endif
}

/*
 * Runs migration steps that depend on the asset format version saved in the uasset.
 * uassetに保存されたアセット形式バージョンに依存する移行処理を実行します。
 */
void UDungeonMeshSetDatabase::MigrateFromAssetVersion(const int32 assetVersion)
{
	if (assetVersion < FDungeonGeneratorAssetVersion::Version2_0 || !bSelectionPolicyMigrated)
	{
		MigrateSelectionPolicies();
	}
	if (assetVersion < FDungeonGeneratorAssetVersion::Version2_0)
	{
		for (FDungeonMeshSet& meshSet : Parts)
			meshSet.MigrateSpawnChancesFromVersion1();
	}
}

/*
 * Applies compatibility fixups that are still required after version-specific migration.
 * バージョン別移行後も必要な互換補正を適用します。
 */
void UDungeonMeshSetDatabase::ApplyPostLoadCompatibilityFixups()
{
	MigrateSelectionPolicies();
}

#if WITH_EDITOR
void UDungeonMeshSetDatabase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bSelectionPolicyMigrated = true;
	for (FDungeonMeshSet& meshSet : Parts)
	{
		meshSet.MarkSelectionPoliciesMigrated();
	}
	MigrateSelectionPolicies();
}
#endif

void UDungeonMeshSetDatabase::MigrateSelectionPolicies()
{
	if (!IsValid(MeshSetSelector))
	{
		const EDungeonMeshSetSelectionMethod legacyMethod = SelectionMethod != EDungeonMeshSetSelectionMethod::Random ? SelectionMethod : dungeon::selection::ToLegacyMeshSetMethod(SelectionPolicy);
		MeshSetSelector = UDungeonMeshSetSelectorBase::CreateFromLegacyMethod(this, legacyMethod, DungeonPartsSelector);
	}

	SelectionPolicy = dungeon::selection::SanitizeMeshSetPolicy(SelectionPolicy);
	SelectionMethod = dungeon::selection::ToLegacyMeshSetMethod(SelectionPolicy);
	bSelectionPolicyMigrated = true;

	for (FDungeonMeshSet& meshSet : Parts)
	{
		meshSet.MigrateSelectionPolicies(this);
	}
}

const FDungeonMeshSet* UDungeonMeshSetDatabase::AtImplement(const size_t index) const
{
	const int32 size = Parts.Num();
	return (size > 0) ? &Parts[index % size] : nullptr;
}

const FDungeonMeshSet* UDungeonMeshSetDatabase::SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random, const FDungeonMeshSetQuery& query) const
{
	(void)identifier;
	(void)depthRatioFromStart;

	const int32 size = Parts.Num();
	if (size <= 0)
		return nullptr;

	if (IsValid(MeshSetSelector))
	{
		const int32 index = MeshSetSelector->SelectMeshSetIndexNative(query, random, size);
		if (0 <= index && index < size)
			return &Parts[index];
	}

	if (random != nullptr)
		return &Parts[random->Get<uint32_t>(size)];
	return &Parts[0];
}
