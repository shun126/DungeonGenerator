/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#include "OnlineNotices/PluginNoticeSubsystem.h"
#include "OnlineNotices/PluginNoticeSettings.h"
#include "Debug/Debug.h"
#include "../PluginInformation.h"
#include <Framework/Notifications/NotificationManager.h>
#include <Interfaces/IHttpResponse.h>
#include <Interfaces/IHttpRequest.h>
#include <Interfaces/IPluginManager.h>
#include <HttpModule.h>
#include <Misc/ConfigCacheIni.h>
#include <Misc/DateTime.h>
#include <Misc/EngineVersion.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <Widgets/Notifications/SNotificationList.h>

#if DUNGEON_GENERATOR_PLUGIN_BETA_VERSION == true
#define ENABLE_LOCAL_DEBUG
#endif

namespace
{
	const TCHAR* kStateSection = TEXT("DungeonGeneratorOnlineNotices");
	const TCHAR* kSeenIdsKey = TEXT("SeenIds");
	const TCHAR* kLastFetchUtcKey = TEXT("LastFetchUtc");
	const TCHAR* kLastEtagKey = TEXT("LastEtag");
	const TCHAR* kMessagesField = TEXT("messages");
	const TCHAR* kSeverityWarning = TEXT("warning");
	const TCHAR* kSeverityCritical = TEXT("critical");
}

void UPluginNoticeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsRunningCommandlet())
	{
		return;
	}

	LoadState();
	MaybeFetchAsync();
}

void UPluginNoticeSubsystem::Deinitialize()
{
	SaveState();
	Super::Deinitialize();
}

UPluginNoticeSubsystem::FPluginSemVersion UPluginNoticeSubsystem::FPluginSemVersion::Parse(const FString& VersionString)
{
	FPluginSemVersion Result;
	TArray<FString> Tokens;
	VersionString.ParseIntoArray(Tokens, TEXT("."), true);
	if (Tokens.Num() == 0)
	{
		return Result;
	}

	for (const FString& Token : Tokens)
	{
		int32 Value = 0;
		if (!Token.IsNumeric())
		{
			return Result;
		}
		Value = FCString::Atoi(*Token);
		Result.Parts.Add(Value);
	}

	Result.bValid = Result.Parts.Num() > 0;
	return Result;
}

int32 UPluginNoticeSubsystem::FPluginSemVersion::Compare(const FPluginSemVersion& Other) const
{
	const int32 MaxParts = FMath::Max(Parts.Num(), Other.Parts.Num());
	for (int32 Index = 0; Index < MaxParts; ++Index)
	{
		const int32 Left = Parts.IsValidIndex(Index) ? Parts[Index] : 0;
		const int32 Right = Other.Parts.IsValidIndex(Index) ? Other.Parts[Index] : 0;
		if (Left != Right)
		{
			return Left < Right ? -1 : 1;
		}
	}
	return 0;
}

void UPluginNoticeSubsystem::LoadState()
{
	State = FPluginNoticeState();

	FString SeenIdsJson;
	const FString ConfigFile = GetStateConfigFile();
	GConfig->GetString(kStateSection, kSeenIdsKey, SeenIdsJson, ConfigFile);
	if (!SeenIdsJson.IsEmpty())
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SeenIdsJson);
		TArray<TSharedPtr<FJsonValue>> SeenArray;
		if (FJsonSerializer::Deserialize(Reader, SeenArray))
		{
			for (const TSharedPtr<FJsonValue>& Value : SeenArray)
			{
				FString IdString;
				if (Value.IsValid() && Value->TryGetString(IdString))
				{
					State.SeenIds.Add(IdString);
				}
			}
		}
	}

	FString LastFetchIso;
	if (GConfig->GetString(kStateSection, kLastFetchUtcKey, LastFetchIso, ConfigFile))
	{
		FDateTime Parsed;
		if (TryParseIso8601(LastFetchIso, Parsed))
		{
			State.LastFetchUtc = Parsed;
		}
	}

	GConfig->GetString(kStateSection, kLastEtagKey, State.LastEtag, ConfigFile);
}

