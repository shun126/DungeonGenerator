/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonBlueprint.h"
#include "PluginInfomation.h"
#include "Core/Debug/BuildInfomation.h"
#include <Math/UnrealMathUtility.h>

const FString& UDungeonBlueprint::GetPluginVersion() noexcept
{
	static const FString text(TEXT(DUNGENERATOR_PLUGIN_VERSION_NAME));
	return text;
}

const FString& UDungeonBlueprint::GetDocumentURL() noexcept
{
	static const FString text(TEXT(DUNGENERATOR_PLUGIN_DOCS_URL));
	return text;
}

const FString& UDungeonBlueprint::GetSupportURL() noexcept
{
	static const FString text(TEXT(DUNGENERATOR_PLUGIN_SUPPORT_URL));
	return text;
}

const FString& UDungeonBlueprint::GetBuildTag() noexcept
{
	static const FString text(TEXT(JENKINS_BUILD_TAG));
	return text;
}

const FString& UDungeonBlueprint::GetCommitID() noexcept
{
	static const FString text(TEXT(JENKINS_GIT_COMMIT));
	return text;
}

const FString& UDungeonBlueprint::GetIdentifier() noexcept
{
	static const FString text(TEXT(JENKINS_UUID));
	return text;
}

const FString& UDungeonBlueprint::GetLicenseType() noexcept
{
	static const FString text(TEXT(JENKINS_LICENSE));
	return text;
}

FVector2D UDungeonBlueprint::TransformWorldToTexture(const FVector worldLocation, const float worldToTextureScale) noexcept
{
	return FVector2D(worldLocation.X, worldLocation.Y) * worldToTextureScale;
}
