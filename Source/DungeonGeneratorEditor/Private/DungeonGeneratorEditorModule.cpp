/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorStyle.h"
#include "DungeonGeneratorCommands.h"
#include "BuildInformation.h"
#include "DungeonGeneratedActor.h"
#include "Actor/DungeonGenerateActorDetails.h"
#include "Helper/DungeonFinalizer.h"
#include "Debug/Debug.h"
#include "SubActor/DungeonRoomSensorDatabaseTypeActions.h"
#include "Parameter/DungeonMeshSetDatabaseTypeActions.h"
#include "Parameter/DungeonGenerateParameterTypeActions.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "Validation/DungeonParameterValidator.h"


#include <PropertyCustomizationHelpers.h>
#include <PropertyEditorModule.h>
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
#include <HAL/PlatformApplicationMisc.h>
#include <GenericPlatform/GenericPlatformProperties.h>
#include <Misc/App.h>
#include <Misc/EngineVersion.h>
#include <Misc/Paths.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <Widgets/Views/SHeaderRow.h>

#include "Misc/FileHelper.h"

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
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomClassLayout(TEXT("DungeonGenerateActor"));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

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
				SAssignNew(mVerifyButton, SButton)
					.Text(LOCTEXT("VerifyDungeonButton", "Verify"))
					.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedVerifyButton)
			];

		dungeonBox->AddSlot()
			.VAlign(VAlign_Center)
			.FillHeight(1.f)
			.Padding(2.f)
			[
				SAssignNew(mCopyDiagnosticsButton, SButton)
					.Text(LOCTEXT("CopyDiagnosticsButton", "Copy diagnostics"))
					.OnClicked_Raw(this, &FDungeonGenerateEditorModule::OnClickedCopyDiagnosticsButton)
			];

		dungeonBox->AddSlot()
			.VAlign(VAlign_Fill)
			.FillHeight(4.f)
			.Padding(2.f)
			[
				SAssignNew(mValidationListView, SListView<TSharedPtr<FDungeonValidationIssue>>)
					//.ItemHeight(24.f)
					.ListItemsSource(&mValidationIssueItems)
					.OnGenerateRow_Raw(this, &FDungeonGenerateEditorModule::OnGenerateValidationRow)
					.HeaderRow(
						SNew(SHeaderRow)
						+ SHeaderRow::Column("Severity").DefaultLabel(LOCTEXT("ValidationSeverity", "Severity")).FillWidth(0.15f)
						+ SHeaderRow::Column("Code").DefaultLabel(LOCTEXT("ValidationCode", "Code")).FillWidth(0.2f)
						+ SHeaderRow::Column("Message").DefaultLabel(LOCTEXT("ValidationMessage", "Message")).FillWidth(0.65f)
					)
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
	mVerifyButton->SetEnabled(enabled);
	mCopyDiagnosticsButton->SetEnabled(enabled);
	if (enabled)
	{
		RunValidation(true);
	}
	else
	{
		mValidationIssueItems.Reset();
		if (mValidationListView.IsValid())
		{
			mValidationListView->RequestListRefresh();
		}
	}
	UpdateGenerateButtonEnabled();

}


void FDungeonGenerateEditorModule::UpdateGenerateButtonEnabled() const
{
	const bool enabled = mDungeonGenerateParameter.IsValid() && !HasValidationErrors();
	mGenerateDungeonButton->SetEnabled(enabled);
}

void FDungeonGenerateEditorModule::RunValidation(const bool bDeepCheck)
{
	mValidationIssueItems.Reset();
	TArray<FDungeonValidationIssue> issues;
	FDungeonParameterValidator::Validate(mDungeonGenerateParameter.Get(), issues, bDeepCheck);
	for (const FDungeonValidationIssue& issue : issues)
	{
		mValidationIssueItems.Emplace(MakeShared<FDungeonValidationIssue>(issue));
	}
	if (mValidationListView.IsValid())
	{
		mValidationListView->RequestListRefresh();
	}
	UpdateGenerateButtonEnabled();
}

bool FDungeonGenerateEditorModule::HasValidationErrors() const
{
	return mValidationIssueItems.ContainsByPredicate([](const TSharedPtr<FDungeonValidationIssue>& issue)
		{
			return issue.IsValid() && issue->Severity == EDungeonValidationSeverity::Error;
		});
}

