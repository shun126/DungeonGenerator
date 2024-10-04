/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <AssetTypeActions_Base.h>

class FDungeonAssetTypeActionsBase : public FAssetTypeActions_Base
{
public:
    // Constructor
    explicit FDungeonAssetTypeActionsBase() = default;

    // Destructor
	virtual ~FDungeonAssetTypeActionsBase() override = default;

	// FAssetTypeActions_Base overrides
	virtual FColor GetTypeColor() const override
    {
        static constexpr FColor Color(156, 156, 56);
        return Color;
    }

    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) override
    {
        FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
    }
};
