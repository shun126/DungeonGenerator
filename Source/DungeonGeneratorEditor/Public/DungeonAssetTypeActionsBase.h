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
    virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) override
    {
        FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
    }
};
