/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAssetTypeActionsBase.h"

class FDungeonMeshSetDatabaseTypeActions : public FDungeonAssetTypeActionsBase
{
public:
	explicit FDungeonMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonMeshSetDatabaseTypeActions() override = default;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
