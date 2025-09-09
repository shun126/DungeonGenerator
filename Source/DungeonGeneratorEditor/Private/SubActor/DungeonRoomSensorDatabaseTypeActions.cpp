/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonRoomSensorDatabaseTypeActions.h"
#include "SubActor/DungeonRoomSensorDatabase.h"

FDungeonRoomSensorDatabaseTypeActions::FDungeonRoomSensorDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FText FDungeonRoomSensorDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("Room sensor database"));
}

UClass* FDungeonRoomSensorDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonRoomSensorDatabase::StaticClass();
}

uint32 FDungeonRoomSensorDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}

FColor FDungeonRoomSensorDatabaseTypeActions::GetTypeColor() const
{
	static constexpr FColor Color(56, 156, 156);
	return Color;
}
