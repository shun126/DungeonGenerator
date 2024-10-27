/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Modules/ModuleManager.h>

// Forward declaration
class ADungeonActor;
class UDungeonGenerateParameter;
class FToolBarBuilder;
class FMenuBuilder;
class UStaticMesh;

class FDungeonGenerateEditorModule : public IModuleInterface
{
public:
	FDungeonGenerateEditorModule() = default;
	virtual ~FDungeonGenerateEditorModule() override = default;

	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// This function will be bound to Command (by default it will bring up plugin window)
	void PluginButtonClicked();

private:
	void RegisterMenus();

	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	FString GetObjectPath() const;
	void SetAssetData(const FAssetData& assetData);

	FReply OnClickedGenerateButton();
	FReply OnClickedClearButton();

	void DisposeDungeon(UWorld* world, const bool flushLevelStreaming);


	static UWorld* GetWorldFromGameViewport();

private:
	TWeakObjectPtr<ADungeonActor> mDungeonActor;
	TWeakObjectPtr<UDungeonGenerateParameter> mDungeonGenerateParameter;
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<SEditableTextBox> mRandomSeedValue;
	TSharedPtr<SButton> mGenerateDungeonButton;
};
