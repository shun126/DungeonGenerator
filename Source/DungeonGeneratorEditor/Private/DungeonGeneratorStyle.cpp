/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorStyle.h"
#include <Framework/Application/SlateApplication.h>
#include <Interfaces/IPluginManager.h>
#include <Styling/SlateStyleRegistry.h>

#include <Misc/EngineVersionComparison.h>
#include <Styling/SlateStyleMacros.h>

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FDungeonGeneratorStyle::StyleInstance = nullptr;

//static const FVector2D Icon16x16(16.0f, 16.0f);
static const FVector2D Icon20x20(20.0f, 20.0f);

void FDungeonGeneratorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FDungeonGeneratorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FDungeonGeneratorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("DungeonGeneratorStyle"));
	return StyleSetName;
}

TSharedRef< FSlateStyleSet > FDungeonGeneratorStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("DungeonGeneratorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("DungeonGenerator")->GetBaseDir() / TEXT("Resources"));
	Style->Set("DungeonGenerator.OpenPluginWindow", new IMAGE_BRUSH(TEXT("PlaceholderButtonIcon"), Icon20x20));
	return Style;
}

void FDungeonGeneratorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FDungeonGeneratorStyle::Get()
{
	return *StyleInstance;
}
