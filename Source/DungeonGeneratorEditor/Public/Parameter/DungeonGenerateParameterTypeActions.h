/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAssetTypeActionsBase.h"

class UDungeonGenerateParameterTypeActions : public FDungeonAssetTypeActionsBase
{
public:
	explicit UDungeonGenerateParameterTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~UDungeonGenerateParameterTypeActions() override = default;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
