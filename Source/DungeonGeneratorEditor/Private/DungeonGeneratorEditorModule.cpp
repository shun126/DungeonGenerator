/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "DungeonGenerateParameterTypeActions.h"
#include "DungeonRoomAssetTypeActions.h"
#include "BuildInfomation.h"
#include "../../DungeonGenerator/Public/DungeonGenerateParameter.h"
#include "../../DungeonGenerator/Public/DungeonGeneratorCore.h"
#include "../../DungeonGenerator/Public/DungeonMiniMapTexture.h"
#include "../../DungeonGenerator/Public/dungeonMiniMapTextureLayer.h"
#include <PropertyCustomizationHelpers.h>
#include <AssetToolsModule.h>
#include <IAssetTools.h>
#include <PackageTools.h>
#include <ToolMenus.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <GameFramework/PlayerStart.h>
#include <Misc/EngineVersionComparison.h>
#include <UObject/SavePackage.h>
#include <UObject/UObjectGlobals.h>
#include <Widgets/Input/SDirectoryPicker.h>
#include <Widgets/Input/SNumericEntryBox.h>

static const FName DungeonGeneratorTabName("DungeonGenerator");

#define LOCTEXT_NAMESPACE "FDungeonGenerateEditorModule"

void FDungeonGenerateEditorModule::StartupModule()
{
	FDungeonGeneratorStyle::Initialize();
	FDungeonGeneratorStyle::ReloadTextures();

	FDungeonGeneratorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FDungeonGeneratorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FDungeonGenerateEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDungeonGenerateEditorModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DungeonGeneratorTabName, FOnSpawnTab::CreateRaw(this, &FDungeonGenerateEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FDungeonGeneratorTabTitle", "DungeonGenerator"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// AssetToolsモジュールを取得
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// LegacyAsset
	{
		EAssetTypeCategories::Type gameAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
			FName(TEXT("DungeonGeneratorAssets")),
			FText::FromName(TEXT("DungeonGenerator"))
		);

		// FDungeonRoomAssetTypeActionsを追加
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonRoomAssetTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// UDungeonGenerateParameterTypeActionsを追加
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new UDungeonGenerateParameterTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}
	}
}

void FDungeonGenerateEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FDungeonGeneratorStyle::Shutdown();

	FDungeonGeneratorCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DungeonGeneratorTabName);
}


TSharedRef<SDockTab> FDungeonGenerateEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> dockTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SVerticalBox)
			// UDungeonGenerateParameter
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("DungeonGenerateParameterLabel", "DungeonGenerateParameter"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			//SAssignNew(EntryBox, SObjectPropertyEntryBox)
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UDungeonGenerateParameter::StaticClass())
		.ObjectPath_Raw(this, &FDungeonGenerateEditorModule::GetObjectPath)
		.OnObjectChanged_Raw(this, &FDungeonGenerateEditorModule::SetAssetData)
		]
		]
	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RandomSeedLabel", "Random seed"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(mRandomSeedValue, SEditableTextBox)
			.Text(LOCTEXT("RandomSeedValue", ""))
		]
		]

	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateDungeonButton", "Generate dungeon"))
		.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateButton)
		]

	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateDungeonButton", "Clear dungeon"))
		.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedClearButton)
		]


	// Texture output size
	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TextureOutputSizeLabel", "Texture output size"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.MinValue(1)
			.MinSliderValue(1)
			.Value(512)
			.Value_Raw(this, &FDungeonGenerateEditorModule::OnGetGenerateTextureSize)
		.OnValueChanged_Raw(this, &FDungeonGenerateEditorModule::OnSetGenerateTextureSize)
		]
		]

	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateTextureSizeButton", "Generate texture with size"))
			.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateTextureWithSizeButton)
		]

	// Texture output scale
	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TextureOutputScaleLabel", "Texture output scale"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SNumericEntryBox<int32>)
			.AllowSpin(true)
			.MinValue(1)
			.MinSliderValue(1)
			.Value(1)
			.Value_Raw(this, &FDungeonGenerateEditorModule::OnGetGenerateTextureScale)
			.OnValueChanged_Raw(this, &FDungeonGenerateEditorModule::OnSetGenerateTextureScale)
		]
		]

	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateTextureScaleButton", "Generate texture with scale"))
		.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateTextureWithScaleButton)
		]


	+SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.VAlign(VAlign_Center)
		.AutoWidth()
		.Padding(2.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("JenkinsBuildTagLabel", "Build tag"))
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("JenkinsBuildTagValue", JENKINS_JOB_TAG))
		]
		]
		];

	return dockTab;
}

void FDungeonGenerateEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DungeonGeneratorTabName);
}

void FDungeonGenerateEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FDungeonGeneratorCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FDungeonGeneratorCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

FString FDungeonGenerateEditorModule::GetObjectPath() const
{
	UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	return dungeonGenerateParameter ? dungeonGenerateParameter->GetPathName() : FString("");
}

