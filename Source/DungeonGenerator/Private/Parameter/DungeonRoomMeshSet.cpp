/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonRoomMeshSet.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

#if WITH_EDITOR
FString FDungeonRoomMeshSet::DumpToJson(const uint32 indent) const
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

	return json;
}
#endif
