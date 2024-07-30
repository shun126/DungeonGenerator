/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonActorParts.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

#if WITH_EDITOR
FString FDungeonActorParts::DumpToJson(const uint32 indent) const
{
	FString json = Super::DumpToJson(indent);
	if (IsValid(ActorClass))
	{
		json += TEXT(",\n");
		json += dungeon::Indent(indent) + TEXT("\"ActorClass\":\"") + ActorClass->GetName() + TEXT("\"");
	}
	return json;
}
#endif
