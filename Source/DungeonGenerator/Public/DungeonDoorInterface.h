/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomProps.h"
#include <UObject/Interface.h>
#include "DungeonDoorInterface.generated.h"

/*
ダンジョンドアインターフェイスクラス
*/
UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonDoorInterface : public UInterface
{
	GENERATED_BODY()
};

/*
ダンジョンドアインターフェイスクラス
*/
class DUNGEONGENERATOR_API IDungeonDoorInterface
{
	GENERATED_BODY()

public:
	// 引数と戻り値がないメンバ関数
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="DungeonGenerator")
    	void OnInitialize(EDungeonRoomProps props);

	// 引数と戻り値があるメンバ関数
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "DungeonGenerator")
    	void OnReset();
};
