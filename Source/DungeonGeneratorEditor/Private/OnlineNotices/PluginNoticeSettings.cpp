/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#include "OnlineNotices/PluginNoticeSettings.h"

UPluginNoticeSettings::UPluginNoticeSettings()
	: bEnableOnlineNotices(true)
	, NoticesUrl(TEXT("https://happy-game-dev.undo.jp/plugins/DungeonGenerator/dungeon-generator-notices.json"))
	, FetchIntervalHours(24)
	, HttpTimeoutSeconds(5)
	, bPerUserState(true)
	, bRestrictLinksToSameDomain(true)
{
}

FName UPluginNoticeSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}
