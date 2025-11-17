/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Input/Reply.h>
#include <Modules/ModuleManager.h>
#include <Widgets/Docking/SDockTab.h>
#include <Widgets/Input/SEditableTextBox.h>

// Forward declaration
class ADungeonGeneratedActor;
class FMenuBuilder;
class FUICommandList;
class FSpawnTabArgs;
class FToolBarBuilder;
class SButton;
class UDungeonGenerateParameter;
class UStaticMesh;
class UTexture2D;
struct FAssetData;

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
	TWeakObjectPtr<ADungeonGeneratedActor> mDungeonActor;
	TWeakObjectPtr<UDungeonGenerateParameter> mDungeonGenerateParameter;
	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<SEditableTextBox> mRandomSeedValue;
	TSharedPtr<SButton> mGenerateDungeonButton;
};
