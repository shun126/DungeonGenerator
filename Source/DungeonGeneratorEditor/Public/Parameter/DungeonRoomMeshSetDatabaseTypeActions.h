/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAssetTypeActionsBase.h"

class FDungeonRoomMeshSetDatabaseTypeActions : public FDungeonAssetTypeActionsBase
{
public:
	explicit FDungeonRoomMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonRoomMeshSetDatabaseTypeActions() override = default;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual FColor GetTypeColor() const override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
