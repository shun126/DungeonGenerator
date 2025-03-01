/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonAisleMeshSetDatabaseTypeActions.h"
#include "Parameter/DungeonAisleMeshSetDatabase.h"

FDungeonAisleMeshSetDatabaseTypeActions::FDungeonAisleMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FText FDungeonAisleMeshSetDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("Mesh set database (Aisle)[deprecated]"));
}

UClass* FDungeonAisleMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonAisleMeshSetDatabase::StaticClass();
}

uint32 FDungeonAisleMeshSetDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}

FColor FDungeonAisleMeshSetDatabaseTypeActions::GetTypeColor() const
{
	static constexpr FColor Color(56, 56, 156);
	return Color;
}
