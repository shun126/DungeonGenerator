/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonGenerateParameter.h"
#include "Parameter/Selector/DungeonPartsSelectorBase.h"
#include "Parameter/DungeonSelectionPolicyUtility.h"
#include "PluginInformation.h"
#include "Core/Debug/BuildInformation.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"
#include "Helper/DungeonAisleGridMap.h"
#include "Helper/DungeonRandom.h"
#include "SubActor/DungeonRoomSensorDatabase.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>
#include <Serialization/Archive.h>
#include <UObject/Package.h>
#include <unordered_set>
#if WITH_EDITOR
#include <UObject/UnrealType.h>
#endif

namespace
{
	/*
	 * Converts the legacy shared endpoint policy to the current start-room policy.
	 * 旧共通エンドポイント方針を現在の開始部屋方針へ変換します。
	 */
	EDungeonStartLocationPolicy ToStartRoomPolicy(const EDungeonStartLocationPolicy policy) noexcept
	{
		switch (policy)
		{
		case EDungeonStartLocationPolicy::UseNorthernMost:
			return EDungeonStartLocationPolicy::UseNorthernMost;
		case EDungeonStartLocationPolicy::UseEasternMost:
			return EDungeonStartLocationPolicy::UseEasternMost;
		case EDungeonStartLocationPolicy::UseWesternMost:
			return EDungeonStartLocationPolicy::UseWesternMost;
		case EDungeonStartLocationPolicy::UseHighestPoint:
			return EDungeonStartLocationPolicy::UseHighestPoint;
		case EDungeonStartLocationPolicy::UseLowestPoint:
			return EDungeonStartLocationPolicy::UseLowestPoint;
		case EDungeonStartLocationPolicy::UseCentralPoint:
			return EDungeonStartLocationPolicy::UseCentralPoint;
		case EDungeonStartLocationPolicy::UseMultiStart:
			return EDungeonStartLocationPolicy::UseMultiStart;
		case EDungeonStartLocationPolicy::UseSouthernMost:
		default:
			return EDungeonStartLocationPolicy::UseSouthernMost;
		}
	}

	/*
	 * Converts the legacy shared endpoint policy to the current goal-room policy.
	 * 旧共通エンドポイント方針を現在のゴール部屋方針へ変換します。
	 */
	EDungeonGoalLocationPolicy ToGoalRoomPolicy(const EDungeonStartLocationPolicy policy) noexcept
	{
		switch (policy)
		{
		case EDungeonStartLocationPolicy::UseNorthernMost:
			return EDungeonGoalLocationPolicy::UseNorthernMost;
		case EDungeonStartLocationPolicy::UseEasternMost:
			return EDungeonGoalLocationPolicy::UseEasternMost;
		case EDungeonStartLocationPolicy::UseWesternMost:
			return EDungeonGoalLocationPolicy::UseWesternMost;
		case EDungeonStartLocationPolicy::UseHighestPoint:
			return EDungeonGoalLocationPolicy::UseHighestPoint;
		case EDungeonStartLocationPolicy::UseLowestPoint:
			return EDungeonGoalLocationPolicy::UseLowestPoint;
		case EDungeonStartLocationPolicy::UseCentralPoint:
			return EDungeonGoalLocationPolicy::UseCentralPoint;
		case EDungeonStartLocationPolicy::UseSouthernMost:
		case EDungeonStartLocationPolicy::UseMultiStart:
		default:
			return EDungeonGoalLocationPolicy::UseSouthernMost;
		}
	}
}

namespace
{
	namespace generateParameter
	{
		FDungeonPartsQuery MakePartsSelectionQuery(const EDungeonPartsSelectorTarget target, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const uint8 neighborMask6)
		{
			FDungeonPartsQuery query;
			query.Target = target;
			query.PieceType = static_cast<uint8>(grid.GetType());
			query.Rotation = static_cast<uint8>(grid.GetDirection().Get());
			query.NeighborMask6 = neighborMask6;
			query.GridX = gridLocation.X;
			query.GridY = gridLocation.Y;
			query.GridZ = gridLocation.Z;
			query.RoomId = static_cast<int32>(grid.GetIdentifier());
			query.RoomStructuralRole = grid.GetRoomStructuralRole();
			query.RoomGameplayRole = grid.GetRoomGameplayRole();
			query.ZoneIndex = grid.GetZoneIndex();
			query.DepthFromStart = static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f;
			query.DistanceToGoal = 1.f - query.DepthFromStart;
			query.SeedKey = static_cast<int32>(gridIndex);
			return query;
		}

		int32 SelectCustomPartsIndex(const UDungeonPartsSelectorBase* partsSelector, const EDungeonPartsSelectorTarget target, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const uint8 neighborMask6, const std::shared_ptr<dungeon::Random>& random, const int32 numCandidates)
		{
			if (!IsValid(partsSelector) || numCandidates <= 0)
				return INDEX_NONE;

			const int32 customIndex = partsSelector->SelectPartsIndexNative(MakePartsSelectionQuery(target, gridLocation, gridIndex, grid, neighborMask6), random, numCandidates);
			if (0 <= customIndex && customIndex < numCandidates)
				return customIndex;

			static int32 InvalidSelectionWarningCount = 0;
			if (InvalidSelectionWarningCount < 8)
			{
				++InvalidSelectionWarningCount;
				DUNGEON_GENERATOR_WARNING(TEXT("Custom Parts selector returned invalid index %d (NumCandidates=%d). Falling back to built-in selection. (%d/8)"), customIndex, numCandidates, InvalidSelectionWarningCount);
			}

			return INDEX_NONE;
		}

		template<typename T>
		const T* GetPartsAt(const TArray<T>& parts, const int32 index)
		{
			if (0 <= index && index < parts.Num())
				return &parts[index];
			return nullptr;
		}

		EDungeonPartsSelectionMethod ResolveLegacyMethod(const EDungeonSelectionPolicy policy, const EDungeonPartsSelectionMethod method)
		{
			if (method != EDungeonPartsSelectionMethod::Random)
				return method;

			return dungeon::selection::ToLegacyPartsMethod(policy);
		}

		void EnsurePartsSelector(UObject* outer, TObjectPtr<UDungeonPartsSelectorBase>& selector, const EDungeonSelectionPolicy policy, const EDungeonPartsSelectionMethod method, UDungeonPartsSelectorBase* customSelector)
		{
			if (IsValid(selector))
				return;

			selector = UDungeonPartsSelectorBase::CreateFromLegacyMethod(outer, ResolveLegacyMethod(policy, method), customSelector);
		}

