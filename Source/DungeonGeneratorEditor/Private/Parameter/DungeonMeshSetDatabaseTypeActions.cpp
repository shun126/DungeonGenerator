/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonMeshSetDatabaseTypeActions.h"
#include "Parameter/DungeonMeshSetDatabase.h"

FDungeonMeshSetDatabaseTypeActions::FDungeonMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FText FDungeonMeshSetDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("Mesh set database"));
}

UClass* FDungeonMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonMeshSetDatabase::StaticClass();
}

uint32 FDungeonMeshSetDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}

FColor FDungeonMeshSetDatabaseTypeActions::GetTypeColor() const
{
	static constexpr FColor Color(56, 56, 156);
	return Color;
}
