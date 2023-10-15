/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonRoomDatabaseTypeActions : public FAssetTypeActions_Base
{
public:
	FDungeonRoomDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonRoomDatabaseTypeActions() = default;

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

inline FDungeonRoomDatabaseTypeActions::FDungeonRoomDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

inline FColor FDungeonRoomDatabaseTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

inline void FDungeonRoomDatabaseTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

inline FText FDungeonRoomDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonRoomDatabase"));
}

inline uint32 FDungeonRoomDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}
