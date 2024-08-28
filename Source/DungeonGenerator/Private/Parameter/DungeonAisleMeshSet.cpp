/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonAisleMeshSet.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

#if WITH_EDITOR
FString FDungeonAisleMeshSet::DumpToJson(const uint32 indent) const
{
	FString json = Super::DumpToJson(indent) + TEXT(",\n");
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
