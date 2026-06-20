/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#include "Validation/DungeonParameterValidator.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonMeshSetDatabase.h"

#include <Misc/PackageName.h>
#include <Containers/Set.h>

namespace
{
	FDungeonValidationIssue MakeIssue(
		const EDungeonValidationSeverity severity,
		const FName code,
		const FText& message,
		const FText& hint,
		const FName parameterName,
		const FSoftObjectPath& relatedAsset = FSoftObjectPath())
	{
		FDungeonValidationIssue issue;
		issue.Severity = severity;
		issue.Code = code;
		issue.Message = message;
		issue.FixHint = hint;
		issue.ParameterName = parameterName;
		issue.RelatedAsset = relatedAsset;
		issue.bCanAutoFix = false;
		return issue;
	}

	bool HasAnyStaticMesh(const UDungeonMeshSetDatabase* database, const int32 meshType)
	{
		if (!database)
		{
			return false;
		}

		bool found = false;
		database->Each([&found, meshType](const FDungeonMeshSet& meshSet)
			{
				auto checkMeshParts = [&found](const FDungeonMeshParts& parts)
					{
						if (parts.StaticMesh != nullptr)
						{
							found = true;
						}
					};

				switch (meshType)
				{
				case 0:
					meshSet.EachFloorParts(checkMeshParts);
					break;
				case 1:
					meshSet.EachWallParts(checkMeshParts);
					break;
				case 2:
					meshSet.EachRoofParts(checkMeshParts);
					break;
				case 3:
					meshSet.EachSlopeParts(checkMeshParts);
					break;
				default:
					break;
				}
			});
		return found;
	}

	void ValidateAssetPath(const FSoftObjectPath& objectPath, const FName parameterName, TArray<FDungeonValidationIssue>& outIssues)
	{
		if (!objectPath.IsValid())
		{
			return;
		}

		if (!FPackageName::DoesPackageExist(objectPath.GetLongPackageName()))
		{
			outIssues.Emplace(MakeIssue(
				EDungeonValidationSeverity::Error,
				TEXT("DG_ASSET_MISSING"),
				FText::Format(NSLOCTEXT("DungeonParameterValidator", "MissingAsset", "Referenced asset does not exist: {0}"), FText::FromString(objectPath.ToString())),
				NSLOCTEXT("DungeonParameterValidator", "MissingAssetHint", "Fix the asset reference or reassign a valid asset."),
				parameterName,
				objectPath
			));
		}
	}

	void ValidateFixtureAssets(const FDungeonFixtureSettings& fixtures, const FName parameterName, TArray<FDungeonValidationIssue>& outIssues)
	{
		ValidateAssetPath(FSoftObjectPath(fixtures.DungeonPartsSelector), FName(*(FString(parameterName.ToString()) + TEXT(".DungeonPartsSelector"))), outIssues);
		for (const FDungeonRandomActorParts& torchParts : fixtures.TorchParts)
		{
			ValidateAssetPath(FSoftObjectPath(torchParts.ActorClass), FName(*(FString(parameterName.ToString()) + TEXT(".TorchParts.ActorClass"))), outIssues);
		}
		for (const FDungeonDoorActorParts& doorParts : fixtures.DoorParts)
		{
			ValidateAssetPath(FSoftObjectPath(doorParts.ActorClass), FName(*(FString(parameterName.ToString()) + TEXT(".DoorParts.ActorClass"))), outIssues);
		}
		for (const FDungeonDoorActorParts& doorParts : fixtures.UniqueDoorParts)
		{
			ValidateAssetPath(FSoftObjectPath(doorParts.ActorClass), FName(*(FString(parameterName.ToString()) + TEXT(".UniqueDoorParts.ActorClass"))), outIssues);
		}
	}
}

