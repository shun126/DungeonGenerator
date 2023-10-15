/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonInteriorDatabaseTypeActions : public FAssetTypeActions_Base
{
public:
	explicit FDungeonInteriorDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonInteriorDatabaseTypeActions() = default;

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

inline FDungeonInteriorDatabaseTypeActions::FDungeonInteriorDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

inline FColor FDungeonInteriorDatabaseTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

inline void FDungeonInteriorDatabaseTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

inline FText FDungeonInteriorDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonInteriorDatabase"));
}

inline uint32 FDungeonInteriorDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}
