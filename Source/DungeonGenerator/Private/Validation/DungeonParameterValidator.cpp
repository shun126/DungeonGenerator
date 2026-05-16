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

	if (params->GridSize <= 0.f)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "GridSizeInvalid", "GridSize must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "GridSizeInvalidHint", "Set Horizontal Size to a positive value."),
			TEXT("GridSize")
		));
	}

	if (params->VerticalGridSize <= 0.f)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "VerticalGridSizeInvalid", "VerticalGridSize must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "VerticalGridSizeInvalidHint", "Set Vertical Size to a positive value."),
			TEXT("VerticalGridSize")
		));
	}

	if (params->RoomWidth.Min > params->RoomWidth.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomWidthInvalid", "RoomWidth.Min cannot be greater than RoomWidth.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomWidthInvalidHint", "Set RoomWidth Min to be less than or equal to Max."),
			TEXT("RoomWidth")
		));
	}

	if (params->RoomDepth.Min > params->RoomDepth.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomDepthInvalid", "RoomDepth.Min cannot be greater than RoomDepth.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomDepthInvalidHint", "Set RoomDepth Min to be less than or equal to Max."),
			TEXT("RoomDepth")
		));
	}

	if (params->RoomHeight.Min > params->RoomHeight.Max)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomHeightInvalid", "RoomHeight.Min cannot be greater than RoomHeight.Max."),
			NSLOCTEXT("DungeonParameterValidator", "RoomHeightInvalidHint", "Set RoomHeight Min to be less than or equal to Max."),
			TEXT("RoomHeight")
		));
	}

	if (params->NumberOfCandidateRooms <= 0)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_PARAM_RANGE"),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountInvalid", "NumberOfCandidateRooms must be greater than 0."),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountInvalidHint", "Set Number Of Candidate Rooms to 1 or more."),
			TEXT("NumberOfCandidateRooms")
		));
	}

	if (params->NumberOfCandidateRooms < 5)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_ATTEMPTS_LOW"),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountLow", "NumberOfCandidateRooms is very low and generation may fail frequently."),
			NSLOCTEXT("DungeonParameterValidator", "RoomCountLowHint", "Increase Number Of Candidate Rooms to improve generation success rate."),
			TEXT("NumberOfCandidateRooms")
		));
	}

	if (params->MergeRooms && params->UseMissionGraph)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_CONSTRAINT"),
			NSLOCTEXT("DungeonParameterValidator", "MergeMissionConflict", "UseMissionGraph is not effective while MergeRooms is enabled."),
			NSLOCTEXT("DungeonParameterValidator", "MergeMissionConflictHint", "Disable MergeRooms or disable UseMissionGraph."),
			TEXT("UseMissionGraph")
		));
	}

	if (params->UseMissionGraph && params->AisleComplexity > 0)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Warning,
			TEXT("DG_PARAM_CONSTRAINT"),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphAisle", "AisleComplexity is ignored while UseMissionGraph is enabled."),
			NSLOCTEXT("DungeonParameterValidator", "MissionGraphAisleHint", "Set AisleComplexity to 0 when UseMissionGraph is enabled."),
			TEXT("AisleComplexity")
		));
	}

	if (params->DungeonRoomMeshPartsDatabase == nullptr)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_DB_MISSING"),
			NSLOCTEXT("DungeonParameterValidator", "RoomDbMissing", "DungeonRoomMeshPartsDatabase is not assigned."),
			NSLOCTEXT("DungeonParameterValidator", "RoomDbMissingHint", "Assign a valid room mesh database."),
			TEXT("DungeonRoomMeshPartsDatabase")
		));
	}

	if (params->DungeonAisleMeshPartsDatabase == nullptr)
	{
		outIssues.Emplace(MakeIssue(
			EDungeonValidationSeverity::Error,
			TEXT("DG_DB_MISSING"),
			NSLOCTEXT("DungeonParameterValidator", "AisleDbMissing", "DungeonAisleMeshPartsDatabase is not assigned."),
			NSLOCTEXT("DungeonParameterValidator", "AisleDbMissingHint", "Assign a valid aisle mesh database."),
			TEXT("DungeonAisleMeshPartsDatabase")
		));
	}

	if (params->DungeonRoomMeshPartsDatabase)
	{
		if (!HasAnyStaticMesh(params->DungeonRoomMeshPartsDatabase, 0))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomFloorMissing", "Room floor mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomFloorMissingHint", "Set at least one floor mesh in the room mesh database."), TEXT("DungeonRoomMeshPartsDatabase"), FSoftObjectPath(params->DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonRoomMeshPartsDatabase, 1))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomWallMissing", "Room wall mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomWallMissingHint", "Set at least one wall mesh in the room mesh database."), TEXT("DungeonRoomMeshPartsDatabase"), FSoftObjectPath(params->DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonRoomMeshPartsDatabase, 2))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomRoofMissing", "Room roof mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomRoofMissingHint", "Set at least one roof mesh in the room mesh database."), TEXT("DungeonRoomMeshPartsDatabase"), FSoftObjectPath(params->DungeonRoomMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonRoomMeshPartsDatabase, 3))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Warning, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "RoomSlopeMissing", "Room slope/stairs mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "RoomSlopeMissingHint", "Set slope mesh if your generation settings can create slopes."), TEXT("DungeonRoomMeshPartsDatabase"), FSoftObjectPath(params->DungeonRoomMeshPartsDatabase)));
		}
	}

	if (params->DungeonAisleMeshPartsDatabase)
	{
		if (!HasAnyStaticMesh(params->DungeonAisleMeshPartsDatabase, 0))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleFloorMissing", "Aisle floor mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleFloorMissingHint", "Set at least one floor mesh in the aisle mesh database."), TEXT("DungeonAisleMeshPartsDatabase"), FSoftObjectPath(params->DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonAisleMeshPartsDatabase, 1))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleWallMissing", "Aisle wall mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleWallMissingHint", "Set at least one wall mesh in the aisle mesh database."), TEXT("DungeonAisleMeshPartsDatabase"), FSoftObjectPath(params->DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonAisleMeshPartsDatabase, 2))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Error, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleRoofMissing", "Aisle roof mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleRoofMissingHint", "Set at least one roof mesh in the aisle mesh database."), TEXT("DungeonAisleMeshPartsDatabase"), FSoftObjectPath(params->DungeonAisleMeshPartsDatabase)));
		}
		if (!HasAnyStaticMesh(params->DungeonAisleMeshPartsDatabase, 3))
		{
			outIssues.Emplace(MakeIssue(EDungeonValidationSeverity::Warning, TEXT("DG_MESH_MISSING"), NSLOCTEXT("DungeonParameterValidator", "AisleSlopeMissing", "Aisle slope/stairs mesh is not configured."), NSLOCTEXT("DungeonParameterValidator", "AisleSlopeMissingHint", "Set slope mesh if your generation settings can create slopes."), TEXT("DungeonAisleMeshPartsDatabase"), FSoftObjectPath(params->DungeonAisleMeshPartsDatabase)));
		}
	}

	if (bDeepCheck)
	{
		ValidateAssetPath(FSoftObjectPath(params->DungeonRoomMeshPartsDatabase), TEXT("DungeonRoomMeshPartsDatabase"), outIssues);
		ValidateAssetPath(FSoftObjectPath(params->DungeonAisleMeshPartsDatabase), TEXT("DungeonAisleMeshPartsDatabase"), outIssues);
		ValidateAssetPath(FSoftObjectPath(params->DungeonRoomSensorDatabase), TEXT("DungeonRoomSensorDatabase"), outIssues);
	}
}
