/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonActorParts.h"
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Core/Helper/Direction.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif



















int32 FDungeonMeshSet::SelectDungeonMeshPartsIndex(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod)
{
	switch (partsSelectionMethod)
	{
	case EDungeonPartsSelectionMethod::GridIndex:
		return gridIndex % size;

	case EDungeonPartsSelectionMethod::Direction:
		return grid.GetDirection().Get() % size;

	case EDungeonPartsSelectionMethod::Random:
		return random->Get<uint32_t>(size);

	default:
		return gridIndex % size;
	}
}

// aka: SelectParts, SelectRandomActorParts
FDungeonActorParts* FDungeonMeshSet::SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	FDungeonActorParts* actorParts = const_cast<FDungeonActorParts*>(&parts[index]);
	return IsValid(actorParts->ActorClass) ? actorParts : nullptr;
}

// aka: SelectParts, SelectActorParts
FDungeonRandomActorParts* FDungeonMeshSet::SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	FDungeonRandomActorParts* actorParts = const_cast<FDungeonRandomActorParts*>(&parts[index]);
	if (!IsValid(actorParts->ActorClass))
		return nullptr;

	const float value = random->Get<float>();
	if (value > actorParts->Frequency)
		return nullptr;

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
	json += dungeon::Indent(indent) + TEXT("}");

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

	return json;
}
#endif
