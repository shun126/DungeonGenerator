/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
部屋に置かれるべきアイテム
Same content as dungeon::Room::Item
*/
UENUM(BlueprintType)
enum class EDungeonRoomItem : uint8
{
	Empty,
	Key,
	UniqueKey,
};

//! 部屋に置かれるべきアイテムの種類数
static constexpr uint8 DungeonRoomItemSize = 3;

/*
部屋に置かれるべきアイテムのシンボル名を取得します
Same content as dungeon::Room::Parts
\param[in]	item	EDungeonRoomItem
\return		EDungeonRoomItemのシンボル名
*/
extern const FString& GetDungeonRoomItemName(const EDungeonRoomItem item);
