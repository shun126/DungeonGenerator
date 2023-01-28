/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "DungeonGenerateParameterTypeActions.h"
#include "DungeonRoomAssetTypeActions.h"
#include "BuildInfomation.h"
#include "../../DungeonGenerator/Public/DungeonGenerateParameter.h"
#include "../../DungeonGenerator/Public/DungeonGenerator.h"
//#include <EngineUtils.h>
//#include <Kismet/GameplayStatics.h>
#include <PropertyCustomizationHelpers.h>
#include <AssetToolsModule.h>
#include <IAssetTools.h>
#include <ToolMenus.h>
#include <GameFramework/PlayerStart.h>

static const FName DungeonGeneratorTabName("DungeonGeneratorModule");

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
		.SetDisplayName(LOCTEXT("FDungeonGeneratorTabTitle", "DungeonGeneratorModule"))
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

FReply FDungeonGenerateEditorModule::OnClickedGenerateButton()
{
	CDungeonGenerator::DestorySpawnedActors();

	if (UDungeonGenerateParameter* dungeonGenerateParameter = mDungeonGenerateParameter.Get())
	{
		std::shared_ptr<CDungeonGenerator> dungeonGenerator = std::make_shared<CDungeonGenerator>();
		dungeonGenerator->Create(dungeonGenerateParameter);
		dungeonGenerator->AddTerraine();
		dungeonGenerator->AddObject();

		const int32 value = dungeonGenerateParameter->GetGeneratedRandomSeed();
		mRandomSeedValue->SetText(FText::FromString(FString::FromInt(value)));

		dungeonGenerator->MovePlayerStart();

		// TODO:ミニマップテクスチャの出力を検討して下さい

		return FReply::Handled();
	}
	else
	{
		return FReply::Unhandled();
	}
}

FReply FDungeonGenerateEditorModule::OnClickedClearButton()
{
	CDungeonGenerator::DestorySpawnedActors();
	return FReply::Handled();
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDungeonGenerateEditorModule, DungeonGeneratorEditor)
