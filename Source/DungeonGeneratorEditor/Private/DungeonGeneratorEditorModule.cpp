/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "DungeonGenerateParameterTypeActions.h"
#include "DungeonInteriorAssetTypeActions.h"
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

	// AssetToolsモジュールを取得
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// LegacyAsset
	{
		EAssetTypeCategories::Type gameAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
			FName(TEXT("DungeonGeneratorAssets")),
			FText::FromName(TEXT("DungeonGenerator"))
		);

		// FDungeonInteriorAssetTypeActionsを追加
		{
			TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonInteriorAssetTypeActions(gameAssetCategory));
			AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
		}

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
			SAssignNew(mGenerateDungeonButton, SButton)
			.Text(LOCTEXT("GenerateDungeonButton", "Generate dungeon"))
		.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedGenerateButton)
		]

	+ SVerticalBox::Slot()
		.VAlign(VAlign_Center)
		.FillHeight(1.f)
		.Padding(2.f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ClearDungeonButton", "Clear dungeon"))
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
			SAssignNew(mGenerateTextureSizeButton, SButton)
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
			SAssignNew(mGenerateTextureScaleButton, SButton)
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
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to create dungeon generator object"));
		return FReply::Unhandled();
	}
	if (!mDungeonGeneratorCore->Create(dungeonGenerateParameter))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to generate dungeon"));
		OnClickedClearButton();
		return FReply::Unhandled();
	}
	mDungeonGeneratorCore->SyncLoadStreamLevels();
	if (dungeonGenerateParameter->IsMovePlayerStartToStartingPoint())
	{
		mDungeonGeneratorCore->MovePlayerStart();
	}

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
		DUNGEON_GENERATOR_ERROR(TEXT("Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

	if (mDungeonGeneratorCore == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Generate the dungeon first."));
		return FReply::Unhandled();
	}

	UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
	if (!IsValid(dungeonMiniMapTextureLayer))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to create DungeonMiniMapTextureLayer object"));
		return FReply::Unhandled();
	}

	if (!dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithSize(mDungeonGeneratorCore, mGenerateTextureSize, dungeonGenerateParameter->GetGridSize()))
	{
		return FReply::Unhandled();
	}

	return SaveTextures(dungeonMiniMapTextureLayer);
}

FReply FDungeonGenerateEditorModule::OnClickedGenerateTextureWithScaleButton()
{
	const UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	if (dungeonGenerateParameter == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

	if (mDungeonGeneratorCore == nullptr)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Generate the dungeon first."));
		return FReply::Unhandled();
	}

	UDungeonMiniMapTextureLayer* dungeonMiniMapTextureLayer = NewObject<UDungeonMiniMapTextureLayer>();
	if (!IsValid(dungeonMiniMapTextureLayer))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Failed to create DungeonMiniMapTextureLayer object"));
		return FReply::Unhandled();
	}

	if (!dungeonMiniMapTextureLayer->GenerateMiniMapTextureWithScale(mDungeonGeneratorCore, mGenerateTextureScale, dungeonGenerateParameter->GetGridSize()))
	{
		return FReply::Unhandled();
	}

	return SaveTextures(dungeonMiniMapTextureLayer);
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

	// Mount the package destination
	const TCHAR* basePath = TEXT("/Game/ProceduralTextures/");
	const FString absoluteBasePath = FPaths::ProjectContentDir() + TEXT("/ProceduralTextures/");
	FPackageName::RegisterMountPoint(basePath, absoluteBasePath);

	// Reserve unmounted package destination
	Finalizer unmount([basePath, &absoluteBasePath]()
		{
			FPackageName::UnRegisterMountPoint(basePath, absoluteBasePath);
		}
	);

	// Save textures as assets
	{
		TArray<UPackage*> packages;

		// Generate texture assets per floors
		const int32 floorHeight = dungeonMiniMapTextureLayer->GetFloorHeight();
		for (int32 floor = 0; floor < floorHeight; ++floor)
		{
			// Create package
			const FString packageName = FString::Printf(TEXT("Floor%d"), floor);
			const FString packagePath = basePath + packageName;
			UPackage* package = CreatePackage(*packagePath);
			if (!IsValid(package))
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Failed to create Package"));
				return FReply::Unhandled();
			}

			// Load package
			package->FullyLoad();
			package->MarkPackageDirty();

			// Get minimap texture
			UDungeonMiniMapTexture* dungeonMiniMapTexture = dungeonMiniMapTextureLayer->GetByFloor(floor);

			// Duplicate texture object and change to saveable
#if UE_VERSION_NEWER_THAN(5, 1, 0)
			FName textureName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName), EUniqueObjectNameOptions::GloballyUnique);
#else
			FName textureName = MakeUniqueObjectName(package, UTexture2D::StaticClass(), FName(*packageName));
#endif
			UTexture2D* texture = DuplicateObject(dungeonMiniMapTexture->GetTexture(), package, textureName);
			texture->SetFlags(RF_Public | RF_Standalone);
			texture->ClearFlags(RF_Transient);

			// Register texture objects as assets
			FAssetRegistryModule::AssetCreated(texture);

			// Package and save texture objects
			const FString longPackageName = FPackageName::LongPackageNameToFilename(packagePath, FPackageName::GetAssetPackageExtension());
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
				return FReply::Unhandled();
			}

			// Record saved packages
			packages.Add(package);
		}

		// Unload packages
		if (!PackageTools::UnloadPackages(packages))
		{
			DUNGEON_GENERATOR_ERROR(TEXT("Failed to unload Packages"));
			return FReply::Unhandled();
		}

		packages.Empty();
	}

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