void UPluginNoticeSubsystem::SaveState() const
{
	const FString ConfigFile = GetStateConfigFile();

	TArray<TSharedPtr<FJsonValue>> SeenValues;
	SeenValues.Reserve(State.SeenIds.Num());
	for (const FString& Id : State.SeenIds)
	{
		SeenValues.Add(MakeShared<FJsonValueString>(Id));
	}

	FString SeenIdsJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SeenIdsJson);
	FJsonSerializer::Serialize(SeenValues, Writer);

	GConfig->SetString(kStateSection, kSeenIdsKey, *SeenIdsJson, ConfigFile);

	if (State.LastFetchUtc != FDateTime())
	{
		GConfig->SetString(kStateSection, kLastFetchUtcKey, *State.LastFetchUtc.ToIso8601(), ConfigFile);
	}
	else
	{
		GConfig->RemoveKey(kStateSection, kLastFetchUtcKey, ConfigFile);
	}

	if (!State.LastEtag.IsEmpty())
	{
		GConfig->SetString(kStateSection, kLastEtagKey, *State.LastEtag, ConfigFile);
	}
	else
	{
		GConfig->RemoveKey(kStateSection, kLastEtagKey, ConfigFile);
	}

	GConfig->Flush(false, ConfigFile);
}

FString UPluginNoticeSubsystem::GetStateConfigFile()
{
	const UPluginNoticeSettings* Settings = GetDefault<UPluginNoticeSettings>();
	return (Settings && Settings->bPerUserState) ? GEditorPerProjectIni : GGameIni;
}

bool UPluginNoticeSubsystem::ShouldFetchNow() const
{
	const UPluginNoticeSettings* Settings = GetDefault<UPluginNoticeSettings>();
	if (!Settings || !Settings->bEnableOnlineNotices)
	{
		return false;
	}

	if (Settings->NoticesUrl.IsEmpty())
	{
		return false;
	}

	if (State.LastFetchUtc == FDateTime())
	{
		return true;
	}

	const FTimespan Interval = FTimespan::FromHours(Settings->FetchIntervalHours);
	const FDateTime Now = FDateTime::UtcNow();
	return (Now - State.LastFetchUtc) >= Interval;
}

void UPluginNoticeSubsystem::MaybeFetchAsync()
{
	if (bRequestInFlight)
	{
		return;
	}
#if !defined(ENABLE_LOCAL_DEBUG)
	if (!ShouldFetchNow())
	{
		return;
	}
#endif
	StartHttpRequest();
}

void UPluginNoticeSubsystem::StartHttpRequest()
{
	const UPluginNoticeSettings* Settings = GetDefault<UPluginNoticeSettings>();
	if (!Settings)
	{
		return;
	}

	FHttpModule& HttpModule = FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
	Request->SetURL(Settings->NoticesUrl);
	Request->SetVerb(TEXT("GET"));
	Request->SetTimeout(Settings->HttpTimeoutSeconds);
#if !defined(ENABLE_LOCAL_DEBUG)
	if (!State.LastEtag.IsEmpty())
	{
		Request->SetHeader(TEXT("If-None-Match"), State.LastEtag);
	}
#endif
	Request->OnProcessRequestComplete().BindUObject(this, &UPluginNoticeSubsystem::OnHttpCompleted);

	bRequestInFlight = true;
	Request->ProcessRequest();
}

void UPluginNoticeSubsystem::OnHttpCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	bRequestInFlight = false;
	State.LastFetchUtc = FDateTime::UtcNow();

	if (!bWasSuccessful || !Response.IsValid())
	{
		SaveState();
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 304)
	{
#if !defined(ENABLE_LOCAL_DEBUG)
		const FString NewEtag = Response->GetHeader(TEXT("ETag"));
		if (!NewEtag.IsEmpty())
		{
			State.LastEtag = NewEtag;
		}
		SaveState();
		return;
#endif
	}

	if (ResponseCode < 200 || ResponseCode >= 300)
	{
		SaveState();
		return;
	}

	const FString ResponseString = Response->GetContentAsString();
	TArray<FPluginNoticeMessage> Messages;
	if (!ParseResponseMessages(ResponseString, Messages))
	{
		DUNGEON_GENERATOR_EDITOR_LOG(TEXT("Failed to parse notices response."));
		SaveState();
		return;
	}

	const FString NewEtag = Response->GetHeader(TEXT("ETag"));
	if (!NewEtag.IsEmpty())
	{
		State.LastEtag = NewEtag;
	}

	for (const FPluginNoticeMessage& Message : Messages)
	{
		if (ShouldShowMessage(Message))
		{
			ShowNotice(Message);
			MarkSeen(Message.Id);
		}
	}

	SaveState();
}

