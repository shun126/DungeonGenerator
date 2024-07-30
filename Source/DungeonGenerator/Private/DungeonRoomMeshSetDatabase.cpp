/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomMeshSetDatabase.h"
#include "Core/Math/Random.h"

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

const FDungeonMeshSet* UDungeonRoomMeshSetDatabase::AtImplement(const size_t index) const
{
	const int32 size = Parts.Num();
	return (size > 0) ? &Parts[index % size] : nullptr;
}

const FDungeonMeshSet* UDungeonRoomMeshSetDatabase::SelectImplement(const std::shared_ptr<dungeon::Random>& random) const
{
	const int32 size = Parts.Num();
	if (size <= 0)
		return nullptr;

	const uint32_t index = random->Get<uint32_t>(size);
	return &Parts[index];
}

#if WITH_EDITOR
FString UDungeonRoomMeshSetDatabase::DumpToJson(const uint32 indent) const
{
	FString json = dungeon::Indent(indent) + TEXT("\"Parts\":[\n");
	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		if (i != 0)
			json += TEXT(",");
		json += dungeon::Indent(indent + 1) + TEXT("{\n");
		json += Parts[i].DumpToJson(indent + 2);
		json += TEXT("\n");
		json += dungeon::Indent(indent + 1) + TEXT("}\n");
	}
	json += dungeon::Indent(indent) + TEXT("]");
	return json;
}
#endif
