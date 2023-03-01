/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomProps.h"
#include <UObject/Interface.h>
#include "DungeonDoorInterface.generated.h"

/**
Dungeon Door Interface Class
*/
UINTERFACE(MinimalAPI, Blueprintable)
class UDungeonDoorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
Dungeon Door Interface Class
*/
class DUNGEONGENERATOR_API IDungeonDoorInterface
{
	GENERATED_BODY()

public:
	/**
	Function called during initialization after object creation
	\param[in]	props	Types of props that need to be installed on the door
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "DungeonGenerator")
    	void OnInitialize(EDungeonRoomProps props);

	/**
	Function called on reset
	*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "DungeonGenerator")
    	void OnReset();
};