bool UPluginNoticeSubsystem::ParseResponseMessages(const FString& ResponseString, TArray<FPluginNoticeMessage>& OutMessages)
{
	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* MessagesArray = nullptr;
	if (!RootObject->TryGetArrayField(kMessagesField, MessagesArray) || !MessagesArray)
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Value : *MessagesArray)
	{
		const TSharedPtr<FJsonObject> MessageObject = Value ? Value->AsObject() : nullptr;
		if (!MessageObject.IsValid())
		{
			continue;
		}

		FPluginNoticeMessage Message;
		MessageObject->TryGetStringField(TEXT("id"), Message.Id);
		MessageObject->TryGetStringField(TEXT("severity"), Message.Severity);
		MessageObject->TryGetStringField(TEXT("title"), Message.Title);
		MessageObject->TryGetStringField(TEXT("body"), Message.Body);
		MessageObject->TryGetStringField(TEXT("url"), Message.Url);
		MessageObject->TryGetStringField(TEXT("min_engine_version"), Message.MinEngineVersion);
		MessageObject->TryGetStringField(TEXT("max_engine_version"), Message.MaxEngineVersion);
		MessageObject->TryGetStringField(TEXT("min_plugin_version"), Message.MinPluginVersion);
		MessageObject->TryGetStringField(TEXT("max_plugin_version"), Message.MaxPluginVersion);
		MessageObject->TryGetStringField(TEXT("since_utc"), Message.SinceUtc);
		MessageObject->TryGetStringField(TEXT("until_utc"), Message.UntilUtc);

		if (Message.Id.IsEmpty() || Message.Severity.IsEmpty() || Message.Title.IsEmpty() || Message.Body.IsEmpty())
		{
			continue;
		}

		OutMessages.Add(MoveTemp(Message));
	}

	return true;
}

bool UPluginNoticeSubsystem::ShouldShowMessage(const FPluginNoticeMessage& Message) const
{
#if !defined(ENABLE_LOCAL_DEBUG)
	if (State.SeenIds.Contains(Message.Id))
	{
		return false;
	}
#endif

	if (!Message.Severity.Equals(kSeverityWarning, ESearchCase::IgnoreCase)
		&& !Message.Severity.Equals(kSeverityCritical, ESearchCase::IgnoreCase))
	{
		return false;
	}

	const FDateTime Now = FDateTime::UtcNow();
	if (!Message.SinceUtc.IsEmpty())
	{
		FDateTime SinceTime;
		if (TryParseIso8601(Message.SinceUtc, SinceTime) && Now < SinceTime)
		{
			return false;
		}
	}

	if (!Message.UntilUtc.IsEmpty())
	{
		FDateTime UntilTime;
		if (TryParseIso8601(Message.UntilUtc, UntilTime) && Now > UntilTime)
		{
			return false;
		}
	}

	const FEngineVersion CurrentEngineVersion = FEngineVersion::Current();
	if (!Message.MinEngineVersion.IsEmpty())
	{
		FEngineVersion MinVersion;
		if (FEngineVersion::Parse(Message.MinEngineVersion, MinVersion)
			&& CompareEngineVersions(CurrentEngineVersion, MinVersion) < 0)
		{
			return false;
		}
	}

	if (!Message.MaxEngineVersion.IsEmpty())
	{
		FEngineVersion MaxVersion;
		if (FEngineVersion::Parse(Message.MaxEngineVersion, MaxVersion)
			&& CompareEngineVersions(CurrentEngineVersion, MaxVersion) > 0)
		{
			return false;
		}
	}

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("DungeonGenerator"));
	const FString PluginVersionString = Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString();
	const FPluginSemVersion CurrentVersion = FPluginSemVersion::Parse(PluginVersionString);
	if (CurrentVersion.bValid)
	{
		if (!Message.MinPluginVersion.IsEmpty())
		{
			const FPluginSemVersion MinVersion = FPluginSemVersion::Parse(Message.MinPluginVersion);
			if (MinVersion.bValid && CurrentVersion.Compare(MinVersion) < 0)
			{
				return false;
			}
		}

		if (!Message.MaxPluginVersion.IsEmpty())
		{
			const FPluginSemVersion MaxVersion = FPluginSemVersion::Parse(Message.MaxPluginVersion);
			if (MaxVersion.bValid && CurrentVersion.Compare(MaxVersion) > 0)
			{
				return false;
			}
		}
	}

	return true;
}

