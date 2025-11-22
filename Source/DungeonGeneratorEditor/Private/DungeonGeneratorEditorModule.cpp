/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "SubActor/DungeonRoomSensorDatabaseTypeActions.h"
#include "Parameter/DungeonMeshSetDatabaseTypeActions.h"
#include "Parameter/DungeonGenerateParameterTypeActions.h"
#include "BuildInfomation.h"

#include "DungeonGeneratedActor.h"
#include "Helper/DungeonFinalizer.h"
#include "Parameter/DungeonGenerateParameter.h"

#include <PropertyCustomizationHelpers.h>
#include <AssetToolsModule.h>
#include <FileHelpers.h>
#include <IAssetTools.h>
#include <PackageTools.h>
#include <ToolMenus.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Texture2D.h>
#include <GameFramework/PlayerStart.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/MessageDialog.h>
#include <UObject/UObjectGlobals.h>
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

	// Get AssetTools module
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	// LegacyAsset
	EAssetTypeCategories::Type gameAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("DungeonGeneratorAssets")),
		FText::FromName(TEXT("DungeonGenerator"))
	);

	// Register UDungeonGenerateParameterTypeActions
	{
		TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new UDungeonGenerateParameterTypeActions(gameAssetCategory));
		AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
	}

	// Register FDungeonMeshSetDatabaseTypeActions
	{
		TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonMeshSetDatabaseTypeActions(gameAssetCategory));
		AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
	}


	// Register FDungeonRoomSensorDatabaseTypeActions
	{
		TSharedPtr<IAssetTypeActions> actionType = MakeShareable(new FDungeonRoomSensorDatabaseTypeActions(gameAssetCategory));
		AssetTools.RegisterAssetTypeActions(actionType.ToSharedRef());
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


	// Plug-in information
	{
		//auto informationBox = SNew(SVerticalBox);
		const auto informationBox = dungeonBox;
		informationBox->AddSlot()
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
			dungeonBox /* textureBox , informationBox*/
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
	FToolMenuOwnerScoped ownerScoped(this);

	{
		UToolMenu* menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		FToolMenuSection& section = menu->FindOrAddSection("WindowLayout");
		section.AddMenuEntryWithCommandList(FDungeonGeneratorCommands::Get().OpenPluginWindow, PluginCommands);
	}

	{
		UToolMenu* toolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		FToolMenuSection& section = toolbarMenu->FindOrAddSection("Settings");
		FToolMenuEntry& entry = section.AddEntry(FToolMenuEntry::InitToolBarButton(FDungeonGeneratorCommands::Get().OpenPluginWindow));
		entry.SetCommandList(PluginCommands);
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

}

FReply FDungeonGenerateEditorModule::OnClickedGenerateButton()
{
	UWorld* world = GetWorldFromGameViewport();
	if (!IsValid(world))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to find World object"));
		//DUNGEON_GENERATOR_ERROR(TEXT("Failed to find World object"));
		return FReply::Unhandled();
	}

	DisposeDungeon(world, true);

	// Get dungeon generation parameters
	const UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get();
	if (dungeonGenerateParameter == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Set the DungeonGenerateParameter"));
		//DUNGEON_GENERATOR_ERROR(TEXT("Set the DungeonGenerateParameter"));
		return FReply::Unhandled();
	}

#if WITH_EDITOR & JENKINS_FOR_DEVELOP
	const FVector origin = FVector(-3000, 2000, 1000);
#else
	const FVector& origin = FVector::ZeroVector;
#endif

	// spawn DungeonVegetationActor
	ADungeonGeneratedActor* dungeonActor = ADungeonGeneratedActor::SpawnDungeonActor(world, origin);
	if (IsValid(dungeonActor) == false)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to create dungeon vegetation actor"));
		return FReply::Unhandled();
	}

	if (!dungeonActor->Generate(dungeonGenerateParameter))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Message", "Failed to generate dungeon"));
		OnClickedClearButton();
		return FReply::Unhandled();
	}

	// ダンジョンアクターを記録
	mDungeonActor = dungeonActor;

	// Set random seeds for generated dungeons
	const int32 value = dungeonGenerateParameter->GetGeneratedRandomSeed();
	mRandomSeedValue->SetText(FText::FromString(FString::FromInt(value)));


	return FReply::Handled();
}

FReply FDungeonGenerateEditorModule::OnClickedClearButton()
{
	DisposeDungeon(GetWorldFromGameViewport(), false);


	return FReply::Handled();
}

void FDungeonGenerateEditorModule::DisposeDungeon(UWorld* world, const bool flushLevelStreaming)
{

	// Delete all generated actors
	ADungeonGeneratedActor::DestroySpawnedActors(world);
	mDungeonActor.Reset();

	if (flushLevelStreaming)
	{
		// Update the world
		world->FlushLevelStreaming();
	}
}


UWorld* FDungeonGenerateEditorModule::GetWorldFromGameViewport()
{
	if (IsValid(GEngine))
	{
		if (const auto* worldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport))
			return worldContext->World();
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDungeonGenerateEditorModule, DungeonGeneratorEditor)
