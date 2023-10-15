/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Modules/ModuleManager.h>
#include <memory>

// Forward declaration
class FToolBarBuilder;
class FMenuBuilder;
class CDungeonGeneratorCore;
class UDungeonGenerateParameter;
class UDungeonMiniMap;
class UDungeonMiniMapTextureLayer;
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

	TSharedRef<SDockTab> OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs);

	FString GetObjectPath() const;
	void SetAssetData(const FAssetData& assetData);

	FReply OnClickedGenerateButton();
	FReply OnClickedClearButton();

	TOptional<int32> OnGetGenerateTextureSize() const;
	void OnSetGenerateTextureSize(int32 value);

	TOptional<int32> OnGetGenerateTextureScale() const;
	void OnSetGenerateTextureScale(int32 value);

	void UpdateGenerateTextureButtons();
	FReply OnClickedGenerateTextureWithSizeButton();
	FReply OnClickedGenerateTextureWithScaleButton();

	bool SavePackages(const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer);
	bool SaveLayerInfoPackage(const FString& basePath, const FString& worldName, const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer);
	bool SaveTexturePackage(UDungeonMiniMap* dungeonMiniMapLayerInfo, TArray<UPackage*>& packages, const FString& basePath, const FString& worldName, const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer);

	static UWorld* GetWorldFromGameViewport();

private:
	std::shared_ptr<CDungeonGeneratorCore> mDungeonGeneratorCore;

	TSharedPtr<FUICommandList> PluginCommands;
	TSharedPtr<SEditableTextBox> mRandomSeedValue;
	TSharedPtr<SButton> mGenerateDungeonButton;
	TSharedPtr<SButton> mGenerateTextureSizeButton;
	TSharedPtr<SButton> mGenerateTextureScaleButton;
	TWeakObjectPtr<UDungeonGenerateParameter> mDungeonGenerateParameter = nullptr;
	int32 mGenerateTextureSize = 512;
	int32 mGenerateTextureScale = 3;
	TSharedPtr<SEditableTextBox> mWorldToTextureScale;
};

inline TOptional<int32> FDungeonGenerateEditorModule::OnGetGenerateTextureSize() const
{
	return mGenerateTextureSize;
}

inline void FDungeonGenerateEditorModule::OnSetGenerateTextureSize(int32 value)
{
	mGenerateTextureSize = value;
}

inline TOptional<int32> FDungeonGenerateEditorModule::OnGetGenerateTextureScale() const
{
	return mGenerateTextureScale;
}

inline void FDungeonGenerateEditorModule::OnSetGenerateTextureScale(int32 value)
{
	mGenerateTextureScale = value;
}
