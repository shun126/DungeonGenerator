/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGenerateParameterTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonGenerateParameter.h"

UDungeonGenerateParameterTypeActions::UDungeonGenerateParameterTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: MyAssetCategory(InAssetCategory)
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
	return FText::FromName(TEXT("DungeonGenerateParameter"));
}

UClass* UDungeonGenerateParameterTypeActions::GetSupportedClass() const
{
	return UDungeonGenerateParameter::StaticClass();
}

uint32 UDungeonGenerateParameterTypeActions::GetCategories()
{
	return MyAssetCategory;
}
