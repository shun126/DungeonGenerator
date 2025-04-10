/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonMeshSetDatabase.h"
#if WITH_EDITOR
#include "Parameter/DungeonAisleMeshSetDatabase.h"
#include "Parameter/DungeonRoomMeshSetDatabase.h"
#endif
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include <cmath>

#if WITH_EDITOR
#include "Helper/DungeonDebugUtility.h"
#endif

UDungeonMeshSetDatabase::UDungeonMeshSetDatabase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

const FDungeonMeshSet* UDungeonMeshSetDatabase::AtImplement(const size_t index) const
{
	const int32 size = Parts.Num();
	return (size > 0) ? &Parts[index % size] : nullptr;
}

const FDungeonMeshSet* UDungeonMeshSetDatabase::SelectImplement(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const
{
	const int32 size = Parts.Num();
	if (size <= 0)
		return nullptr;

	switch (SelectionMethod)
	{
	case EDungeonMeshSetSelectionMethod::Random:
		return &Parts[random->Get<uint32_t>(size)];

	case EDungeonMeshSetSelectionMethod::Identifier:
		return &Parts[identifier % size];

	case EDungeonMeshSetSelectionMethod::DepthFromStart:
	{
		const float ratio = static_cast<float>(depthRatioFromStart) / 255.f;
		const float index = static_cast<float>(size - 1) * ratio;
		return &Parts[static_cast<size_t>(std::round(index))];
	}

	default:
		DUNGEON_GENERATOR_ERROR(TEXT("Set the correct SelectionMethod"));
		return nullptr;
	}
}

#if WITH_EDITOR
FString UDungeonMeshSetDatabase::DumpToJson(const uint32 indent) const
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

void UDungeonMeshSetDatabase::Migrate()
{
	if (IsValid(MigrationMeshSetDatabase) == false)
		return;

	Parts.Reset();

	if (const auto* dungeonAisleMeshSetDatabase = Cast<UDungeonAisleMeshSetDatabase>(MigrationMeshSetDatabase))
	{
		for (const auto& parts : dungeonAisleMeshSetDatabase->Parts)
		{
			FDungeonMeshSet meshSet = MigrateRoomMeshSet(parts);
			meshSet.SloopPartsSelectionMethod = parts.SloopPartsSelectionMethod;
			meshSet.SlopeParts = parts.SlopeParts;
			Parts.Add(meshSet);
		}
	}
	else if (const auto* dungeonRoomMeshSetDatabase = Cast<UDungeonRoomMeshSetDatabase>(MigrationMeshSetDatabase))
	{
		for (const auto& parts : dungeonRoomMeshSetDatabase->Parts)
		{
			Parts.Add(MigrateRoomMeshSet(parts));
		}
	}
}

FDungeonMeshSet UDungeonMeshSetDatabase::MigrateRoomMeshSet(const FDungeonRoomMeshSet& dungeonRoomMeshSet)
{
	FDungeonMeshSet meshSet;
	meshSet.FloorPartsSelectionMethod = dungeonRoomMeshSet.FloorPartsSelectionMethod;
	meshSet.FloorParts = dungeonRoomMeshSet.FloorParts;
	meshSet.WallPartsSelectionMethod = dungeonRoomMeshSet.WallPartsSelectionMethod;
	meshSet.WallParts = dungeonRoomMeshSet.WallParts;
	meshSet.RoofPartsSelectionMethod = dungeonRoomMeshSet.RoofPartsSelectionMethod;
	meshSet.RoofParts = dungeonRoomMeshSet.RoofParts;
	return meshSet;
}
#endif