		void SanitizeFixtureSettings(UObject* outer, FDungeonFixtureSettings& fixtures)
		{
			UObject* selectorOuter = outer != nullptr ? outer : static_cast<UObject*>(GetTransientPackage());
			EnsurePartsSelector(selectorOuter, fixtures.PillarPartsSelector, fixtures.PillarPartsSelectionPolicy, fixtures.PillarPartsSelectionMethod, fixtures.DungeonPartsSelector);
			EnsurePartsSelector(selectorOuter, fixtures.TorchPartsSelector, fixtures.TorchPartsSelectionPolicy, fixtures.TorchPartsSelectionMethod, fixtures.DungeonPartsSelector);
			EnsurePartsSelector(selectorOuter, fixtures.DoorPartsSelector, fixtures.DoorPartsSelectionPolicy, fixtures.DoorPartsSelectionMethod, fixtures.DungeonPartsSelector);
			EnsurePartsSelector(selectorOuter, fixtures.UniqueDoorPartsSelector, fixtures.UniqueDoorPartsSelectionPolicy, fixtures.UniqueDoorPartsSelectionMethod, fixtures.DungeonPartsSelector);

			fixtures.PillarPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(fixtures.PillarPartsSelectionPolicy);
			fixtures.PillarPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(fixtures.PillarPartsSelectionPolicy);

			fixtures.TorchPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(fixtures.TorchPartsSelectionPolicy);
			fixtures.TorchPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(fixtures.TorchPartsSelectionPolicy);

			fixtures.DoorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(fixtures.DoorPartsSelectionPolicy);
			fixtures.DoorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(fixtures.DoorPartsSelectionPolicy);

			fixtures.UniqueDoorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(fixtures.UniqueDoorPartsSelectionPolicy);
			fixtures.UniqueDoorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(fixtures.UniqueDoorPartsSelectionPolicy);
		}

		const FDungeonDoorActorParts* SelectFixtureDoorParts(const FDungeonFixtureSettings& fixtures, const TArray<FDungeonDoorActorParts>& parts, const UDungeonPartsSelectorBase* selector, const EDungeonPartsSelectorTarget target, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random)
		{
			(void)fixtures;
			return FDungeonMeshSet::SelectPartsByGrid(gridLocation, gridIndex, grid, random, parts, selector, target);
		}
	}
}

#if WITH_EDITOR
#include <Misc/FileHelper.h>
#endif
#include <unordered_set>

UDungeonGenerateParameter::UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PluginVersion(DUNGEON_GENERATOR_PLUGIN_VERSION)
{
}

UDungeonGenerateParameter* UDungeonGenerateParameter::GenerateRandomParameter(const UDungeonGenerateParameter* sourceParameter) noexcept
{
	const auto random = std::make_unique<dungeon::Random>();

	UDungeonGenerateParameter* parameter = NewObject<UDungeonGenerateParameter>();
	parameter->SetRandomParameter();

	if (sourceParameter)
	{
		parameter->Structure = sourceParameter->Structure;
		parameter->Path = sourceParameter->Path;
		parameter->Zones = sourceParameter->Zones;
		parameter->Gameplay = sourceParameter->Gameplay;
		parameter->Theme = sourceParameter->Theme;
	}

	return parameter;
}

void UDungeonGenerateParameter::SetRandomParameter() noexcept
{
	const auto random = std::make_unique<dungeon::Random>();

	Structure.RoomWidth.Min = random->Get<int32>(1, 5);
	Structure.RoomWidth.Max = Structure.RoomWidth.Min + random->Get<int32>(1, 5);
	Structure.RoomDepth.Min = random->Get<int32>(1, 5);
	Structure.RoomDepth.Max = Structure.RoomDepth.Min + random->Get<int32>(1, 5);
	Structure.RoomHeight.Min = random->Get<int32>(1, 3);
	Structure.RoomHeight.Max = Structure.RoomHeight.Min + random->Get<int32>(1, 3);
	Structure.HorizontalRoomMargin = random->Get<uint8>(1, 5);
	Structure.VerticalRoomMargin = random->Get<uint8>(5);
	const int32 roomCount = random->Get<int32>(5, 50);
	Structure.RoomCountRange = { roomCount, roomCount };
	Structure.FloorMode = static_cast<EDungeonFloorMode>(random->Get<uint8>(0, static_cast<uint8>(EDungeonFloorMode::Vertical) + 1));
	Path.LayoutCandidateCount = random->Get<uint8>(3, 4);

	Path.ProgressionPolicy = random->Get<bool>() ? EDungeonProgressionPolicy::KeysAndLocks : EDungeonProgressionPolicy::StartToGoal;
	if (Path.ProgressionPolicy == EDungeonProgressionPolicy::KeysAndLocks)
	{
		Path.ExtraCorridorComplexity = 0;
	}
	else
	{
		Path.ExtraCorridorComplexity = random->Get<uint8>(0, 10);
	}

	Path.CorridorCeilingHeightPolicy = static_cast<EDungeonAisleCeilingHeightPolicy>(random->Get<uint8>(static_cast<uint8>(EDungeonAisleCeilingHeightPolicy::SIZE)));
}

void UDungeonGenerateParameter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams sharedParams;
	sharedParams.bIsPushBased = true;
	// DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, RandomSeed, sharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, GeneratedRandomSeed, sharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UDungeonGenerateParameter, GeneratedDungeonCRC32, sharedParams);
}

bool UDungeonGenerateParameter::IsSupportedForNetworking() const
{
	//return Super::IsSupportedForNetworking();
	return true;
}

void UDungeonGenerateParameter::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(FDungeonGeneratorAssetVersion::GUID);
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		LoadedAssetVersion = FDungeonGeneratorAssetVersion::Get(Ar);
	}
}

void UDungeonGenerateParameter::PostLoad()
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
void UDungeonGenerateParameter::MigrateFromAssetVersion(const int32 assetVersion)
{
	if (assetVersion < FDungeonGeneratorAssetVersion::Version2_0)
	{
		const bool bHasLegacyTopLevelData = HasLegacyTopLevelPropertyData();
		MigrateLegacyTopLevelProperties(true);
		if (!bHasLegacyTopLevelData && !bFixtureSelectionPoliciesMigrated)
		{
			MigrateLegacyFixtureSelectionPolicies();
		}
	}
	else if (!bFixtureSelectionPoliciesMigrated)
	{
		MigrateLegacyFixtureSelectionPolicies();
	}
	MigrateLegacyRoomSensorSettings();
}

