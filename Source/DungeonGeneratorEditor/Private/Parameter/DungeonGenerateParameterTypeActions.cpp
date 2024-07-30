/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonGenerateParameterTypeActions.h"
#include "Parameter/DungeonGenerateParameter.h"

UDungeonGenerateParameterTypeActions::UDungeonGenerateParameterTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

FColor UDungeonGenerateParameterTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

void UDungeonGenerateParameterTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

FText UDungeonGenerateParameterTypeActions::GetName() const
{
	return FText::FromName(TEXT("Generate parameter"));
}

UClass* UDungeonGenerateParameterTypeActions::GetSupportedClass() const
{
	return UDungeonGenerateParameter::StaticClass();
}

uint32 UDungeonGenerateParameterTypeActions::GetCategories()
{
	return mAssetCategory;
}
