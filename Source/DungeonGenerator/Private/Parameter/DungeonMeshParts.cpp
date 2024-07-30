/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonMeshParts.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

#if WITH_EDITOR
FString FDungeonMeshParts::DumpToJson(const uint32 indent) const
{
	FString json = Super::DumpToJson(indent);
	if (IsValid(StaticMesh))
	{
		json += TEXT(",\n");
		json += dungeon::Indent(indent) + TEXT("\"StaticMesh\":\"") + StaticMesh->GetName() + TEXT("\"");
	}
	return json;
}
#endif
