/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonActorParts.h"
#include "Parameter/DungeonPartsSelector.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonSelectionPolicyUtility.h"
#include "Core/Debug/Debug.h"
#include "Core/Helper/Direction.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif


const FDungeonMeshPartsWithDirection* FDungeonMeshSet::SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridIndex, grid, random, FloorParts, FloorPartsSelectionPolicy);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectWallPartsByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridIndex, grid, random, WallParts, WallPartsSelectionPolicy);
}

const FDungeonMeshPartsWithDirection* FDungeonMeshSet::SelectRoofParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridIndex, grid, random, RoofParts, RoofPartsSelectionPolicy);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridIndex, grid, random, SlopeParts, SlopePartsSelectionPolicy);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectCatwalkParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridIndex, grid, random, CatwalkParts, CatwalkPartsSelectionPolicy);
}

const FDungeonRandomActorParts* FDungeonMeshSet::SelectChandelierParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectRandomActorParts(gridIndex, grid, random, ChandelierParts, ChandelierPartsSelectionPolicy);
}

void FDungeonMeshSet::MigrateSelectionPolicies()
{
	if (!bSelectionPoliciesMigrated)
	{
		FloorPartsSelectionPolicy = dungeon::selection::ToPolicy(FloorPartsSelectionMethod);
		WallPartsSelectionPolicy = dungeon::selection::ToPolicy(WallPartsSelectionMethod);
		RoofPartsSelectionPolicy = dungeon::selection::ToPolicy(RoofPartsSelectionMethod);
		SlopePartsSelectionPolicy = dungeon::selection::ToPolicy(SloopPartsSelectionMethod);
		ChandelierPartsSelectionPolicy = dungeon::selection::ToPolicy(ChandelierPartsSelectionMethod);
		CatwalkPartsSelectionPolicy = dungeon::selection::ToPolicy(CatwalkPartsSelectionMethod);
	}

	FloorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(FloorPartsSelectionPolicy);
	FloorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(FloorPartsSelectionPolicy);

	WallPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(WallPartsSelectionPolicy);
	WallPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(WallPartsSelectionPolicy);

	RoofPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(RoofPartsSelectionPolicy);
	RoofPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(RoofPartsSelectionPolicy);

	SlopePartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(SlopePartsSelectionPolicy);
	SloopPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(SlopePartsSelectionPolicy);

	ChandelierPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(ChandelierPartsSelectionPolicy);
	ChandelierPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(ChandelierPartsSelectionPolicy);

	CatwalkPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(CatwalkPartsSelectionPolicy);
	CatwalkPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(CatwalkPartsSelectionPolicy);

	bSelectionPoliciesMigrated = true;
}

int32 FDungeonMeshSet::SelectDungeonMeshPartsIndexByGrid(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonSelectionPolicy selectionPolicy)
{
	int32 partsIndex = gridIndex % size;
	switch (dungeon::selection::SanitizePartsPolicy(selectionPolicy))
	{
	case EDungeonSelectionPolicy::Random:
		if (random != nullptr)
			partsIndex = random->Get<int32_t>(size);
		break;

	case EDungeonSelectionPolicy::Direction:
		partsIndex = grid.GetDirection().Get() % size;
		break;

	case EDungeonSelectionPolicy::Identifier:
		partsIndex = grid.GetIdentifier() % size;
		break;

	case EDungeonSelectionPolicy::DepthFromStart:
	{
		const float ratio = static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f;
		const float index = static_cast<float>(size - 1) * ratio;
		partsIndex = FMath::RoundToInt(index);
		break;
	}

	case EDungeonSelectionPolicy::GridIndex:
	default:
		break;
	}

	check(partsIndex >= 0);

	return partsIndex;
}

int32 FDungeonMeshSet::SelectDungeonMeshPartsIndexByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const int32 size)
{
	const int32 offset = gridLocation.Z & 1;
	if (direction.IsNorthSouth())
		return (gridLocation.X + offset) % size;
	return (gridLocation.Y + offset) % size;
}

FDungeonActorParts* FDungeonMeshSet::SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonSelectionPolicy selectionPolicy)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndexByGrid(gridIndex, grid, random, size, selectionPolicy);
	FDungeonActorParts* actorParts = const_cast<FDungeonActorParts*>(&parts[index]);
	return IsValid(actorParts->ActorClass) ? actorParts : nullptr;
}

