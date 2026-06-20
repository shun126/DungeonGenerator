/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Migration/DungeonAssetMigrationService.h"

#include <Logging/MessageLog.h>
#include <MessageLogModule.h>
#include <Misc/MessageDialog.h>
#include <Modules/ModuleManager.h>
#include <UObject/Package.h>
#include <UObject/UObjectHash.h>

#define LOCTEXT_NAMESPACE "FDungeonAssetMigrationService"

namespace
{
	const FName DungeonGeneratorMigrationLogName(TEXT("DungeonGeneratorMigration"));

	FText FormatVersionText(const int32 Version)
	{
		if (Version == FDungeonGeneratorAssetVersion::BeforeCustomVersionWasAdded)
		{
			return LOCTEXT("BeforeCustomVersionWasAdded", "Before Custom Version");
		}
		return FText::AsNumber(Version);
	}
}

void FDungeonAssetMigrationService::Startup()
{
	if (bStarted)
	{
		return;
	}

	if (FModuleManager::Get().IsModuleLoaded("MessageLog") || FModuleManager::Get().LoadModule("MessageLog") != nullptr)
	{
		auto& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		if (!MessageLogModule.IsRegisteredLogListing(DungeonGeneratorMigrationLogName))
		{
			MessageLogModule.RegisterLogListing(DungeonGeneratorMigrationLogName, LOCTEXT("DungeonGeneratorMigrationLog", "DungeonGenerator Migration"));
		}
	}

	FDungeonAssetMigrationDelegates::OnAssetPostLoad.BindRaw(this, &FDungeonAssetMigrationService::OnAssetPostLoad);

	PreviousIsPackageOKToSaveDelegate = FCoreUObjectDelegates::IsPackageOKToSaveDelegate;
	FCoreUObjectDelegates::IsPackageOKToSaveDelegate.BindRaw(this, &FDungeonAssetMigrationService::IsPackageOKToSave);

	PackageSavedDelegateHandle = UPackage::PackageSavedWithContextEvent.AddRaw(this, &FDungeonAssetMigrationService::OnPackageSaved);
	bStarted = true;
}

void FDungeonAssetMigrationService::Shutdown()
{
	if (!bStarted)
	{
		return;
	}

	FDungeonAssetMigrationDelegates::OnAssetPostLoad.Unbind();
	UPackage::PackageSavedWithContextEvent.Remove(PackageSavedDelegateHandle);

	FCoreUObjectDelegates::IsPackageOKToSaveDelegate = PreviousIsPackageOKToSaveDelegate;
	PreviousIsPackageOKToSaveDelegate.Unbind();

	if (FModuleManager::Get().IsModuleLoaded("MessageLog"))
	{
		auto& MessageLogModule = FModuleManager::GetModuleChecked<FMessageLogModule>("MessageLog");
		MessageLogModule.UnregisterLogListing(DungeonGeneratorMigrationLogName);
	}

	bStarted = false;
}

