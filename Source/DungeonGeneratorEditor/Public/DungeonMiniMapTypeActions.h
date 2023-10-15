/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonMiniMapTypeActions : public FAssetTypeActions_Base
{
public:
	FDungeonMiniMapTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonMiniMapTypeActions() = default;

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

inline FDungeonMiniMapTypeActions::FDungeonMiniMapTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

inline FColor FDungeonMiniMapTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

inline void FDungeonMiniMapTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

inline FText FDungeonMiniMapTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonMiniMap"));
}

inline uint32 FDungeonMiniMapTypeActions::GetCategories()
{
	return mAssetCategory;
}