void FDungeonParameterValidator::Validate(const UDungeonGenerateParameter* params, TArray<FDungeonValidationIssue>& outIssues, const bool bDeepCheck)
{
	outIssues.Reset();

	if (params == nullptr)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_NULL"),
			NSLOCTEXT("DungeonParameterValidator", "NullParameter", "DungeonGenerateParameter is not set."),
			NSLOCTEXT("DungeonParameterValidator", "NullParameterHint", "Assign a valid DungeonGenerateParameter asset before generation."),
			TEXT("DungeonGenerateParameter")
		));
		return;
	}

	const auto& structure = params->GetStructureSettings();
	const auto& path = params->GetPathSettings();
	const auto& roomRoles = params->GetRoomRoleSettings();
	const auto& zones = params->GetZoneSettings();
	const auto& theme = params->GetThemeSettings();
	const auto& gameplay = params->GetGameplaySpawnSettings();

	if (theme.HorizontalGridSize <= 0.f)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "HorizontalGridSizeInvalid", "HorizontalGridSize must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "GridSizeInvalidHint", "Set Horizontal Size to a positive value."),
			TEXT("Theme.HorizontalGridSize")
		));
	}

	if (theme.VerticalGridSize <= 0.f)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "VerticalGridSizeInvalid", "VerticalGridSize must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "VerticalGridSizeInvalidHint", "Set Vertical Size to a positive value."),
			TEXT("Theme.VerticalGridSize")
		));
	}

	if (structure.RoomWidth.Min > structure.RoomWidth.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomWidthInvalid", "RoomWidth.Min cannot be greater than RoomWidth.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomWidthInvalidHint", "Set RoomWidth Min to be less than or equal to Max."),
			TEXT("RoomWidth")
		));
	}

	if (structure.RoomDepth.Min > structure.RoomDepth.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomDepthInvalid", "RoomDepth.Min cannot be greater than RoomDepth.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomDepthInvalidHint", "Set RoomDepth Min to be less than or equal to Max."),
			TEXT("RoomDepth")
		));
	}

	if (structure.RoomHeight.Min > structure.RoomHeight.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomHeightInvalid", "RoomHeight.Min cannot be greater than RoomHeight.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomHeightInvalidHint", "Set RoomHeight Min to be less than or equal to Max."),
			TEXT("RoomHeight")
		));
	}

	if (structure.RoomCountRange.Max <= 0 || structure.RoomCountRange.Min > structure.RoomCountRange.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountInvalid", "NumberOfCandidateRooms must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountInvalidHint", "Set Number Of Candidate Rooms to 1 or more."),
			TEXT("NumberOfCandidateRooms")
		));
	}

	if (structure.RoomCountRange.Max < 5)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_ATTEMPTS_LOW"),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountLow", "NumberOfCandidateRooms is very low and generation may fail frequently."),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountLowHint", "Increase Number Of Candidate Rooms to improve generation success rate."),
			TEXT("NumberOfCandidateRooms")
		));
	}

	if (params->IsUseMissionGraph() && path.ExtraCorridorComplexity > 0)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_CONSTRAINT"),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphAisle", "Path.ExtraCorridorComplexity is ignored while Keys And Locks progression is enabled."),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphAisleHint", "Keys And Locks uses a MissionGraph-safe route so locked doors cannot be bypassed."),
			TEXT("Path.ExtraCorridorComplexity")
		));
	}

	if (params->IsUseMissionGraph() && path.LoopRouteDensity > 0.f)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_CONSTRAINT"),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphLoopDensity", "Path.LoopRouteDensity is ignored while Keys And Locks progression is enabled."),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphLoopDensityHint", "Keys And Locks currently disables unsafe loops so locked doors cannot be bypassed."),
			TEXT("Path.LoopRouteDensity")
		));
	}

	const int32 layoutCandidateCount = params->GetLayoutCandidateCount();
	if (layoutCandidateCount > 8)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_LAYOUT_COST"),
			NSLOCTEXT("DungeonParameterValidator", "LayoutCandidateHigh", "LayoutCandidateCount is high and may increase runtime generation cost."),
			NSLOCTEXT("DungeonParameterValidator", "LayoutCandidateHighHint", "Use a lower value for runtime generation, or keep high candidate counts for editor previews."),
			TEXT("Path.LayoutCandidateCount")
		));
	}

	TSet<EDungeonRoomGameplayRole> seenRoomRoles;
	for (const FDungeonRoomRoleProfile& profile : roomRoles.Roles)
	{
		if (seenRoomRoles.Contains(profile.Role))
		{
			outIssues.Emplace(MakeIssue(
				EDungeonValidationSeverity::Warning,
				TEXT("DG_ROOM_ROLE_DUPLICATE"),
				FText::Format(NSLOCTEXT("DungeonParameterValidator", "DuplicateRoomRoleProfile", "Gameplay.RoomRoles contains more than one profile for {0}. Only the first matching profile is used."), StaticEnum<EDungeonRoomGameplayRole>()->GetDisplayNameTextByValue(static_cast<int64>(profile.Role))),
				NSLOCTEXT("DungeonParameterValidator", "DuplicateRoomRoleProfileHint", "Keep one profile per room role to avoid confusing role weights and theme overrides."),
				TEXT("Gameplay.RoomRoles")
			));
		}
		seenRoomRoles.Add(profile.Role);
	}

	if (theme.DungeonRoomMeshPartsDatabase == nullptr)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_DB_MISSING"),
			NSLOCTEXT("DungeonParameterValidator", "RoomDbMissing", "DungeonRoomMeshPartsDatabase is not assigned."),
			NSLOCTEXT("DungeonParameterValidator", "RoomDbMissingHint", "Assign a valid room mesh database."),
			TEXT("DungeonRoomMeshPartsDatabase")
		));
	}

	if (theme.DungeonAisleMeshPartsDatabase == nullptr)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_DB_MISSING"),
			NSLOCTEXT("DungeonParameterValidator", "AisleDbMissing", "DungeonAisleMeshPartsDatabase is not assigned."),
			NSLOCTEXT("DungeonParameterValidator", "AisleDbMissingHint", "Assign a valid aisle mesh database."),
			TEXT("DungeonAisleMeshPartsDatabase")
		));
	}

	if (theme.DungeonRoomMeshPartsDatabase)
	{
		if (!HasAnyStaticMesh(theme.DungeonRoomMeshPartsDatabase, 0))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomFloorMissing", "Room floor mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomFloorMissingHint", "Set at least one floor mesh in the room mesh database."), TEXT("Theme.DungeonRoomMeshPartsDatabase"), FSoftObjectPath(theme.DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonRoomMeshPartsDatabase, 1))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomWallMissing", "Room wall mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomWallMissingHint", "Set at least one wall mesh in the room mesh database."), TEXT("Theme.DungeonRoomMeshPartsDatabase"), FSoftObjectPath(theme.DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonRoomMeshPartsDatabase, 2))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomRoofMissing", "Room roof mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomRoofMissingHint", "Set at least one roof mesh in the room mesh database."), TEXT("Theme.DungeonRoomMeshPartsDatabase"), FSoftObjectPath(theme.DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonRoomMeshPartsDatabase, 3))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Warning, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomSlopeMissing", "Room slope/stairs mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomSlopeMissingHint", "Set slope mesh if your generation settings can create slopes."), TEXT("Theme.DungeonRoomMeshPartsDatabase"), FSoftObjectPath(theme.DungeonRoomMeshPartsDatabase)));
		}
	}

	if (theme.DungeonAisleMeshPartsDatabase)
	{
		if (!HasAnyStaticMesh(theme.DungeonAisleMeshPartsDatabase, 0))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleFloorMissing", "Aisle floor mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleFloorMissingHint", "Set at least one floor mesh in the aisle mesh database."), TEXT("Theme.DungeonAisleMeshPartsDatabase"), FSoftObjectPath(theme.DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonAisleMeshPartsDatabase, 1))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleWallMissing", "Aisle wall mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleWallMissingHint", "Set at least one wall mesh in the aisle mesh database."), TEXT("Theme.DungeonAisleMeshPartsDatabase"), FSoftObjectPath(theme.DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonAisleMeshPartsDatabase, 2))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleRoofMissing", "Aisle roof mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleRoofMissingHint", "Set at least one roof mesh in the aisle mesh database."), TEXT("Theme.DungeonAisleMeshPartsDatabase"), FSoftObjectPath(theme.DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(theme.DungeonAisleMeshPartsDatabase, 3))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Warning, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleSlopeMissing", "Aisle slope/stairs mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleSlopeMissingHint", "Set slope mesh if your generation settings can create slopes."), TEXT("Theme.DungeonAisleMeshPartsDatabase"), FSoftObjectPath(theme.DungeonAisleMeshPartsDatabase)));
		}
	}

	if (bDeepCheck)
	{
		ValidateAssetPath(FSoftObjectPath(theme.DungeonRoomMeshPartsDatabase), TEXT("Theme.DungeonRoomMeshPartsDatabase"), outIssues);
		ValidateAssetPath(FSoftObjectPath(theme.DungeonAisleMeshPartsDatabase), TEXT("Theme.DungeonAisleMeshPartsDatabase"), outIssues);
		ValidateFixtureAssets(theme.Fixtures, TEXT("Theme.Fixtures"), outIssues);
		ValidateAssetPath(FSoftObjectPath(gameplay.DungeonRoomSensorClass), TEXT("Gameplay.DungeonRoomSensorClass"), outIssues);
		for (const FSoftObjectPath& actorPath : gameplay.SpawnActorInAisle)
		{
			ValidateAssetPath(actorPath, TEXT("Gameplay.SpawnActorInAisle"), outIssues);
		}
		for (const FDungeonRoomRoleProfile& profile : roomRoles.Roles)
		{
			ValidateAssetPath(FSoftObjectPath(profile.GameplayOverride.DungeonRoomSensorClass), TEXT("Gameplay.RoomRoles.GameplayOverride.DungeonRoomSensorClass"), outIssues);
			if (profile.ThemeOverride.bOverrideFixtures)
			{
				ValidateFixtureAssets(profile.ThemeOverride.Fixtures, TEXT("Gameplay.RoomRoles.ThemeOverride.Fixtures"), outIssues);
			}
		}
		for (const FDungeonZoneDefinition& zone : zones.Zones)
		{
			ValidateAssetPath(FSoftObjectPath(zone.GameplayOverride.DungeonRoomSensorClass), TEXT("Zones.GameplayOverride.DungeonRoomSensorClass"), outIssues);
			if (zone.ThemeOverride.bOverrideFixtures)
			{
				ValidateFixtureAssets(zone.ThemeOverride.Fixtures, TEXT("Zones.ThemeOverride.Fixtures"), outIssues);
			}
			for (const FSoftObjectPath& actorPath : zone.GameplayOverride.SpawnActorInAisle)
			{
				ValidateAssetPath(actorPath, TEXT("Zones.GameplayOverride.SpawnActorInAisle"), outIssues);
			}
		}
	}
}
