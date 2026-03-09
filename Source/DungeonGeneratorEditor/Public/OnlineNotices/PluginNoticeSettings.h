/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>
#include "PluginNoticeSettings.generated.h"

/**
 * Configuration class for the plugin notification mechanism (for disruptive changes and critical compatibility issues only).
 *
 * This class is a configuration class for Project Settings that controls the behavior of the notification system within the plugin.
 *
 * 【Important Design Principles】
 * - Notifications are strictly limited to "disruptive changes" and "critical compatibility issues".
 * - Notifications for new feature announcements, roadmaps, advertising, or sales promotions will not be sent.
 * - External communication is disabled by default (Opt-in)
 *
 * [Communication Details]
 * - Only HTTPS GET requests are used
 * - No personal information, project details, or identifiers are transmitted
 * - Follows the OS's default proxy settings
 * - Communication failures do not affect editor operation
 *
 * プラグイン通知機構（破壊的変更・重大互換性専用）の設定クラス。
 *
 * 本クラスは、プラグイン内通知システムの挙動を制御するための
 * Project Settings 用設定クラスです。
 *
 * 【重要な設計方針】
 * - 通知の用途は「破壊的変更」および「重大な互換性問題」に限定されます
 * - 新機能紹介、ロードマップ、宣伝、販売促進目的の通知は行いません
 * - 外部通信は既定で有効（Opt-in）
 *
 * 【通信について】
 * - HTTPS GET のみを使用します
 * - 個人情報、プロジェクト情報、識別子は一切送信しません
 * - OS 標準のプロキシ設定に従います
 * - 通信失敗時はエディタ動作に影響を与えません
 */
UCLASS(config=Editor, defaultconfig, meta=(DisplayName="Online Notices"))
class DUNGEONGENERATOREDITOR_API UPluginNoticeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPluginNoticeSettings();

	virtual FName GetCategoryName() const override;

	/**
	 * Enables downloading and showing plugin notices from the configured URL.
	 *
	 * 設定したURLからプラグイン通知を取得して表示するかを切り替えます。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices")
	bool bEnableOnlineNotices;

	/**
	 * Endpoint URL used to fetch online notice data.
	 *
	 * オンライン通知データを取得するエンドポイントURLです。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices")
	FString NoticesUrl;

	/**
	 * Polling interval, in hours, for refreshing online notices.
	 *
	 * オンライン通知を再取得する間隔（時間）です。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices", meta=(ClampMin="1", UIMin="1"))
	int32 FetchIntervalHours;

	/**
	 * HTTP request timeout in seconds when fetching notices.
	 *
	 * 通知取得時のHTTPタイムアウト秒数です。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices", meta=(ClampMin="1", UIMin="1"))
	int32 HttpTimeoutSeconds;

	/**
	 * Stores read/dismiss state per user instead of sharing globally.
	 *
	 * 既読・非表示状態を全体共有ではなくユーザー単位で保持します。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices")
	bool bPerUserState;

	/**
	 * Allows opening notice links only when they match the notices URL domain.
	 *
	 * 通知URLと同一ドメインのリンクのみ開けるよう制限します。
	 */
	UPROPERTY(Config, EditAnywhere, Category="Online Notices")
	bool bRestrictLinksToSameDomain;
};


