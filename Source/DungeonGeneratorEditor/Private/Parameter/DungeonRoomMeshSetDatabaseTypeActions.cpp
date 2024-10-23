/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonRoomMeshSetDatabaseTypeActions.h"
#include "Parameter/DungeonRoomMeshSetDatabase.h"

FDungeonRoomMeshSetDatabaseTypeActions::FDungeonRoomMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FText FDungeonRoomMeshSetDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("Mesh set database (Room)[deprecated]"));
}

UClass* FDungeonRoomMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonRoomMeshSetDatabase::StaticClass();
}

uint32 FDungeonRoomMeshSetDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}
