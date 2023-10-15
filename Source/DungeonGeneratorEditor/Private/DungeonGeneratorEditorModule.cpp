/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "DungeonGenerateParameterTypeActions.h"
#include "DungeonInteriorDatabaseTypeActions.h"
#include "DungeonMiniMapTypeActions.h"
#include "DungeonAisleMeshSetDatabaseTypeActions.h"
#include "DungeonRoomMeshSetDatabaseTypeActions.h"
#include "DungeonRoomDatabaseTypeActions.h"
#include "BuildInfomation.h"
#include "../../DungeonGenerator/Public/DungeonGenerateParameter.h"
#include "../../DungeonGenerator/Public/DungeonGeneratorCore.h"
#include "../../DungeonGenerator/Public/MiniMap/DungeonMiniMap.h"
#include "../../DungeonGenerator/Public/MiniMap/DungeonMiniMapTexture.h"
#include "../../DungeonGenerator/Public/MiniMap/DungeonMiniMapTextureLayer.h"
#include <PropertyCustomizationHelpers.h>
#include <AssetToolsModule.h>
#include <IAssetTools.h>
#include <PackageTools.h>
#include <ToolMenus.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <GameFramework/PlayerStart.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/MessageDialog.h>
#include <UObject/SavePackage.h>
#include <UObject/UObjectGlobals.h>
#include <Widgets/Input/SDirectoryPicker.h>
#include <Widgets/Input/SNumericEntryBox.h>

//#define _ENABLE_UNREGISTER_MOUNT_POINT
//#define _ENABLE_SAVE_AND_CLOSE_PACKAGES

static const FName DungeonGeneratorTabName("DungeonGenerator");

#define LOCTEXT_NAMESPACE "FDungeonGenerateEditorModule"

DECLARE_LOG_CATEGORY_EXTERN(DungeonGeneratorLogger, Log, All);
#define DUNGEON_GENERATOR_ERROR(Format, ...)		UE_LOG(DungeonGeneratorLogger, Error, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		UE_LOG(DungeonGeneratorLogger, Warning, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		UE_LOG(DungeonGeneratorLogger, Display, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_LOG(Format, ...)			UE_LOG(DungeonGeneratorLogger, Log, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		UE_LOG(DungeonGeneratorLogger, Verbose, Format, ##__VA_ARGS__)
DEFINE_LOG_CATEGORY(DungeonGeneratorLogger);

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

	// Get AssetTools module
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// LegacyAsset
	{
		EAssetTypeCategories::Type gameAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
			FName(TEXT("DungeonGeneratorAssets")),
			FText::FromName(TEXT("DungeonGenerator"))
		);

		// Register FDungeonInteriorDatabaseTypeActions
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonInteriorDatabaseTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// Register FDungeonAisleMeshSetDatabaseTypeActions
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonAisleMeshSetDatabaseTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// Register FDungeonRoomMeshSetDatabaseTypeActions
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonRoomMeshSetDatabaseTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// Register FDungeonRoomDatabaseTypeActions
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonRoomDatabaseTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// Register FDungeonMiniMapLayerInfoTypeActions
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonMiniMapTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

		// Register UDungeonGenerateParameterTypeActions
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

