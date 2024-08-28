/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonAisleMeshSetDatabaseTypeActions : public FAssetTypeActions_Base
{
public:
	explicit FDungeonAisleMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonAisleMeshSetDatabaseTypeActions() override = default;

	// FAssetTypeActions_Base overrides
	virtual FColor GetTypeColor() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;

private:
	EAssetTypeCategories::Type mAssetCategory;
};