/*
 * Migrates legacy top-level 1.x properties into the current grouped settings.
 * 旧1.xのトップレベルプロパティを現在のグループ化された設定へ移行します。
 */
void UDungeonGenerateParameter::MigrateLegacyTopLevelProperties(const bool bForceLegacyDefaults)
{
	if (!bForceLegacyDefaults && !HasLegacyTopLevelPropertyData())
	{
		return;
	}

	if (FMath::IsNearlyEqual(Theme.HorizontalGridSize, 400.f))
	{
		Theme.HorizontalGridSize = GridSize;
	}

	if (FMath::IsNearlyEqual(Theme.VerticalGridSize, 400.f))
	{
		Theme.VerticalGridSize = VerticalGridSize;
	}

	if (Structure.RoomCountRange.Min == 10 && Structure.RoomCountRange.Max == 10)
	{
		Structure.RoomCountRange = { NumberOfCandidateRooms, NumberOfCandidateRooms };
	}

	if (Structure.RoomWidth.Min == 3 && Structure.RoomWidth.Max == 8)
	{
		Structure.RoomWidth = RoomWidth;
	}

	if (Structure.RoomDepth.Min == 3 && Structure.RoomDepth.Max == 8)
	{
		Structure.RoomDepth = RoomDepth;
	}

	if (Structure.RoomHeight.Min == 2 && Structure.RoomHeight.Max == 4)
	{
		Structure.RoomHeight = RoomHeight;
	}

	if (Structure.HorizontalRoomMargin == 2)
	{
		Structure.HorizontalRoomMargin = RoomMargin;
	}

	if (Structure.VerticalRoomMargin == 0)
	{
		Structure.VerticalRoomMargin = VerticalRoomMargin;
	}

	if (Structure.FloorMode == EDungeonFloorMode::Free)
	{
		if (Flat)
		{
			Structure.FloorMode = EDungeonFloorMode::Flat;
		}
		else
		{
			switch (ExpansionPolicy)
			{
			case EDungeonExpansionPolicy::Flat:
				Structure.FloorMode = EDungeonFloorMode::Flat;
				break;
			case EDungeonExpansionPolicy::ExpandVertically:
				Structure.FloorMode = EDungeonFloorMode::Vertical;
				break;
			case EDungeonExpansionPolicy::ExpandAnyDirection:
			case EDungeonExpansionPolicy::ExpandHorizontally:
			default:
				Structure.FloorMode = EDungeonFloorMode::Free;
				break;
			}
		}
	}

	if (LayoutCandidateCount > 0 && Path.LayoutCandidateCount == 3)
	{
		Path.LayoutCandidateCount = FMath::Clamp(static_cast<int32>(LayoutCandidateCount), 3, 16);
	}

	if (bForceLegacyDefaults || Path.ExtraCorridorComplexity == 0)
	{
		Path.ExtraCorridorComplexity = AisleComplexity;
	}

	if (Path.CorridorCeilingHeightPolicy == EDungeonAisleCeilingHeightPolicy::Random)
	{
		Path.CorridorCeilingHeightPolicy = AisleCeilingHeightPolicy;
	}

	if (Path.ProgressionPolicy == EDungeonProgressionPolicy::StartToGoal)
	{
		Path.ProgressionPolicy = UseMissionGraph ? EDungeonProgressionPolicy::KeysAndLocks : EDungeonProgressionPolicy::StartToGoal;
	}

	if (Path.bMovePlayerStartToStartRoom)
	{
		Path.bMovePlayerStartToStartRoom = MovePlayerStartToStartingPoint;
	}

	if (Path.StartRoomPolicy == EDungeonStartLocationPolicy::UseSouthernMost)
	{
		Path.StartRoomPolicy = ToStartRoomPolicy(StartLocationPolicy);
	}

	if (Path.GoalRoomPolicy == EDungeonGoalLocationPolicy::UseSouthernMost)
	{
		Path.GoalRoomPolicy = ToGoalRoomPolicy(GoalLocationPolicy);
	}

	if (!IsValid(Theme.DungeonRoomMeshPartsDatabase) && IsValid(DungeonRoomMeshPartsDatabase))
	{
		Theme.DungeonRoomMeshPartsDatabase = DungeonRoomMeshPartsDatabase;
		Theme.DungeonRoomMeshPartsDatabase->MigrateSelectionPolicies();
	}

	if (!IsValid(Theme.DungeonAisleMeshPartsDatabase) && IsValid(DungeonAisleMeshPartsDatabase))
	{
		Theme.DungeonAisleMeshPartsDatabase = DungeonAisleMeshPartsDatabase;
		Theme.DungeonAisleMeshPartsDatabase->MigrateSelectionPolicies();
	}

	if (Theme.Fixtures.PillarParts.Num() <= 0 && PillarParts.Num() > 0)
	{
		Theme.Fixtures.PillarParts = PillarParts;
	}

	if (Theme.Fixtures.TorchParts.Num() <= 0 && TorchParts.Num() > 0)
	{
		Theme.Fixtures.TorchParts = TorchParts;
	}

	if (Theme.Fixtures.DoorParts.Num() <= 0 && DoorParts.Num() > 0)
	{
		Theme.Fixtures.DoorParts = DoorParts;
	}

	if (Theme.Fixtures.PillarPartsSelectionPolicy == EDungeonSelectionPolicy::Random &&
		(PillarParts.Num() > 0 || PillarPartsSelectionPolicy != EDungeonSelectionPolicy::Random || PillarPartsSelectionMethod != EDungeonPartsSelectionMethod::Random))
	{
		Theme.Fixtures.PillarPartsSelectionPolicy = PillarPartsSelectionPolicy != EDungeonSelectionPolicy::Random ? PillarPartsSelectionPolicy : dungeon::selection::ToPolicy(PillarPartsSelectionMethod);
		Theme.Fixtures.PillarPartsSelectionMethod = PillarPartsSelectionMethod;
	}

	if (Theme.Fixtures.TorchPartsSelectionPolicy == EDungeonSelectionPolicy::Random &&
		(TorchParts.Num() > 0 || TorchPartsSelectionPolicy != EDungeonSelectionPolicy::Random || TorchPartsSelectionMethod != EDungeonPartsSelectionMethod::Random))
	{
		Theme.Fixtures.TorchPartsSelectionPolicy = TorchPartsSelectionPolicy != EDungeonSelectionPolicy::Random ? TorchPartsSelectionPolicy : dungeon::selection::ToPolicy(TorchPartsSelectionMethod);
		Theme.Fixtures.TorchPartsSelectionMethod = TorchPartsSelectionMethod;
	}

	if (Theme.Fixtures.DoorPartsSelectionPolicy == EDungeonSelectionPolicy::Random &&
		(DoorParts.Num() > 0 || DoorPartsSelectionPolicy != EDungeonSelectionPolicy::Random || DoorPartsSelectionMethod != EDungeonPartsSelectionMethod::Random))
	{
		Theme.Fixtures.DoorPartsSelectionPolicy = DoorPartsSelectionPolicy != EDungeonSelectionPolicy::Random ? DoorPartsSelectionPolicy : dungeon::selection::ToPolicy(DoorPartsSelectionMethod);
		Theme.Fixtures.DoorPartsSelectionMethod = DoorPartsSelectionMethod;
	}

	if (Theme.Fixtures.FrequencyOfTorchlightGeneration == EDungeonFrequencyOfGeneration::Rarely)
	{
		Theme.Fixtures.FrequencyOfTorchlightGeneration = FrequencyOfTorchlightGeneration;
	}

	if (!IsValid(Theme.Fixtures.DungeonPartsSelector) && IsValid(DungeonPartsSelector))
	{
		Theme.Fixtures.DungeonPartsSelector = DungeonPartsSelector;
	}


	bFixtureSelectionPoliciesMigrated = true;
}