/*
aka: https://docs.unrealengine.com/5.2/ja/slate-overview-for-unreal-engine/
*/
TSharedRef<SDockTab> FDungeonGenerateEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// dungeon generation settings
	auto dungeonBox = SNew(SVerticalBox);
	{
		dungeonBox->AddSlot()
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
			];

		dungeonBox->AddSlot()
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
			];
	
		dungeonBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			.Padding(2.f)
			[
				SAssignNew(mGenerateDungeonButton, SButton)
				.Text(LOCTEXT("GenerateDungeonButton", "Generate dungeon"))
				.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateButton)
			];

		dungeonBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			.Padding(2.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ClearDungeonButton", "Clear dungeon"))
				.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedClearButton)
			];
	}

	// mini-map texture settings
	//auto textureBox = SNew(SVerticalBox);
	auto textureBox = dungeonBox;
	{
		textureBox->AddSlot()
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
			];

		textureBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			.Padding(2.f)
			[
				SAssignNew(mGenerateTextureSizeButton, SButton)
				.Text(LOCTEXT("GenerateTextureSizeButton", "Generate texture with size"))
				.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateTextureWithSizeButton)
			];

		// Texture output scale
		textureBox->AddSlot()
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
			];

		textureBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			.Padding(2.f)
			[
				SAssignNew(mGenerateTextureScaleButton, SButton)
				.Text(LOCTEXT("GenerateTextureScaleButton", "Generate texture with scale"))
				.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateTextureWithScaleButton)
			];

		textureBox->AddSlot()
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
					.Text(LOCTEXT("WorldToTextureScaleLabel", "World to texture space scale"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SAssignNew(mWorldToTextureScale, SEditableTextBox)
					.Text(LOCTEXT("WorldToTextureScale", ""))
				]
			];
	}

	// Plug-in infomation
	//auto infomationBox = SNew(SVerticalBox);
	auto infomationBox = dungeonBox;
	{
		infomationBox->AddSlot()
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
			];
	}

	TSharedRef<SDockTab> dockTab = SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			dungeonBox /* textureBox , infomationBox*/
		];

	SetAssetData(FAssetData());

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
	UDungeonGenerateParameter* dungeonGenerateParameter = Cast<UDungeonGenerateParameter>(assetData.GetAsset());
	mDungeonGenerateParameter = dungeonGenerateParameter;

	// Enable buttons the parameters are set.
	const bool enabled = dungeonGenerateParameter != nullptr;
	mGenerateDungeonButton->SetEnabled(enabled);
	UpdateGenerateTextureButtons();
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateButton()
{
	UWorld* world = GetWorldFromGameViewport();
	if (!IsValid(world))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to find World object"));
		return FReply::Unhandled();
	}

	// Delete all generated actors
	CDungeonGeneratorCore::DestroySpawnedActors(world);

	// Release generated dungeons
	if (mDungeonGeneratorCore)
	{
		mDungeonGeneratorCore->UnloadStreamLevels();
	}

	// Get dungeon generation parameters
	const UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	if (dungeonGenerateParameter == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

	// Generate dungeon
	mDungeonGeneratorCore = std::make_shared<CDungeonGeneratorCore>(world);
	if (mDungeonGeneratorCore == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to create dungeon generator object"));
		return FReply::Unhandled();
	}
	if (!mDungeonGeneratorCore->Create(dungeonGenerateParameter))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to generate dungeon"));
		OnClickedClearButton();
		return FReply::Unhandled();
	}
	mDungeonGeneratorCore->SyncLoadStreamLevels();
	mDungeonGeneratorCore->MovePlayerStart();

	// Set random seeds for generated dungeons
	const int32 value = dungeonGenerateParameter->GetGeneratedRandomSeed();
	mRandomSeedValue->SetText(FText::FromString(FString::FromInt(value)));

	UpdateGenerateTextureButtons();

	return FReply::Handled();
}

FReply FDungeonGenerateEditorModule::OnClickedClearButton()
{
	// Release generated dungeons
	if (mDungeonGeneratorCore)
	{
		mDungeonGeneratorCore->UnloadStreamLevels();
	}
	mDungeonGeneratorCore.reset();

	// Delete all generated actors
	CDungeonGeneratorCore::DestroySpawnedActors(GetWorldFromGameViewport());

	UpdateGenerateTextureButtons();

	return FReply::Handled();
}