void UPluginNoticeSubsystem::ShowNotice(const FPluginNoticeMessage& Message)
{
	FNotificationInfo Info(FText::FromString(Message.Title));
	Info.SubText = FText::FromString(Message.Body);
	Info.bFireAndForget = false;
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.2f;
	Info.ExpireDuration = 0.0f;

	const UPluginNoticeSettings* Settings = GetDefault<UPluginNoticeSettings>();
	const bool bHasUrl = Settings && !Message.Url.IsEmpty();
	const bool bUrlIsHttp = bHasUrl && (Message.Url.StartsWith(TEXT("https://")) || Message.Url.StartsWith(TEXT("http://")));
	const bool bShowUrlButton = bUrlIsHttp && (!Settings->bRestrictLinksToSameDomain || IsSameDomain(Settings->NoticesUrl, Message.Url));

	if (bShowUrlButton)
	{
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			FText::FromString(TEXT("詳細を見る")),
			FText::FromString(TEXT("Open notice details")),
			FSimpleDelegate::CreateLambda([Url = Message.Url]()
			{
				FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
			})
		));
	}

	TSharedRef<TWeakPtr<SNotificationItem>> NotificationHandle = MakeShared<TWeakPtr<SNotificationItem>>();
	Info.ButtonDetails.Add(FNotificationButtonInfo(
		FText::FromString(TEXT("閉じる")),
		FText::FromString(TEXT("Close")),
		FSimpleDelegate::CreateLambda([NotificationHandle]()
		{
			if (TSharedPtr<SNotificationItem> Notification = NotificationHandle->Pin())
			{
				Notification->SetCompletionState(SNotificationItem::CS_None);
				Notification->ExpireAndFadeout();
			}
		})
	));

	*NotificationHandle = FSlateNotificationManager::Get().AddNotification(Info);
	if (TSharedPtr<SNotificationItem> Notification = NotificationHandle->Pin())
	{
		Notification->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void UPluginNoticeSubsystem::MarkSeen(const FString& MessageId)
{
	if (!MessageId.IsEmpty())
	{
		State.SeenIds.Add(MessageId);
	}
}

bool UPluginNoticeSubsystem::IsSameDomain(const FString& BaseUrl, const FString& TargetUrl)
{
	auto ExtractDomain = [](const FString& Url) -> FString
	{
		FString WorkingUrl = Url;
		WorkingUrl.TrimStartAndEndInline();

		int32 SchemeIndex = INDEX_NONE;
		if (WorkingUrl.FindChar(TEXT(':'), SchemeIndex))
		{
			WorkingUrl = WorkingUrl.Mid(SchemeIndex + 1);
			WorkingUrl.TrimStartAndEndInline();
			if (WorkingUrl.StartsWith(TEXT("//")))
			{
				WorkingUrl.RightChopInline(2);
			}
		}

		int32 SlashIndex = INDEX_NONE;
		if (WorkingUrl.FindChar(TEXT('/'), SlashIndex))
		{
			WorkingUrl = WorkingUrl.Left(SlashIndex);
		}

		int32 PortIndex = INDEX_NONE;
		if (WorkingUrl.FindChar(TEXT(':'), PortIndex))
		{
			WorkingUrl = WorkingUrl.Left(PortIndex);
		}

		WorkingUrl.TrimStartAndEndInline();
		return WorkingUrl.ToLower();
	};

	const FString BaseDomain = ExtractDomain(BaseUrl);
	const FString TargetDomain = ExtractDomain(TargetUrl);
	if (BaseDomain.IsEmpty() || TargetDomain.IsEmpty())
	{
		return false;
	}

	return BaseDomain.Equals(TargetDomain, ESearchCase::IgnoreCase);
}

bool UPluginNoticeSubsystem::TryParseIso8601(const FString& IsoString, FDateTime& OutDateTime)
{
	return FDateTime::ParseIso8601(*IsoString, OutDateTime);
}

int32 UPluginNoticeSubsystem::CompareEngineVersions(const FEngineVersion& Left, const FEngineVersion& Right)
{
	if (Left.GetMajor() != Right.GetMajor())
	{
		return Left.GetMajor() < Right.GetMajor() ? -1 : 1;
	}

	if (Left.GetMinor() != Right.GetMinor())
	{
		return Left.GetMinor() < Right.GetMinor() ? -1 : 1;
	}

	if (Left.GetPatch() != Right.GetPatch())
	{
		return Left.GetPatch() < Right.GetPatch() ? -1 : 1;
	}

	if (Left.GetChangelist() != Right.GetChangelist())
	{
		return Left.GetChangelist() < Right.GetChangelist() ? -1 : 1;
	}

	return 0;
}
