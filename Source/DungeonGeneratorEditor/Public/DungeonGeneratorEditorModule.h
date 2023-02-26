/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Modules/ModuleManager.h>

// Forward declaration
class FToolBarBuilder;
class FMenuBuilder;
class UDungeonGenerateParameter;
class UStaticMesh;

class FDungeonGenerateEditorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// This function will be bound to Command (by default it will bring up plugin window)
	void PluginButtonClicked();

private:
	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	FReply OnClickedGenerateButton();
	FReply OnClickedClearButton();

	FString GetObjectPath() const;
	void SetAssetData(const FAssetData& assetData);

private:
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<SEditableTextBox> mRandomSeedValue;
	TWeakObjectPtr<UDungeonGenerateParameter> mDungeonGenerateParameter = nullptr;
};
