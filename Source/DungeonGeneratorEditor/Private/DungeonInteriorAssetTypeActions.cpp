/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorAssetTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonInteriorAsset.h"

FDungeonInteriorAssetTypeActions::FDungeonInteriorAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FColor FDungeonInteriorAssetTypeActions::GetTypeColor() const
{
	return FColor::Purple;
}

void FDungeonInteriorAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

FText FDungeonInteriorAssetTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonInteriorAsset"));
}

UClass* FDungeonInteriorAssetTypeActions::GetSupportedClass() const
{
	return UDungeonInteriorAsset::StaticClass();
}

uint32 FDungeonInteriorAssetTypeActions::GetCategories()
{
	return mAssetCategory;
}