/*
 * Returns true when legacy top-level properties contain data that should be copied into current settings.
 * 現在の設定へコピーすべき旧トップレベルプロパティのデータがある場合にtrueを返します。
 */
bool UDungeonGenerateParameter::HasLegacyTopLevelPropertyData() const
{
	return !FMath::IsNearlyEqual(GridSize, 400.f) ||
		!FMath::IsNearlyEqual(VerticalGridSize, 400.f) ||
		NumberOfCandidateRooms != 10 ||
		RoomWidth.Min != 3 ||
		RoomWidth.Max != 8 ||
		RoomDepth.Min != 3 ||
		RoomDepth.Max != 8 ||
		RoomHeight.Min != 2 ||
		RoomHeight.Max != 4 ||
		RoomMargin != 2 ||
		VerticalRoomMargin != 0 ||
		ExpansionPolicy != EDungeonExpansionPolicy::ExpandHorizontally ||
		Flat ||
		LayoutCandidateCount > 0 ||
		UseMissionGraph ||
		AisleComplexity != 5 ||
		AisleCeilingHeightPolicy != EDungeonAisleCeilingHeightPolicy::Random ||
		!MovePlayerStartToStartingPoint ||
		StartLocationPolicy != EDungeonStartLocationPolicy::UseSouthernMost ||
		GoalLocationPolicy != EDungeonStartLocationPolicy::UseSouthernMost ||
		IsValid(DungeonRoomMeshPartsDatabase) ||
		IsValid(DungeonAisleMeshPartsDatabase) ||
		PillarPartsSelectionPolicy != EDungeonSelectionPolicy::Random ||
		PillarPartsSelectionMethod != EDungeonPartsSelectionMethod::Random ||
		PillarParts.Num() > 0 ||
		TorchPartsSelectionPolicy != EDungeonSelectionPolicy::Random ||
		TorchPartsSelectionMethod != EDungeonPartsSelectionMethod::Random ||
		FrequencyOfTorchlightGeneration != EDungeonFrequencyOfGeneration::Rarely ||
		TorchParts.Num() > 0 ||
		DoorPartsSelectionPolicy != EDungeonSelectionPolicy::Random ||
		DoorPartsSelectionMethod != EDungeonPartsSelectionMethod::Random ||
		DoorParts.Num() > 0 ||
		IsValid(DungeonPartsSelector) ||
		IsValid(DungeonRoomSensorClass) ||
		IsValid(DungeonRoomSensorDatabase);
}

/*
 * Applies compatibility fixups that are still required after version-specific migration.
 * バージョン別移行後も必要な互換補正を適用します。
 */
void UDungeonGenerateParameter::MigrateLegacyRoomSensorSettings()
{
	if (!IsValid(Gameplay.DungeonRoomSensorClass) && IsValid(DungeonRoomSensorClass))
	{
		Gameplay.DungeonRoomSensorClass = DungeonRoomSensorClass;
	}

	const UDungeonRoomSensorDatabase* legacyRoomSensorDatabase = IsValid(Gameplay.DungeonRoomSensorDatabase) ?
		Gameplay.DungeonRoomSensorDatabase.Get() :
		DungeonRoomSensorDatabase.Get();
	if (legacyRoomSensorDatabase == nullptr)
	{
		return;
	}

	if (!IsValid(Gameplay.DungeonRoomSensorClass))
	{
		Gameplay.DungeonRoomSensorClass = legacyRoomSensorDatabase->GetFirstValidRoomSensorClass();
	}

	if (Gameplay.SpawnActorInAisle.IsEmpty())
	{
		Gameplay.SpawnActorInAisle = legacyRoomSensorDatabase->GetSpawnActorInAisle();
	}
}