FReply FDungeonGenerateEditorModule::OnClickedVerifyButton()
{
	RunValidation(true);
	return FReply::Handled();
}

FReply FDungeonGenerateEditorModule::OnClickedCopyDiagnosticsButton() const
{
	FPlatformApplicationMisc::ClipboardCopy(*FormatIssuesForClipboard());
	return FReply::Handled();
}

TSharedRef<ITableRow> FDungeonGenerateEditorModule::OnGenerateValidationRow(TSharedPtr<FDungeonValidationIssue> item, const TSharedRef<STableViewBase>& ownerTable)
{
	if (!item.IsValid())
	{
		return SNew(STableRow<TSharedPtr<FDungeonValidationIssue>>, ownerTable)
		[
			SNew(STextBlock).Text(LOCTEXT("InvalidValidationRow", "Invalid item"))
		];
	}

	return SNew(STableRow<TSharedPtr<FDungeonValidationIssue>>, ownerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.15f).Padding(2.f)
		[
			SNew(STextBlock).Text(FText::FromString(FormatSeverity(item->Severity)))
		]
		+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2.f)
		[
			SNew(STextBlock).Text(FText::FromName(item->Code))
		]
		+ SHorizontalBox::Slot().FillWidth(0.65f).Padding(2.f)
		[
			SNew(STextBlock).Text(item->Message)
		]
	];
}

FString FDungeonGenerateEditorModule::FormatSeverity(const EDungeonValidationSeverity severity)
{
	switch (severity)
	{
	case EDungeonValidationSeverity::Error:
		return TEXT("Error");
	case EDungeonValidationSeverity::Warning:
		return TEXT("Warning");
	default:
		return TEXT("Info");
	}
}

FString FDungeonGenerateEditorModule::FormatIssuesForClipboard() const
{
	FString pluginVersion = TEXT("Unknown");
	FString pluginVersionName = TEXT("Unknown");
	FString pluginFile = FPaths::ProjectPluginsDir() / TEXT("DungeonGenerator/DungeonGenerator.uplugin");
	if (!FPaths::FileExists(pluginFile))
	{
		pluginFile = FPaths::Combine(FPaths::EnginePluginsDir(), TEXT("Marketplace/DungeonGenerator/DungeonGenerator.uplugin"));
	}
	FString jsonText;
	if (FFileHelper::LoadFileToString(jsonText, *pluginFile))
	{
		TSharedPtr<FJsonObject> root;
		TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(jsonText);
		if (FJsonSerializer::Deserialize(reader, root) && root.IsValid())
		{
			pluginVersion = FString::FromInt(root->GetIntegerField(TEXT("Version")));
			pluginVersionName = root->GetStringField(TEXT("VersionName"));
		}
	}

	FString report;
	report += TEXT("[DungeonGenerator Diagnostics]\n");
	report += FString::Printf(TEXT("PluginVersion: %s (%s)\n"), *pluginVersionName, *pluginVersion);
	report += FString::Printf(TEXT("UEVersion: %s\n"), *FEngineVersion::Current().ToString());
	report += FString::Printf(TEXT("OS: %s\n"), ANSI_TO_TCHAR(FPlatformProperties::IniPlatformName()));

	if (const UDungeonGenerateParameter* params = mDungeonGenerateParameter.Get())
	{
		const FDungeonGridSize gridSize = params->GetGridSize();
		report += FString::Printf(TEXT("Summary: RoomCount=%d GridSize=%.2f VerticalGridSize=%.2f Seed=%d\n"), params->GetNumberOfCandidateRooms(), gridSize.HorizontalSize, gridSize.VerticalSize, params->GetRandomSeed());
	}

	report += TEXT("Issues:\n");
	for (const TSharedPtr<FDungeonValidationIssue>& item : mValidationIssueItems)
	{
		if (!item.IsValid())
		{
			continue;
		}
		report += FString::Printf(TEXT("- [%s] [%s] %s | Hint: %s\n"), *FormatSeverity(item->Severity), *item->Code.ToString(), *item->Message.ToString(), *item->FixHint.ToString());
	}
	return report;
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
	RunValidation(false);
	if (HasValidationErrors())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ValidationFailed", "Generation aborted. Resolve validation errors first."));
		return FReply::Unhandled();
	}

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