FDungeonRandomActorParts* FDungeonMeshSet::SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonSelectionPolicy selectionPolicy)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndexByGrid(gridIndex, grid, random, size, selectionPolicy);
	FDungeonRandomActorParts* actorParts = const_cast<FDungeonRandomActorParts*>(&parts[index]);
	if (!IsValid(actorParts->ActorClass))
		return nullptr;

	if (random != nullptr)
	{
		const float value = random->Get<float>();
		if (value > actorParts->Frequency)
			return nullptr;
	}

	return actorParts;
}

#if WITH_EDITOR
FString FDungeonMeshSet::DumpToJson(const uint32 indent) const
{
	FString json;

	json += dungeon::Indent(indent) + TEXT("\"FloorParts\":{\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"FloorPartsSelectionMethod\":\"") + UEnum::GetValueAsString(FloorPartsSelectionMethod) + TEXT("\",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < FloorParts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",\n");
		json += dungeon::Indent(indent + 2) + TEXT("{\n");
		json += FloorParts[i].DumpToJson(indent + 3) + TEXT("\n");
		json += dungeon::Indent(indent + 2) + TEXT("}");
	}
	json += TEXT("\n");
	json += dungeon::Indent(indent + 1) + TEXT("]\n");
	json += dungeon::Indent(indent) + TEXT("},\n");

	json += dungeon::Indent(indent) + TEXT("\"WallParts\":{\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"WallPartsSelectionMethod\":\"") + UEnum::GetValueAsString(WallPartsSelectionMethod) + TEXT("\",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < WallParts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",\n");
		json += dungeon::Indent(indent + 2) + TEXT("{\n");
		json += WallParts[i].DumpToJson(indent + 3) + TEXT("\n");
		json += dungeon::Indent(indent + 2) + TEXT("}");
	}
	json += TEXT("\n");
	json += dungeon::Indent(indent + 1) + TEXT("]\n");
	json += dungeon::Indent(indent) + TEXT("},\n");

	json += dungeon::Indent(indent) + TEXT("\"RoofParts\":{\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"RoofPartsSelectionMethod\":\"") + UEnum::GetValueAsString(RoofPartsSelectionMethod) + TEXT("\",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < RoofParts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",\n");
		json += dungeon::Indent(indent + 2) + TEXT("{\n");
		json += RoofParts[i].DumpToJson(indent + 3) + TEXT("\n");
		json += dungeon::Indent(indent + 2) + TEXT("}");
	}
	json += TEXT("\n");
	json += dungeon::Indent(indent + 1) + TEXT("]\n");
	json += dungeon::Indent(indent) + TEXT("},\n");

	json += dungeon::Indent(indent) + TEXT("\"SlopeParts\":{\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"SloopPartsSelectionMethod\":\"") + UEnum::GetValueAsString(SloopPartsSelectionMethod) + TEXT("\",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < SlopeParts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",\n");
		json += dungeon::Indent(indent + 2) + TEXT("{\n");
		json += SlopeParts[i].DumpToJson(indent + 3) + TEXT("\n");
		json += dungeon::Indent(indent + 2) + TEXT("}");
	}
	json += TEXT("\n");
	json += dungeon::Indent(indent + 1) + TEXT("]\n");
	json += dungeon::Indent(indent) + TEXT("}");
	json += TEXT("\n");
	json += dungeon::Indent(indent) + TEXT("},\n");

	json += dungeon::Indent(indent) + TEXT("\"ChandelierParts\":{\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"ChandelierPartsSelectionMethod\":\"") + UEnum::GetValueAsString(ChandelierPartsSelectionMethod) + TEXT("\",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < ChandelierParts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",\n");
		json += dungeon::Indent(indent + 2) + TEXT("{\n");
		json += ChandelierParts[i].DumpToJson(indent + 3) + TEXT("\n");
		json += dungeon::Indent(indent + 2) + TEXT("}");
	}
	json += TEXT("\n");
	json += dungeon::Indent(indent + 1) + TEXT("],\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"MinSpacing\":") + FString::SanitizeFloat(ChandelierMinSpacing) + TEXT(",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"MinCeilingHeight\":") + FString::SanitizeFloat(ChandelierMinCeilingHeight) + TEXT(",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"Radius\":") + FString::SanitizeFloat(ChandelierRadius) + TEXT(",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"WallWeight\":") + FString::SanitizeFloat(ChandelierWallWeight) + TEXT(",\n");
	json += dungeon::Indent(indent + 1) + TEXT("\"CombatWeight\":") + FString::SanitizeFloat(ChandelierCombatWeight) + TEXT("\n");
	json += dungeon::Indent(indent) + TEXT("}");

	return json;
}
#endif
