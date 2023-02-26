/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomAssetTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonRoomAsset.h"

FDungeonRoomAssetTypeActions::FDungeonRoomAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
{
}

FColor FDungeonRoomAssetTypeActions::GetTypeColor() const
{
	return FColor::Orange;
}

void FDungeonRoomAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

FText FDungeonRoomAssetTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonRoomAsset"));
}

UClass* FDungeonRoomAssetTypeActions::GetSupportedClass() const
{
	return UDungeonRoomAsset::StaticClass();
}

uint32 FDungeonRoomAssetTypeActions::GetCategories()
{
	return MyAssetCategory;
}
