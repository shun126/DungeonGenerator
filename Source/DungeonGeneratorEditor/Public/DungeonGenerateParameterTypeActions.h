/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class UDungeonGenerateParameterTypeActions : public FAssetTypeActions_Base
{
public:
	UDungeonGenerateParameterTypeActions(EAssetTypeCategories::Type InAssetCategory);
	virtual ~UDungeonGenerateParameterTypeActions() = default;

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

inline UDungeonGenerateParameterTypeActions::UDungeonGenerateParameterTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: mAssetCategory(InAssetCategory)
{
}

inline FColor UDungeonGenerateParameterTypeActions::GetTypeColor() const
{
	return FColor::Emerald;
}

inline void UDungeonGenerateParameterTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}

inline FText UDungeonGenerateParameterTypeActions::GetName() const
{
	return FText::FromName(TEXT("DungeonGenerateParameter"));
}

inline uint32 UDungeonGenerateParameterTypeActions::GetCategories()
{
	return mAssetCategory;
}
