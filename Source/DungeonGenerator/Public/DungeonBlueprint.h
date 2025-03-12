/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include "DungeonBlueprint.generated.h"

class ADungeonRoomSensorBase;

/**
BluePrint function library
ダンジョン生成ブループリント関数
*/
UCLASS()
class UDungeonBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	Get the version of the plugin
	プラグインのバージョンを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetPluginVersion() noexcept;

	/**
	Get the URL of the plugin document
	プラグインドキュメントのURLを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetDocumentURL() noexcept;

	/**
	Get the URL of the plugin support page
	プラグインサポートページのURLを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetSupportURL() noexcept;

	/**
	Get build tag
	ビルドタグを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetBuildTag() noexcept;

	/**
	Get commit ID
	コミットIDを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetCommitID() noexcept;

	/**
	Get the plugin identifier
	プラグインの識別子を取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetIdentifier() noexcept;

	/**
	Get License Type
	ライセンスを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static const FString& GetLicenseType() noexcept;

	/**
	Transform from world location to texture location
	ワールドの位置からテクスチャの位置へ変換します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static FVector2D TransformWorldToTexture(const FVector worldLocation, const float worldToTextureScale) noexcept;

	/**
	Find DungeonRoomSensorBase by location
	位置からDungeonRoomSensorBaseを検索します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (WorldContext = "worldContextObject"))
	static ADungeonRoomSensorBase* FindDungeonRoomSensorByLocation(const UObject* worldContextObject, const FVector location) noexcept;

	/**
	Find DungeonRoomSensorBase from the player
	プレイヤーからDungeonRoomSensorBaseを検索します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (WorldContext = "worldContextObject"))
	static ADungeonRoomSensorBase* FindDungeonRoomSensorFromPlayer(const UObject* worldContextObject, const int32 playerIndex) noexcept;
};
