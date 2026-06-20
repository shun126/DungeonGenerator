/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonRoomSensorDatabase.h"
#include "Core/Math/Random.h"
#include "Core/Debug/Debug.h"
#include "Helper/DungeonAisleGridMap.h"
#include "Helper/DungeonRandom.h"
#include "Parameter/DungeonMeshSetDatabase.h"
#include <Serialization/Archive.h>
#include <cmath>
#include <unordered_set>

UDungeonRoomSensorDatabase::UDungeonRoomSensorDatabase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDungeonRoomSensorDatabase::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FDungeonGeneratorAssetVersion::GUID);
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		LoadedAssetVersion = FDungeonGeneratorAssetVersion::Get(Ar);
	}
}

void UDungeonRoomSensorDatabase::PostLoad()
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
void UDungeonRoomSensorDatabase::MigrateFromAssetVersion(const int32 assetVersion)
{
	(void)assetVersion;
}

/*
 * Applies compatibility fixups that are still required after version-specific migration.
 * バージョン別移行後も必要な互換補正を適用します。
 */
void UDungeonRoomSensorDatabase::ApplyPostLoadCompatibilityFixups()
{
}

UClass* UDungeonRoomSensorDatabase::Select(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const
{
	TArray<UClass*> roomSensors;
	roomSensors.Reserve(DungeonRoomSensorClass.Num());
	for (const auto& dungeonRoomSensorClass : DungeonRoomSensorClass)
	{
		if (IsValid(dungeonRoomSensorClass))
			roomSensors.Add(dungeonRoomSensorClass);
	}
	if (roomSensors.Num() <= 0)
		return nullptr;

	// 抽選
	switch (SelectionMethod)
	{
	case EDungeonMeshSetSelectionMethod::Random:
	{
		const auto index = random->Get(roomSensors.Num());
		return roomSensors[index];
	}
	case EDungeonMeshSetSelectionMethod::Identifier:
	{
		const auto index = identifier % roomSensors.Num();
		return roomSensors[index];
	}
	case EDungeonMeshSetSelectionMethod::DepthFromStart:
	{
		const float ratio = static_cast<float>(depthRatioFromStart) / 255.f;
		const float index = static_cast<float>(roomSensors.Num() - 1) * ratio;
		return roomSensors[static_cast<size_t>(std::round(index))];
	}
	default:
		DUNGEON_GENERATOR_ERROR(TEXT("Set the correct SelectionMethod"));
		return nullptr;
	}
}

UClass* UDungeonRoomSensorDatabase::GetFirstValidRoomSensorClass() const
{
	for (const auto& dungeonRoomSensorClass : DungeonRoomSensorClass)
	{
		if (IsValid(dungeonRoomSensorClass))
		{
			return dungeonRoomSensorClass;
		}
	}
	return nullptr;
}

const TArray<FSoftObjectPath>& UDungeonRoomSensorDatabase::GetSpawnActorInAisle() const
{
	return SpawnActorInAisle;
}

void UDungeonRoomSensorDatabase::OnEndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap, const float verticalGridSize, const std::function<void(const FSoftObjectPath&, const FTransform&)>& spawnActor) const
{
	if (synchronizedRandom == nullptr)
		return;
	if (aisleGridMap == nullptr)
		return;
	if (SpawnActorInAisle.IsEmpty())
		return;
	aisleGridMap->Each([this, synchronizedRandom, verticalGridSize, spawnActor](const TArray<FDungeonAisleGrid>& aisleGridArray)
		{
			// スポーン先グリッドを抽選します
			std::unordered_set<int32> gridIndexes;
			const int32 totalGrids = aisleGridArray.Num();
			const int32 count = totalGrids / 3;
			for (int32 i = 0; i < count; ++i)
			{
				gridIndexes.emplace(synchronizedRandom->GetIntegerFrom(totalGrids));
			}

			const int32 totalActors = SpawnActorInAisle.Num();
			for (const auto gridIndex : gridIndexes)
			{
				// スポーンする姿勢を求める
				const auto& aisleGrid = aisleGridArray[gridIndex];
				const FTransform transform(
					dungeon::detail::ToRotator(aisleGrid.Direction),
					aisleGrid.Location + FVector(0, 0, verticalGridSize / 2.f)
				);

				// アクターをスポーンします
				const int32 actorType = synchronizedRandom->GetIntegerFrom(totalActors);
				spawnActor(SpawnActorInAisle[actorType], transform);
			}
		}
	);
}
