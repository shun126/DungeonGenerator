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