void FDungeonGenerateEditorModule::UpdateGenerateTextureButtons()
{
	const bool enabled = mDungeonGenerateParameter.IsValid() && mDungeonGeneratorCore != nullptr;
	mGenerateTextureSizeButton->SetEnabled(enabled);
	mGenerateTextureScaleButton->SetEnabled(enabled);
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateTextureWithSizeButton()
{
	const UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	if (dungeonGenerateParameter == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

	if (mDungeonGeneratorCore == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Generate the dungeon first."));
		return FReply::Unhandled();
	}

	UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
	if (!IsValid(dungeonMiniMapTextureLayer))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to create DungeonMiniMapTextureLayer object"));
		return FReply::Unhandled();
	}

	if (!dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithSize(mDungeonGeneratorCore, mGenerateTextureSize, dungeonGenerateParameter->GetGridSize()))
	{
		return FReply::Unhandled();
	}

	if (!SavePackages(dungeonMiniMapTextureLayer))
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateTextureWithScaleButton()
{
	const UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	if (dungeonGenerateParameter == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

	if (mDungeonGeneratorCore == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Generate the dungeon first."));
		return FReply::Unhandled();
	}

	UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
	if (!IsValid(dungeonMiniMapTextureLayer))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to create DungeonMiniMapTextureLayer object"));
		return FReply::Unhandled();
	}

	if (!dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithScale(mDungeonGeneratorCore, mGenerateTextureScale, dungeonGenerateParameter->GetGridSize()))
	{
		return FReply::Unhandled();
	}

	if (!SavePackages(dungeonMiniMapTextureLayer))
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

bool FDungeonGenerateEditorModule::SavePackages(const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer)
{
#if defined(_ENABLE_UNREGISTER_MOUNT_POINT)
	// Finalizer class
	class Finalizer final
	{
	public:
		explicit Finalizer(std::function<void()> function) : mFunction(function) {}
		~Finalizer() { mFunction(); }
	private:
		std::function<void()> mFunction;
	};
#endif
	// Mount the package destination
	static const FString baseName = TEXT("/ProceduralTextures/");
	static const FString basePath = TEXT("/Game") + baseName;
	const FString absoluteBasePath = FPaths::ProjectContentDir() + baseName;
	FPackageName::RegisterMountPoint(basePath, absoluteBasePath);
#if defined(_ENABLE_UNREGISTER_MOUNT_POINT)
	// Reserve unmounted package destination
	Finalizer unmount([&absoluteBasePath]()
		{
			FPackageName::UnRegisterMountPoint(basePath, absoluteBasePath);
		}
	);
#endif
	// Initialize world name
	FString worldName;
	if (const UWorld* world = GetWorldFromGameViewport())
		worldName = world->GetName() + TEXT("_");

	// Save Texture layer infomation
	if (!SaveLayerInfoPackage(basePath, worldName, dungeonMiniMapTextureLayer))
		return false;

	return true;
}

// Texture layer infomation
bool FDungeonGenerateEditorModule::SaveLayerInfoPackage(const FString& basePath, const FString& worldName, const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer)
{
	const FString packageName = worldName + TEXT("MiniMap");
	const FString packagePath = basePath + packageName;

	// Create package
	UPackage* package = CreatePackage(*packagePath);
	if (!IsValid(package))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to create DungeonMiniMap package"));
		return false;
	}
	package->FullyLoad();

	// Create UDungeonMiniMap object
#if UE_VERSION_NEWER_THAN(5, 1, 0)
	FName objectName = MakeUniqueObjectName(package, UDungeonMiniMap::StaticClass(), FName(*packageName), EUniqueObjectNameOptions::GloballyUnique);
#else
	FName objectName = MakeUniqueObjectName(package, UDungeonMiniMap::StaticClass(), FName(*packageName));
#endif
	UDungeonMiniMap* dungeonMiniMap = NewObject<UDungeonMiniMap>(package, objectName, RF_Public | RF_Standalone);
	if (!IsValid(dungeonMiniMap))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to create DungeonMiniMap object"));
		return false;
	}

	// Initialize UDungeonMiniMap object
	dungeonMiniMap->SetGridSize(dungeonMiniMapTextureLayer->GetGridScale());
	dungeonMiniMap->SetWorldToTextureScale(dungeonMiniMapTextureLayer->GetScaleWorldToTexture());
	for (int32_t elevation : dungeonMiniMapTextureLayer->GetFloorElevations())
	{
		dungeonMiniMap->Add(elevation);
	}

	// Notifies that a new asset has been created.
	FAssetRegistryModule::AssetCreated(dungeonMiniMap);
	dungeonMiniMap->MarkPackageDirty();
	dungeonMiniMap->PostEditChange();
	dungeonMiniMap->AddToRoot();
	package->MarkPackageDirty();

	// Save texture assets by floors
	TArray<UPackage*> packages {package};
	if (!SaveTexturePackage(dungeonMiniMap, packages, basePath, worldName, dungeonMiniMapTextureLayer))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to create texture asset"));
		return false;
	}

#if defined(_ENABLE_SAVE_AND_CLOSE_PACKAGES)
	// Package and save UDungeonMiniMap objects
	const FString longPackageName = FPackageName::LongPackageNameToFilename(package->GetName(), FPackageName::GetAssetPackageExtension());
#if UE_VERSION_NEWER_THAN(5, 1, 0)
	FSavePackageArgs saveArgs;
	saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	saveArgs.SaveFlags = SAVE_NoError;
	const bool saved = UPackage::SavePackage(package, dungeonMiniMap, *longPackageName, saveArgs);
#else
	const bool saved = UPackage::SavePackage(package, dungeonMiniMap, RF_Public | RF_Standalone, *longPackageName);
#endif
	if (saved == false)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to save Package '%s'"), *longPackageName);
		return false;
	}

	// Unload packages
	if (!UPackageTools::UnloadPackages(packages))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to unload Packages"));
		return false;
	}