void UDungeonGenerateParameter::ApplyPostLoadCompatibilityFixups()
{
	if (IsUseMissionGraph() &&
		(Path.StartRoomPolicy == EDungeonStartLocationPolicy::UseCentralPoint ||
			Path.StartRoomPolicy == EDungeonStartLocationPolicy::UseMultiStart))
	{
		Path.StartRoomPolicy = EDungeonStartLocationPolicy::UseSouthernMost;
	}

	generateParameter::SanitizeFixtureSettings(this, Theme.Fixtures);
	for (FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
	{
		generateParameter::SanitizeFixtureSettings(this, profile.ThemeOverride.Fixtures);
	}
	for (FDungeonZoneDefinition& zone : Zones.Zones)
	{
		generateParameter::SanitizeFixtureSettings(this, zone.ThemeOverride.Fixtures);
	}
	bFixtureSelectionPoliciesMigrated = true;
}

/*
 * Converts legacy selection methods into the current selection policy fields.
 * 旧選択方式を現在の選択ポリシーフィールドへ変換します。
 */
void UDungeonGenerateParameter::MigrateLegacyFixtureSelectionPolicies()
{
	if (Theme.Fixtures.PillarPartsSelectionPolicy == EDungeonSelectionPolicy::Random)
	{
		Theme.Fixtures.PillarPartsSelectionPolicy = dungeon::selection::ToPolicy(Theme.Fixtures.PillarPartsSelectionMethod);
	}

	if (Theme.Fixtures.TorchPartsSelectionPolicy == EDungeonSelectionPolicy::Random)
	{
		Theme.Fixtures.TorchPartsSelectionPolicy = dungeon::selection::ToPolicy(Theme.Fixtures.TorchPartsSelectionMethod);
	}

	if (Theme.Fixtures.DoorPartsSelectionPolicy == EDungeonSelectionPolicy::Random)
	{
		Theme.Fixtures.DoorPartsSelectionPolicy = dungeon::selection::ToPolicy(Theme.Fixtures.DoorPartsSelectionMethod);
	}

	if (Theme.Fixtures.UniqueDoorPartsSelectionPolicy == EDungeonSelectionPolicy::Random)
	{
		Theme.Fixtures.UniqueDoorPartsSelectionPolicy = dungeon::selection::ToPolicy(Theme.Fixtures.UniqueDoorPartsSelectionMethod);
	}

	bFixtureSelectionPoliciesMigrated = true;
}

#if WITH_EDITOR
void UDungeonGenerateParameter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bFixtureSelectionPoliciesMigrated = true;

	generateParameter::SanitizeFixtureSettings(this, Theme.Fixtures);
	for (FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
	{
		generateParameter::SanitizeFixtureSettings(this, profile.ThemeOverride.Fixtures);
	}
	for (FDungeonZoneDefinition& zone : Zones.Zones)
	{
		generateParameter::SanitizeFixtureSettings(this, zone.ThemeOverride.Fixtures);
	}
}
#endif

#if WITH_EDITOR

void UDungeonGenerateParameter::Dump() const
{
	DumpToJson();
}
#endif

void UDungeonGenerateParameter::SetRandomSeed(const int32 generateRandomSeed)
{
	RandomSeed = generateRandomSeed;
}

void UDungeonGenerateParameter::SetGeneratedRandomSeed(const int32 generatedRandomSeed)
{
	/*
	リプリケーションが有効の場合
	サーバーからクライアントへダンジョンを生成した乱数の種が送信される
	クライアントのCDungeonGeneratorCore::Createから新たに設定するが同じ値なので何もしない
	*/
	if (GeneratedRandomSeed != generatedRandomSeed)
	{
		GeneratedRandomSeed = generatedRandomSeed;
		MARK_PROPERTY_DIRTY_FROM_NAME(UDungeonGenerateParameter, GeneratedRandomSeed, this);
	}
}

int32 UDungeonGenerateParameter::GetGeneratedDungeonCRC32() const noexcept
{
	return GeneratedDungeonCRC32;
}

void UDungeonGenerateParameter::SetGeneratedDungeonCRC32(const int32 generatedDungeonCRC32) noexcept
{
	GeneratedDungeonCRC32 = generatedDungeonCRC32;
}


/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const FIntVector& location) const
{
	return FVector(
		static_cast<float>(location.X),
		static_cast<float>(location.Y),
		static_cast<float>(location.Z)
	) * GetGridSize().To3D();
}

/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const
{
	return FVector(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	) * GetGridSize().To3D();
}

FIntVector UDungeonGenerateParameter::ToGrid(const FVector& location) const
{
	return FIntVector(
		static_cast<int32>(location.X / GetGridSize().HorizontalSize),
		static_cast<int32>(location.Y / GetGridSize().HorizontalSize),
		static_cast<int32>(location.Z / GetGridSize().VerticalSize)
	);
}

