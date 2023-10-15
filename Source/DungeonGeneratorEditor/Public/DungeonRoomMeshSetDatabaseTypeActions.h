/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonRoomMeshSetDatabaseTypeActions : public FAssetTypeActions_Base
{
public:
	FDungeonRoomMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~FDungeonRoomMeshSetDatabaseTypeActions() = default;

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

inline FDungeonRoomMeshSetDatabaseTypeActions::FDungeonRoomMeshSetDatabaseTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

inline FColor FDungeonRoomMeshSetDatabaseTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

inline void FDungeonRoomMeshSetDatabaseTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

inline FText FDungeonRoomMeshSetDatabaseTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonRoomMeshSetDatabase"));
}

inline uint32 FDungeonRoomMeshSetDatabaseTypeActions::GetCategories()
{
	return mAssetCategory;
}
