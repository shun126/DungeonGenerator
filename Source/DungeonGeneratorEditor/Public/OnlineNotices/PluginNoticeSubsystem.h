/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <CoreMinimal.h>
#include <EditorSubsystem.h>
#include <Interfaces/IHttpRequest.h>
#include <Misc/EngineVersion.h>
#include "PluginNoticeSubsystem.generated.h"

/**
 * Message definition for notifying about breaking changes or critical compatibility issues.
 *
 * This structure is an intermediate representation for evaluating and displaying "notification messages"
 * obtained from external JSON within the Unreal Engine editor.
 *
 * Within this plugin, notifications are limited to the following purposes:
 * - Announcements of Breaking Changes
 * - Warnings of Critical Compatibility Issues (potential build failures or malfunctions)
 *
 * Messages introducing new features, roadmaps, or promotional content are not handled.
 *
 * Each message is uniquely identified by an `Id`
 * and read status is managed on the client side.
 * Messages with the same Id are displayed only once, as a rule.
 *
 * By specifying version conditions and validity periods,
 * it enables a design where notifications are sent only to affected users.
 *
 * 破壊的変更または重大な互換性問題を通知するためのメッセージ定義。
 *
 * 本構造体は、外部 JSON から取得した「通知メッセージ」を
 * Unreal Engine エディタ内で評価・表示するための中間表現です。
 *
 * 本プラグインにおいて通知は以下の用途に限定されます：
 * - 破壊的変更（Breaking Change）の予告
 * - 重大な互換性問題（ビルド失敗・動作不良の可能性）の警告
 *
 * 新機能紹介やロードマップ、宣伝目的のメッセージは扱いません。
 *
 * 各メッセージは `Id` によって一意に識別され、
 * クライアント側で既読管理されます。
 * 同一 Id のメッセージは、原則として一度のみ表示されます。
 *
 * バージョン条件や有効期間を指定することで、
 * 「影響を受ける利用者のみに通知する」設計を可能にしています。
 */
USTRUCT()
struct FPluginNoticeMessage
{
	GENERATED_BODY()

	FString Id;
	FString Severity;
	FString Title;
	FString Body;
	FString Url;
	FString MinEngineVersion;
	FString MaxEngineVersion;
	FString MinPluginVersion;
	FString MaxPluginVersion;
	FString SinceUtc;
	FString UntilUtc;
};

/**
 * 破壊的変更・重大互換性通知を管理する EditorSubsystem。
 *
 * 本サブシステムは、Unreal Engine エディタ起動時に初期化され、
 * 外部 JSON に定義された「用途限定の通知」を取得・評価・表示します。
 *
 * 【設計方針】
 * - 通知用途は「破壊的変更」「重大な互換性問題」に限定
 * - 広告・ロードマップ・宣伝目的の通知は行わない
 * - 既定ではオン（Opt-in）
 * - 通信失敗はサイレント（エディタ動作に影響を与えない）
 *
 * 【通信仕様】
 * - HTTPS GET のみ
 * - 非同期通信
 * - クエリ・個人情報・プロジェクト情報は一切送信しない
 * - OS 標準のプロキシ設定に従う
 *
 * 【表示仕様】
 * - 重要度（severity）が warning / critical のみ対象
 * - 既読メッセージは再表示しない
 * - 条件（バージョン・期間）に合致した場合のみ通知
 *
 * 【永続状態】
 * - 既読 ID
 * - 最終取得時刻
 * - ETag（If-None-Match）
 *
 * これらはユーザー単位、またはプロジェクト単位で保存されます。
 */
UCLASS()
class DUNGEONGENERATOREDITOR_API UPluginNoticeSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	struct FPluginNoticeState
	{
		TSet<FString> SeenIds;
		FDateTime LastFetchUtc;
		FString LastEtag;
	};

	struct FPluginSemVersion
	{
		TArray<int32> Parts;
		bool bValid = false;

		static FPluginSemVersion Parse(const FString& VersionString);
		int32 Compare(const FPluginSemVersion& Other) const;
	};

	void LoadState();
	void SaveState() const;
	static FString GetStateConfigFile();
	bool ShouldFetchNow() const;
	void MaybeFetchAsync();
	void StartHttpRequest();
	void OnHttpCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	static bool ParseResponseMessages(const FString& ResponseString, TArray<FPluginNoticeMessage>& OutMessages);
	bool ShouldShowMessage(const FPluginNoticeMessage& Message) const;
	static void ShowNotice(const FPluginNoticeMessage& Message);
	void MarkSeen(const FString& MessageId);
	static bool IsSameDomain(const FString& BaseUrl, const FString& TargetUrl);
	static bool TryParseIso8601(const FString& IsoString, FDateTime& OutDateTime);
	static int32 CompareEngineVersions(const FEngineVersion& Left, const FEngineVersion& Right);

	FPluginNoticeState State;
	bool bRequestInFlight = false;
};