void FDungeonGenerateEditorModule::SetAssetData(const FAssetData& assetData)
{
	mDungeonGenerateParameter = Cast<UDungeonGenerateParameter>(assetData.GetAsset());
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateButton()
{
	UWorld* world = GetWorldFromGameViewport();
	if (!IsValid(world))
	{
		// TODO:エラーログを出力してください
		return FReply::Unhandled();
	}

	// 生成した全アクターを削除
	CDungeonGeneratorCore::DestroySpawnedActors(world);

	// アクターを生成
	if (UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get())
	{
		mDungeonGeneratorCore = std::make_shared<CDungeonGeneratorCore>(world);
		if (mDungeonGeneratorCore == nullptr)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}

		mDungeonGeneratorCore->Create(dungeonGenerateParameter);
		mDungeonGeneratorCore->MovePlayerStart();

		const int32 value = dungeonGenerateParameter->GetGeneratedRandomSeed();
		mRandomSeedValue->SetText(FText::FromString(FString::FromInt(value)));

		// TODO:ミニマップテクスチャの出力を検討して下さい

		return FReply::Handled();
	}
	else
	{
		// TODO:エラーログを出力してください
		return FReply::Unhandled();
	}
}

FReply FDungeonGenerateEditorModule::OnClickedClearButton()
{
	// 生成した全アクターを削除
	CDungeonGeneratorCore::DestroySpawnedActors(GetWorldFromGameViewport());

	mDungeonGeneratorCore.reset();

	return FReply::Handled();
}

TOptional<int32> FDungeonGenerateEditorModule::OnGetGenerateTextureSize() const
{
	return mGenerateTextureSize;
}

void FDungeonGenerateEditorModule::OnSetGenerateTextureSize(int32 value)
{
	mGenerateTextureSize = value;
}

TOptional<int32> FDungeonGenerateEditorModule::OnGetGenerateTextureScale() const
{
	return mGenerateTextureScale;
}

void FDungeonGenerateEditorModule::OnSetGenerateTextureScale(int32 value)
{
	mGenerateTextureScale = value;
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateTextureWithSizeButton()
{
	if (UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get())
	{
		if (mDungeonGeneratorCore == nullptr)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}

		UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
		if (!IsValid(dungeonMiniMapTextureLayer))
		{
			return FReply::Unhandled();
		}

		if (dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithSize(mDungeonGeneratorCore, mGenerateTextureSize, dungeonGenerateParameter->GetGridSize()) == false)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}

		return SaveTextures(dungeonMiniMapTextureLayer);
	}

	return FReply::Unhandled();
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateTextureWithScaleButton()
{
	if (UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get())
	{
		if (mDungeonGeneratorCore == nullptr)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}

		UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
		if (!IsValid(dungeonMiniMapTextureLayer))
		{
			return FReply::Unhandled();
		}

		if (dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithScale(mDungeonGeneratorCore, mGenerateTextureScale, dungeonGenerateParameter->GetGridSize()) == false)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}

		return SaveTextures(dungeonMiniMapTextureLayer);
	}

	return FReply::Unhandled();
}

FReply FDungeonGenerateEditorModule::SaveTextures(const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer) const
{
	class Finalizer final
	{
	public:
		explicit Finalizer(std::function<void()> function) : mFunction(function) {}
		~Finalizer() { mFunction(); }
	private:
		std::function<void()> mFunction;
	};

	const TCHAR* basePath = TEXT("/Game/ProceduralTextures/");
	const FString absoluteBasePath = FPaths::ProjectContentDir() + TEXT("/ProceduralTextures/");
	FPackageName::RegisterMountPoint(basePath, absoluteBasePath);

	Finalizer unmount([basePath, &absoluteBasePath]()
		{
			FPackageName::UnRegisterMountPoint(basePath, absoluteBasePath);
		}
	);

	TArray<UPackage*> packages;

	const int32 floorHeight = dungeonMiniMapTextureLayer->GetFloorHeight();
	for (int32 floor = 0; floor < floorHeight; ++floor)
	{
		const FString packageName = FString::Printf(TEXT("Floor%d"), floor);
		const FString packagePath = basePath + packageName;
		UPackage* package = CreatePackage(*packagePath);
		if (!IsValid(package))
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}
		packages.Add(package);

		package->FullyLoad();
		package->MarkPackageDirty();

		UDungeonMiniMapTexture* dungeonMiniMapTexture = dungeonMiniMapTextureLayer->GetByFloor(floor);

#if UE_VERSION_NEWER_THAN(5, 1, 0)
		FName textureName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName), EUniqueObjectNameOptions::GloballyUnique);
#else
		FName textureName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName));
#endif
		UTexture2D* texture = DuplicateObject(dungeonMiniMapTexture->GetTexture(), package, textureName);
		texture->SetFlags(RF_Public | RF_Standalone);
		texture->ClearFlags(RF_Transient);

		FAssetRegistryModule::AssetCreated(texture);

		const FString longPackageName = FPackageName::LongPackageNameToFilename(packagePath, FPackageName::GetAssetPackageExtension());

#if UE_VERSION_OLDER_THAN(5, 0, 0)
		const bool saved = UPackage::SavePackage(package, texture, RF_Public | RF_Standalone, *longPackageName);
#else
		FSavePackageArgs saveArgs;
		saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		saveArgs.SaveFlags = SAVE_NoError;
		const bool saved = UPackage::SavePackage(package, texture, *longPackageName, saveArgs);
#endif
		if (saved == false)
		{
			// TODO:エラーログを出力してください
			return FReply::Unhandled();
		}
	}

	if (!PackageTools::UnloadPackages(packages))
	{
		// TODO:エラーログを出力してください
		return FReply::Unhandled();
	}

	packages.Empty();

	return FReply::Handled();
}

UWorld* FDungeonGenerateEditorModule::GetWorldFromGameViewport()
{
	if (FWorldContext* worldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport))
		return worldContext->World();

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDungeonGenerateEditorModule, DungeonGeneratorEditor)