void FDungeonAssetMigrationService::OnAssetPostLoad(UObject* Asset, const int32 LoadedAssetVersion)
{
	if (!IsValid(Asset) || IsRunningCookCommandlet())
	{
		return;
	}

	auto* MigratableAsset = Cast<IDungeonMigratableAsset>(Asset);
	if (MigratableAsset == nullptr)
	{
		return;
	}

	FDungeonAssetMigrationState NewState = MakeNotRequiredState(LoadedAssetVersion);
	constexpr int32 LatestVersion = FDungeonGeneratorAssetVersion::LatestVersion;

	if (LoadedAssetVersion > LatestVersion)
	{
		NewState.Status = EDungeonAssetMigrationStatus::Failed;
		AddMessage(
			NewState,
			EDungeonMigrationMessageSeverity::Error,
			TEXT("DG_MIGRATION_FUTURE_VERSION"),
			FText::Format(LOCTEXT("FutureVersionMessage", "This asset was saved with a newer DungeonGenerator asset format ({0}) than this plugin supports ({1})."), FormatVersionText(LoadedAssetVersion), FormatVersionText(LatestVersion)),
			LOCTEXT("FutureVersionHint", "Open this project with a newer DungeonGenerator plugin, or restore an older compatible copy of the asset.")
		);
	}
	else if (LoadedAssetVersion < LatestVersion)
	{
		NewState.Status = EDungeonAssetMigrationStatus::Succeeded;
		NewState.bWasMigrated = true;
		NewState.bRequiresResave = true;

		AddMessage(
			NewState,
			EDungeonMigrationMessageSeverity::Info,
			TEXT("DG_MIGRATION_IN_MEMORY"),
			LOCTEXT("InMemoryMigrationMessage", "旧バージョンの DungeonGenerator アセットを読み込んだため、現在の形式へ自動変換しました。保存前に内容を確認してください。"),
			LOCTEXT("InMemoryMigrationHint", "変換結果に問題がなければ、アセットを保存してください。")
		);

		if (LoadedAssetVersion == FDungeonGeneratorAssetVersion::BeforeCustomVersionWasAdded)
		{
			AddMessage(
				NewState,
				EDungeonMigrationMessageSeverity::Warning,
				TEXT("DG_MIGRATION_NO_CUSTOM_VERSION"),
				LOCTEXT("NoCustomVersionMessage", "This asset has no DungeonGenerator Custom Version, so it was treated as a 1.x asset."),
				LOCTEXT("NoCustomVersionHint", "Review the converted asset before saving. Some 1.x-only values may need manual adjustment.")
			);
		}

		if (HasWarnings(NewState))
		{
			NewState.Status = EDungeonAssetMigrationStatus::SucceededWithWarnings;
		}
	}

	auto& State = MigratableAsset->GetMutableDungeonAssetMigrationState();
	State = NewState;

	if (State.bRequiresResave || State.Status == EDungeonAssetMigrationStatus::Failed)
	{
		Asset->MarkPackageDirty();
		ReportMigrationMessages(Asset, State);
	}
}

bool FDungeonAssetMigrationService::IsPackageOKToSave(UPackage* Package, const FString& Filename, FOutputDevice* Error) const
{
	if (PreviousIsPackageOKToSaveDelegate.IsBound() && !PreviousIsPackageOKToSaveDelegate.Execute(Package, Filename, Error))
	{
		return false;
	}

	TArray<UObject*> MigratableAssets;
	GetMigratableAssetsInPackage(Package, MigratableAssets);

	bool bHasBlockingErrors = false;
	bool bNeedsWarningConfirmation = false;
	TArray<FString> BlockingAssetNames;
	TArray<FString> WarningAssetNames;

	for (UObject* Asset : MigratableAssets)
	{
		const auto* MigratableAsset = Cast<IDungeonMigratableAsset>(Asset);
		if (MigratableAsset == nullptr)
		{
			continue;
		}

		const auto& State = MigratableAsset->GetDungeonAssetMigrationState();
		if (State.Status == EDungeonAssetMigrationStatus::Failed || HasErrors(State))
		{
			bHasBlockingErrors = true;
			BlockingAssetNames.Add(Asset->GetPathName());
		}
		else if (State.Status == EDungeonAssetMigrationStatus::SucceededWithWarnings ||
			State.Status == EDungeonAssetMigrationStatus::RequiresSaveConfirmation ||
			HasWarnings(State))
		{
			bNeedsWarningConfirmation = true;
			WarningAssetNames.Add(Asset->GetPathName());
		}
	}

	if (bHasBlockingErrors)
	{
		const FString AssetList = FString::Join(BlockingAssetNames, TEXT("\n"));
		if (Error != nullptr)
		{
			Error->Logf(TEXT("DungeonGenerator migration errors block saving:\n%s"), *AssetList);
		}

		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(LOCTEXT("MigrationErrorBlocksSave", "DungeonGenerator migration errors block saving this package.\n\n{0}\n\nReview the DungeonGenerator Migration Message Log before saving."), FText::FromString(AssetList))
		);
		return false;
	}

	if (bNeedsWarningConfirmation)
	{
		const FString AssetList = FString::Join(WarningAssetNames, TEXT("\n"));
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::Format(LOCTEXT("MigrationWarningSaveConfirm", "This package contains DungeonGenerator assets migrated with warnings.\n\n{0}\n\nSave anyway?"), FText::FromString(AssetList))
		);
		return Response == EAppReturnType::Yes;
	}

	return true;
}

