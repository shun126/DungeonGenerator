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

FColor FDungeonAisleMeshSetDatabaseTypeActions::GetTypeColor() const
{
	return FColor::Cyan;
}

void FDungeonAisleMeshSetDatabaseTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

FText FDungeonAisleMeshSetDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("Mesh set database (Aisle)"));
}

UClass* FDungeonAisleMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonAisleMeshSetDatabase::StaticClass();
}

uint32 FDungeonAisleMeshSetDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}
