/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Kismet/BlueprintFunctionLibrary.h>
#include "DungeonBlueprint.generated.h"

/**
BluePrint function library class
*/
UCLASS(Blueprintable)
class UDungeonBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetPluginVersion() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetDocumentURL() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetSupportURL() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetBuildTag() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetCommitID() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetIdentifier() noexcept;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		static const FString& GetLicenseType() noexcept;

	// Transform from world location to texture location
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator/Minimap")
		static FVector2D TransformWorldToTexture(const FVector worldLocation, const float worldToTextureScale) noexcept;
};
