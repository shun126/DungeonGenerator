/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
部屋に置かれるべき配置物
Same content as dungeon::Grid::Props
*/
UENUM(BlueprintType)
enum class EDungeonRoomProps : uint8
{
	None,
	Lock,
	UniqueLock,
};

//! 部屋に置かれるべき配置物の種類数
static constexpr uint8 DungeonRoomPropsSize = 3;

/*
部屋に置かれるべき配置物のシンボル名を取得します
Same content as dungeon::Room::Parts
\param[in]	props	EDungeonRoomProps
\return		EDungeonRoomPropsのシンボル名
*/
extern const FString& GetDungeonRoomPropsName(const EDungeonRoomProps props);
