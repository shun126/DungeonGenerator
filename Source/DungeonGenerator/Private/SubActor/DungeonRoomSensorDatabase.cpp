/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonRoomSensorDatabase.h"
#include "Core/Math/Random.h"
#include "Core/Debug/Debug.h"
#include "Parameter/DungeonMeshSetDatabase.h"
#include <cmath>

UDungeonRoomSensorDatabase::UDungeonRoomSensorDatabase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UClass* UDungeonRoomSensorDatabase::Select(const uint16_t identifier, const uint8_t depthRatioFromStart, const std::shared_ptr<dungeon::Random>& random) const
{
	TArray<UClass*> roomSensors;
	roomSensors.Reserve(DungeonRoomSensorClass.Num());
	for (const auto& dungeonRoomSensorClass : DungeonRoomSensorClass)
	{
		if (IsValid(dungeonRoomSensorClass))
			roomSensors.Add(dungeonRoomSensorClass);
	}
	if (roomSensors.Num() <= 0)
		return nullptr;

	// 抽選
	switch (SelectionMethod)
	{
	case EDungeonMeshSetSelectionMethod::Random:
	{
		const auto index = random->Get(roomSensors.Num());
		return roomSensors[index];
	}
	case EDungeonMeshSetSelectionMethod::Identifier:
	{
		const auto index = identifier % roomSensors.Num();
		return roomSensors[index];
	}
	case EDungeonMeshSetSelectionMethod::DepthFromStart:
	{
		const float ratio = static_cast<float>(depthRatioFromStart) / 255.f;
		const float index = static_cast<float>(roomSensors.Num() - 1) * ratio;
		return roomSensors[static_cast<size_t>(std::round(index))];
	}
	default:
		DUNGEON_GENERATOR_ERROR(TEXT("Set the correct SelectionMethod"));
		return nullptr;
	}
}
