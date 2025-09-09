/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAssetTypeActionsBase.h"

class FDungeonRoomSensorDatabaseTypeActions : public FDungeonAssetTypeActionsBase
{
public:
	explicit FDungeonRoomSensorDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonRoomSensorDatabaseTypeActions() override = default;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