#endif

	return true;
}

bool FDungeonGenerateEditorModule::SaveTexturePackage(UDungeonMiniMap* dungeonMiniMap, TArray<UPackage*>& packages, const FString& basePath, const FString& worldName, const UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer)
{
	// Generate texture assets per floors
	const int32 floorHeight = dungeonMiniMapTextureLayer->GetFloorHeight();
	for (int32 floor = 0; floor < floorHeight; ++floor)
	{
		// Create package
		const FString packageName = FString::Printf(TEXT("%sFloor%d"), *worldName, floor);
		const FString packagePath = basePath + packageName;
		UPackage* package = CreatePackage(*packagePath);
		if (!IsValid(package))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Failed to create Package"));
			return false;
		}
		package->FullyLoad();

		// Duplicate texture object and change to saveable
#if UE_VERSION_NEWER_THAN(5, 1, 0)
		FName objectName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName), EUniqueObjectNameOptions::GloballyUnique);
#else
		FName objectName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName));
#endif

		// Get minimap texture
		UDungeonMiniMapTexture* dungeonMiniMapTexture = dungeonMiniMapTextureLayer->GetByFloor(floor);
		UTexture2D* texture = DuplicateObject(dungeonMiniMapTexture->GetTexture(), package, objectName);
		texture->SetFlags(RF_Public | RF_Standalone);
		texture->ClearFlags(RF_Transient);

		// Notifies that a new asset has been created.
		FAssetRegistryModule::AssetCreated(texture);
		texture->MarkPackageDirty();
		texture->PostEditChange();
		texture->AddToRoot();
		package->MarkPackageDirty();

#if defined(_ENABLE_SAVE_AND_CLOSE_PACKAGES)
		// Record saved packages
		packages.Add(package);

		// Package and save texture objects
		const FString longPackageName = FPackageName::LongPackageNameToFilename(package->GetName(), FPackageName::GetAssetPackageExtension());
#if UE_VERSION_NEWER_THAN(5, 1, 0)
		FSavePackageArgs saveArgs;
		saveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		saveArgs.SaveFlags = SAVE_NoError;
		const bool saved = UPackage::SavePackage(package, texture, *longPackageName, saveArgs);
#else
		const bool saved = UPackage::SavePackage(package, texture, RF_Public | RF_Standalone, *longPackageName);
#endif
		if (saved == false)
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Failed to save Package '%s'"), *longPackageName);
			return false;
		}
#endif

		// Add texture into UDungeonMiniMap
		dungeonMiniMap->AddTexture(texture);
	}

	// Set scale value to texture space
	{
		const float ratio = dungeonMiniMapTextureLayer->GetScaleWorldToTexture();
		mWorldToTextureScale->SetText(FText::FromString(FString::SanitizeFloat(ratio)));
	}

	return true;
}

UWorld* FDungeonGenerateEditorModule::GetWorldFromGameViewport()
{
	if (FWorldContext* worldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport))
		return worldContext->World();

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDungeonGenerateEditorModule, DungeonGeneratorEditor)
