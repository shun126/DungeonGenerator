/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonRoomProps.generated.h"

/*
Objects to be placed in the room
Same content as dungeon::Grid::Props
部屋に配置するオブジェクト
dungeon::Grid::Props と同じ内容にして下さい。
*/
UENUM(BlueprintType)
enum class EDungeonRoomProps : uint8
{
	None,
	Lock,
	UniqueLock,
};

/*
Number of different types of placements to be placed in the room
室内に配置するプレースメントの種類数
*/
static constexpr uint8 DungeonRoomPropsSize = static_cast<uint8>(EDungeonRoomProps::UniqueLock) + 1;

/*
Gets the symbolic name of the placement that should be placed in the room
Same content as dungeon::Room::Parts
部屋に置くべき配置のシンボル名を取得する
dungeon::Grid::Parts と同じ内容にして下さい。
@param[in]	props	EDungeonRoomProps
@return		EDungeonRoomProps symbol name
*/
extern const FString& GetDungeonRoomPropsName(const EDungeonRoomProps props);