void FDungeonAssetMigrationService::OnPackageSaved(const FString& PackageFileName, UPackage* Package, FObjectPostSaveContext ObjectSaveContext)
{
	(void)PackageFileName;
	(void)ObjectSaveContext;

	TArray<UObject*> MigratableAssets;
	GetMigratableAssetsInPackage(Package, MigratableAssets);

	for (UObject* Asset : MigratableAssets)
	{
		auto* MigratableAsset = Cast<IDungeonMigratableAsset>(Asset);
		if (MigratableAsset == nullptr)
		{
			continue;
		}

		auto& State = MigratableAsset->GetMutableDungeonAssetMigrationState();
		if (State.Status != EDungeonAssetMigrationStatus::NotRequired || State.bWasMigrated || State.bRequiresResave || State.Messages.Num() > 0)
		{
			State = MakeNotRequiredState(FDungeonGeneratorAssetVersion::LatestVersion);
		}
	}
}

void FDungeonAssetMigrationService::ReportMigrationMessages(const UObject* Asset, const FDungeonAssetMigrationState& State)
{
	if (!IsValid(Asset) || State.Messages.Num() <= 0)
	{
		return;
	}

	FMessageLog MessageLog(DungeonGeneratorMigrationLogName);
	MessageLog.NewPage(FText::FromString(Asset->GetPathName()));

	for (const FDungeonMigrationMessage& Message : State.Messages)
	{
		const FText LogText = FText::Format(
			LOCTEXT("MigrationLogMessageFormat", "{0}: [{1}] {2} Hint: {3}"),
			FText::FromString(Asset->GetPathName()),
			FText::FromName(Message.Code),
			Message.Message,
			Message.FixHint
		);

		switch (Message.Severity)
		{
		case EDungeonMigrationMessageSeverity::Error:
			MessageLog.Error(LogText);
			break;

		case EDungeonMigrationMessageSeverity::Warning:
			MessageLog.Warning(LogText);
			break;

		case EDungeonMigrationMessageSeverity::Info:
		default:
			MessageLog.Info(LogText);
			break;
		}
	}

	MessageLog.Notify(LOCTEXT("MigrationMessagesAvailable", "DungeonGenerator asset migration messages are available."), EMessageSeverity::Info, true);
}

void FDungeonAssetMigrationService::GetMigratableAssetsInPackage(const UPackage* Package, TArray<UObject*>& OutAssets)
{
	OutAssets.Reset();
	if (!IsValid(Package))
	{
		return;
	}

	TArray<UObject*> PackageObjects;
	GetObjectsWithPackage(Package, PackageObjects, true);
	for (UObject* Object : PackageObjects)
	{
		if (!IsValid(Object) || Object->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}

		if (Cast<IDungeonMigratableAsset>(Object) != nullptr)
		{
			OutAssets.Add(Object);
		}
	}
}

void FDungeonAssetMigrationService::AddMessage(FDungeonAssetMigrationState& State, const EDungeonMigrationMessageSeverity Severity, const FName Code, const FText& Message, const FText& FixHint, const FName PropertyPath, const FSoftObjectPath& RelatedAsset)
{
	auto& NewMessage = State.Messages.AddDefaulted_GetRef();
	NewMessage.Severity = Severity;
	NewMessage.Code = Code;
	NewMessage.Message = Message;
	NewMessage.FixHint = FixHint;
	NewMessage.PropertyPath = PropertyPath;
	NewMessage.RelatedAsset = RelatedAsset;
}

bool FDungeonAssetMigrationService::HasErrors(const FDungeonAssetMigrationState& State)
{
	return State.Messages.ContainsByPredicate([](const FDungeonMigrationMessage& Message)
		{
			return Message.Severity == EDungeonMigrationMessageSeverity::Error;
		});
}

bool FDungeonAssetMigrationService::HasWarnings(const FDungeonAssetMigrationState& State)
{
	return State.Messages.ContainsByPredicate([](const FDungeonMigrationMessage& Message)
		{
			return Message.Severity == EDungeonMigrationMessageSeverity::Warning;
		});
}

FDungeonAssetMigrationState FDungeonAssetMigrationService::MakeNotRequiredState(const int32 LoadedAssetVersion)
{
	FDungeonAssetMigrationState State;
	State.SourceVersion = LoadedAssetVersion;
	State.TargetVersion = FDungeonGeneratorAssetVersion::LatestVersion;
	State.Status = EDungeonAssetMigrationStatus::NotRequired;
	State.bWasMigrated = false;
	State.bRequiresResave = false;
	State.Messages.Reset();
	return State;
}

#undef LOCTEXT_NAMESPACE
