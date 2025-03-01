/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAssetTypeActionsBase.h"

class FDungeonAisleMeshSetDatabaseTypeActions : public FDungeonAssetTypeActionsBase
{
public:
	explicit FDungeonAisleMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonAisleMeshSetDatabaseTypeActions() override = default;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
