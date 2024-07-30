/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonRandomActorParts.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

#if WITH_EDITOR
FString FDungeonRandomActorParts::DumpToJson(const uint32 indent) const
{
	FString json = Super::DumpToJson(indent) + TEXT(",\n");
	json += dungeon::Indent(indent) + TEXT("\"Frequency\":") + FString::SanitizeFloat(Frequency);
	return json;
}
#endif