#if WITH_EDITOR
void UDungeonGenerateParameter::DumpToJson() const
{
	auto boolValue = [](const bool value) -> FString
		{
			return value ? TEXT("true") : TEXT("false");
		};

	FString jsonString(TEXT("{\n"));
	{
		jsonString += TEXT(" \"Version\":\"") + FString(TEXT(DUNGEON_GENERATOR_PLUGIN_VERSION_NAME)) + TEXT("\",\n");
		jsonString += TEXT(" \"Tag\":\"") + FString(TEXT(JENKINS_JOB_TAG)) + TEXT("\",\n");
		jsonString += TEXT(" \"UUID\":\"") + FString(TEXT(JENKINS_UUID)) + TEXT("\",\n");
		jsonString += TEXT(" \"License\":\"") + FString(TEXT(JENKINS_LICENSE)) + TEXT("\",\n");

		jsonString += TEXT(" \"RandomSeed\":") + FString::FromInt(RandomSeed) + TEXT(",\n");
		jsonString += TEXT(" \"GeneratedRandomSeed\":") + FString::FromInt(GeneratedRandomSeed) + TEXT(",\n");
		jsonString += TEXT(" \"GeneratedDungeonCRC32\":") + FString::FromInt(GeneratedDungeonCRC32) + TEXT(",\n");
		jsonString += TEXT(" \"NumberOfCandidateRooms\":") + FString::FromInt(GetNumberOfCandidateRooms()) + TEXT(",\n");
		jsonString += TEXT(" \"RoomWidth\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(Structure.RoomWidth.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(Structure.RoomWidth.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"RoomDepth\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(Structure.RoomDepth.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(Structure.RoomDepth.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"RoomHeight\":{\n");
		jsonString += TEXT("  \"Min\":") + FString::FromInt(Structure.RoomHeight.Min) + TEXT(",\n");
		jsonString += TEXT("  \"Max\":") + FString::FromInt(Structure.RoomHeight.Max) + TEXT("\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"HorizontalRoomMargin\":") + FString::FromInt(GetHorizontalRoomMargin()) + TEXT(",\n");
		jsonString += TEXT(" \"VerticalRoomMargin\":") + FString::FromInt(GetVerticalRoomMargin()) + TEXT(",\n");
		jsonString += TEXT(" \"FloorMode\":\"") + UEnum::GetValueAsString(Structure.FloorMode) + TEXT("\",\n");
		jsonString += TEXT(" \"MovePlayerStartToStartingPoint\":") + boolValue(IsMovePlayerStartToStartingPoint()) + TEXT(",\n");
		jsonString += TEXT(" \"UseMissionGraph\":") + boolValue(IsUseMissionGraph()) + TEXT(",\n");
		jsonString += TEXT(" \"Path.LayoutCandidateCount\":") + FString::FromInt(Path.LayoutCandidateCount) + TEXT(",\n");
		jsonString += TEXT(" \"Path.MainRouteBias\":") + FString::SanitizeFloat(Path.MainRouteBias) + TEXT(",\n");
		jsonString += TEXT(" \"Path.LoopRouteDensity\":") + FString::SanitizeFloat(Path.LoopRouteDensity) + TEXT(",\n");
		jsonString += TEXT(" \"Path.ExtraCorridorComplexity\":") + FString::FromInt(Path.ExtraCorridorComplexity) + TEXT(",\n");
		jsonString += TEXT(" \"Path.CorridorCeilingHeightPolicy\":\"") + UEnum::GetValueAsString(Path.CorridorCeilingHeightPolicy) + TEXT("\",\n");
		jsonString += TEXT(" \"AisleComplexity\":") + FString::FromInt(GetAisleComplexity()) + TEXT(",\n");
		jsonString += TEXT(" \"AisleCeilingHeightPolicy\":\"") + UEnum::GetValueAsString(Path.CorridorCeilingHeightPolicy) + TEXT("\",\n");
		jsonString += TEXT(" \"Theme.HorizontalGridSize\":") + FString::SanitizeFloat(Theme.HorizontalGridSize) + TEXT(",\n");
		jsonString += TEXT(" \"Theme.VerticalGridSize\":") + FString::SanitizeFloat(Theme.VerticalGridSize);
		if (IsValid(Theme.DungeonRoomMeshPartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"Theme.DungeonRoomMeshPartsDatabase\":{\n");
			jsonString += Theme.DungeonRoomMeshPartsDatabase->DumpToJson(2) + TEXT("\n");
			jsonString += TEXT(" }");
		}
		if (IsValid(Theme.DungeonAisleMeshPartsDatabase))
		{
			jsonString += TEXT(",\n");
			jsonString += TEXT(" \"Theme.DungeonAisleMeshPartsDatabase\":{\n");
			jsonString += Theme.DungeonAisleMeshPartsDatabase->DumpToJson(2) + TEXT("\n");
			jsonString += TEXT(" }");
		}
		jsonString += TEXT(",\n");
		jsonString += TEXT(" \"Theme.PillarParts\":{\n");
		jsonString += TEXT("  \"Theme.PillarPartsSelectionMethod\":\"") + UEnum::GetValueAsString(Theme.Fixtures.PillarPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < Theme.Fixtures.PillarParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += Theme.Fixtures.PillarParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"Theme.TorchParts\":{\n");
		jsonString += TEXT("  \"Theme.TorchPartsSelectionMethod\":\"") + UEnum::GetValueAsString(Theme.Fixtures.TorchPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < Theme.Fixtures.TorchParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += Theme.Fixtures.TorchParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"Theme.DoorParts\":{\n");
		jsonString += TEXT("  \"Theme.DoorPartsSelectionMethod\":\"") + UEnum::GetValueAsString(Theme.Fixtures.DoorPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < Theme.Fixtures.DoorParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += Theme.Fixtures.DoorParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" },\n");
		jsonString += TEXT(" \"Theme.UniqueDoorParts\":{\n");
		jsonString += TEXT("  \"Theme.UniqueDoorPartsSelectionMethod\":\"") + UEnum::GetValueAsString(Theme.Fixtures.UniqueDoorPartsSelectionMethod) + TEXT("\",\n");
		jsonString += TEXT("  \"Parts\":[\n");
		for (int32 i = 0; i < Theme.Fixtures.UniqueDoorParts.Num(); ++i)
		{
			if (i != 0)
				jsonString += TEXT(",\n");
			jsonString += TEXT("   {\n");
			jsonString += Theme.Fixtures.UniqueDoorParts[i].DumpToJson(4) + TEXT("\n");
			jsonString += TEXT("   }");
		}
		jsonString += TEXT("\n");
		jsonString += TEXT("  ]\n");
		jsonString += TEXT(" }");
		jsonString += TEXT("\n");
	}
	jsonString += TEXT("}");

	const FString fileName(GetName() + ".json");
	const FString filePath(GetJsonDefaultDirectory() / fileName);
	FFileHelper::SaveStringToFile(jsonString, *filePath);
}

FString UDungeonGenerateParameter::GetJsonDefaultDirectory() const
{
	return dungeon::GetDebugDirectory();
}
#endif

UClass* UDungeonGenerateParameter::ResolveRoomSensorClass(const EDungeonRoomGameplayRole gameplayRole, const int32 zoneIndex) const
{
	for (const FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
	{
		if (profile.Role != gameplayRole)
		{
			continue;
		}
		if (IsValid(profile.GameplayOverride.DungeonRoomSensorClass))
		{
			return profile.GameplayOverride.DungeonRoomSensorClass;
		}
		break;
	}

	if (Zones.Zones.IsValidIndex(zoneIndex) && IsValid(Zones.Zones[zoneIndex].GameplayOverride.DungeonRoomSensorClass))
	{
		return Zones.Zones[zoneIndex].GameplayOverride.DungeonRoomSensorClass;
	}

	return IsValid(Gameplay.DungeonRoomSensorClass) ? Gameplay.DungeonRoomSensorClass.Get() : nullptr;
}

const FDungeonZoneThemeOverride* UDungeonGenerateParameter::ResolveThemeOverride(const EDungeonRoomGameplayRole gameplayRole, const int32 zoneIndex, const bool bRoomGrid) const noexcept
{
	if (bRoomGrid)
	{
		for (const FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
		{
			if (profile.Role == gameplayRole)
			{
				return &profile.ThemeOverride;
			}
		}
	}

	if (Zones.Zones.IsValidIndex(zoneIndex))
	{
		return &Zones.Zones[zoneIndex].ThemeOverride;
	}

	return nullptr;
}

const FDungeonFixtureSettings& UDungeonGenerateParameter::ResolveFixtureSettings(const EDungeonRoomGameplayRole gameplayRole, const int32 zoneIndex, const bool bRoomGrid) const noexcept
{
	if (bRoomGrid)
	{
		for (const FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
		{
			if (profile.Role == gameplayRole)
			{
				if (profile.ThemeOverride.bOverrideFixtures)
				{
					return profile.ThemeOverride.Fixtures;
				}
				break;
			}
		}
	}

	if (Zones.Zones.IsValidIndex(zoneIndex) && Zones.Zones[zoneIndex].ThemeOverride.bOverrideFixtures)
	{
		return Zones.Zones[zoneIndex].ThemeOverride.Fixtures;
	}

	return Theme.Fixtures;
}


const UDungeonMeshSetDatabase* UDungeonGenerateParameter::GetDungeonMeshPartsDatabase(const FIntVector& gridLocation, const dungeon::Grid& grid) const noexcept
{
	(void)gridLocation;

	const bool bRoomGrid = dungeon::Identifier(grid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	const UDungeonMeshSetDatabase* defaultDatabase = bRoomGrid ? GetDungeonRoomMeshPartsDatabase() : GetDungeonAisleMeshPartsDatabase();
	if (bRoomGrid)
	{
		for (const FDungeonRoomRoleProfile& profile : Gameplay.RoomRoles.Roles)
		{
			if (profile.Role != grid.GetRoomGameplayRole())
			{
				continue;
			}
			if (IsValid(profile.ThemeOverride.RoomMeshSetDatabase))
			{
				return profile.ThemeOverride.RoomMeshSetDatabase;
			}
			break;
		}
	}

	if (Zones.Zones.IsValidIndex(grid.GetZoneIndex()))
	{
		const FDungeonZoneDefinition& zone = Zones.Zones[grid.GetZoneIndex()];
		const UDungeonMeshSetDatabase* overrideDatabase = bRoomGrid ?
			zone.ThemeOverride.RoomMeshSetDatabase :
			zone.ThemeOverride.AisleMeshSetDatabase;
		return IsValid(overrideDatabase) ? overrideDatabase : defaultDatabase;
	}

	return defaultDatabase;
}

FDungeonMeshSetQuery UDungeonGenerateParameter::MakeMeshSetQuery(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid)
{
	FDungeonMeshSetQuery query;
	query.GridX = gridLocation.X;
	query.GridY = gridLocation.Y;
	query.GridZ = gridLocation.Z;
	query.RoomId = static_cast<int32>(grid.GetIdentifier());
	query.RoomStructuralRole = grid.GetRoomStructuralRole();
	query.RoomGameplayRole = grid.GetRoomGameplayRole();
	query.ZoneIndex = grid.GetZoneIndex();
	query.DepthFromStart = static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f;
	query.DistanceToGoal = 1.f - query.DepthFromStart;
	query.SeedKey = static_cast<int32>(gridIndex);
	return query;
}

const FDungeonMeshSet* UDungeonGenerateParameter::SelectMeshSet(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	if (IsValid(dungeonMeshSetDatabase) == false)
		return nullptr;

	const FDungeonMeshSetQuery query = MakeMeshSetQuery(gridLocation, gridIndex, grid);
	return dungeonMeshSetDatabase->SelectImplement(grid.GetIdentifier(), grid.GetDepthRatioFromStart(), random, query);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectFloorParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetFloorPartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			// Legacy fallback for old assets that used UDungeonGenerateParameter selector for mesh-set parts.
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Floor, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetFloorPartsCount());
				if (const auto* parts = meshSet->GetFloorPartsAt(index))
					return parts;
			}
		}
		return meshSet->SelectFloorParts(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectCatwalkParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetCatwalkPartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Catwalk, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetCatwalkPartsCount());
				if (const auto* parts = meshSet->GetCatwalkPartsAt(index))
					return parts;
			}
		}
		return meshSet->SelectCatwalkParts(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallPartsByGrid(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetWallPartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Wall, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetWallPartsCount());
				if (const auto* parts = meshSet->GetWallPartsAt(index))
					return parts;
			}
		}
		return meshSet->SelectWallPartsByGrid(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallPartsByFace(const FDungeonMeshSet* dungeonMeshSet, const FIntVector& gridLocation, const dungeon::Direction& direction)
{
	check(dungeonMeshSet);
	return dungeonMeshSet->SelectWallPartsByFace(gridLocation, direction);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectRoofParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetRoofPartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Roof, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetRoofPartsCount());
				if (const auto* parts = meshSet->GetRoofPartsAt(index))
					return parts;
			}
		}
		return meshSet->SelectRoofParts(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectSlopeParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetSlopePartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Slope, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetSlopePartsCount());
				if (const auto* parts = meshSet->GetSlopePartsAt(index))
					return parts;
			}
		}
		return meshSet->SelectSlopeParts(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectPillarParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	const bool bRoomGrid = dungeon::Identifier(grid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	const FDungeonFixtureSettings& fixtures = ResolveFixtureSettings(grid.GetRoomGameplayRole(), grid.GetZoneIndex(), bRoomGrid);
	if (fixtures.PillarPartsSelectionPolicy == EDungeonSelectionPolicy::CustomSelector)
	{
		const int32 index = generateParameter::SelectCustomPartsIndex(fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Pillar, gridLocation, gridIndex, grid, 0, random, fixtures.PillarParts.Num());
		if (const auto* parts = generateParameter::GetPartsAt(fixtures.PillarParts, index))
			return parts;
	}
	return FDungeonMeshSet::SelectPartsByGrid(gridLocation, gridIndex, grid, random, fixtures.PillarParts, fixtures.PillarPartsSelector, EDungeonPartsSelectorTarget::Pillar);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectTorchParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const
{
	const bool bRoomGrid = dungeon::Identifier(grid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	const FDungeonFixtureSettings& fixtures = ResolveFixtureSettings(grid.GetRoomGameplayRole(), grid.GetZoneIndex(), bRoomGrid);
	if (fixtures.TorchPartsSelectionPolicy == EDungeonSelectionPolicy::CustomSelector)
	{
		const int32 index = generateParameter::SelectCustomPartsIndex(fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Torch, gridLocation, gridIndex, grid, 0, random, fixtures.TorchParts.Num());
		if (const auto* parts = generateParameter::GetPartsAt(fixtures.TorchParts, index))
		{
			if (!IsValid(parts->ActorClass))
				return nullptr;

			const float value = random->Get<float>();
			if (value > parts->Frequency)
				return nullptr;

			return parts;
		}
	}
	return FDungeonMeshSet::SelectRandomActorParts(gridLocation, gridIndex, grid, random, fixtures.TorchParts, fixtures.TorchPartsSelector, EDungeonPartsSelectorTarget::Torch);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectChandelierParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	if (const auto* meshSet = SelectMeshSet(dungeonMeshSetDatabase, gridLocation, gridIndex, grid, random))
	{
		if (meshSet->GetChandelierPartsSelectionPolicy() == EDungeonSelectionPolicy::CustomSelector)
		{
			if (!IsValid(meshSet->GetDungeonPartsSelector()))
			{
				const int32 index = generateParameter::SelectCustomPartsIndex(Theme.Fixtures.DungeonPartsSelector, EDungeonPartsSelectorTarget::Chandelier, gridLocation, gridIndex, grid, neighborMask6, random, meshSet->GetChandelierPartsCount());
				if (const auto* parts = meshSet->GetChandelierPartsAt(index))
				{
					if (!IsValid(parts->ActorClass))
						return nullptr;

					const float value = random->Get<float>();
					if (value > parts->Frequency)
						return nullptr;

					return parts;
				}
			}
		}
		return meshSet->SelectChandelierParts(gridLocation, gridIndex, grid, random, neighborMask6);
	}
	return nullptr;
}

const FDungeonDoorActorParts* UDungeonGenerateParameter::SelectDoorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const EDungeonRoomProps props, const std::shared_ptr<dungeon::Random>& random) const
{
	const bool bRoomGrid = dungeon::Identifier(grid.GetIdentifier()).IsType(dungeon::Identifier::Type::Aisle) == false;
	const FDungeonFixtureSettings& fixtures = ResolveFixtureSettings(grid.GetRoomGameplayRole(), grid.GetZoneIndex(), bRoomGrid);

	if (props == EDungeonRoomProps::UniqueLock && fixtures.UniqueDoorParts.Num() > 0)
	{
		if (const FDungeonDoorActorParts* uniqueParts = generateParameter::SelectFixtureDoorParts(fixtures, fixtures.UniqueDoorParts, fixtures.UniqueDoorPartsSelector, EDungeonPartsSelectorTarget::UniqueDoor, gridLocation, gridIndex, grid, random))
		{
			if (IsValid(uniqueParts->ActorClass))
				return uniqueParts;
		}
	}

	return generateParameter::SelectFixtureDoorParts(fixtures, fixtures.DoorParts, fixtures.DoorPartsSelector, EDungeonPartsSelectorTarget::Door, gridLocation, gridIndex, grid, random);
}

void UDungeonGenerateParameter::EachRoomFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(Theme.DungeonRoomMeshPartsDatabase))
	{
		Theme.DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(Theme.DungeonAisleMeshPartsDatabase))
	{
		Theme.DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachFloorParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(Theme.DungeonRoomMeshPartsDatabase))
	{
		Theme.DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(Theme.DungeonAisleMeshPartsDatabase))
	{
		Theme.DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachWallParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(Theme.DungeonRoomMeshPartsDatabase))
	{
		Theme.DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	if (IsValid(Theme.DungeonAisleMeshPartsDatabase))
	{
		Theme.DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachRoofParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(Theme.DungeonRoomMeshPartsDatabase))
	{
		Theme.DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachSlopeParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (IsValid(Theme.DungeonAisleMeshPartsDatabase))
	{
		Theme.DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachSlopeParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachRoomCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (Theme.DungeonRoomMeshPartsDatabase)
	{
		Theme.DungeonRoomMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachCatwalkParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachAisleCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	if (Theme.DungeonAisleMeshPartsDatabase)
	{
		Theme.DungeonAisleMeshPartsDatabase->Each([&function](const FDungeonMeshSet& parts)
			{
				parts.EachCatwalkParts(function);
			}
		);
	}
}

void UDungeonGenerateParameter::EachPillarParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	FDungeonMeshSet::EachParts(Theme.Fixtures.PillarParts, function);
}

void UDungeonGenerateParameter::OnEndGeneration(UDungeonRandom* synchronizedRandom, const UDungeonAisleGridMap* aisleGridMap, const std::function<void(const FSoftObjectPath&, const FTransform&)>& spawnActor) const
{
	if (synchronizedRandom == nullptr)
		return;
	if (aisleGridMap == nullptr)
		return;
	aisleGridMap->Each([this, synchronizedRandom, spawnActor](const TArray<FDungeonAisleGrid>& aisleGridArray)
		{
			std::unordered_set<int32> gridIndexes;
			const int32 totalGrids = aisleGridArray.Num();
			const int32 count = totalGrids / 3;
			for (int32 i = 0; i < count; ++i)
			{
				gridIndexes.emplace(synchronizedRandom->GetIntegerFrom(totalGrids));
			}

			for (const auto gridIndex : gridIndexes)
			{
				const FDungeonAisleGrid& aisleGrid = aisleGridArray[gridIndex];
				const TArray<FSoftObjectPath>& spawnActorInAisle = ResolveSpawnActorInAisle(aisleGrid);
				if (spawnActorInAisle.IsEmpty())
				{
					continue;
				}

				const FTransform transform(
					dungeon::detail::ToRotator(aisleGrid.Direction),
					aisleGrid.Location + FVector(0, 0, Theme.VerticalGridSize / 2.f)
				);

				const int32 actorType = synchronizedRandom->GetIntegerFrom(spawnActorInAisle.Num());
				spawnActor(spawnActorInAisle[actorType], transform);
			}
		}
	);
}

const TArray<FSoftObjectPath>& UDungeonGenerateParameter::ResolveSpawnActorInAisle(const FDungeonAisleGrid& aisleGrid) const
{
	if (Zones.Zones.IsValidIndex(aisleGrid.ZoneIndex) && !Zones.Zones[aisleGrid.ZoneIndex].GameplayOverride.SpawnActorInAisle.IsEmpty())
	{
		return Zones.Zones[aisleGrid.ZoneIndex].GameplayOverride.SpawnActorInAisle;
	}
	return Gameplay.SpawnActorInAisle;
}
