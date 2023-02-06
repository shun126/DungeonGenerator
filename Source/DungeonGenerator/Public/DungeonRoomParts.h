/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
部屋のパーツ
Same content as dungeon::Room::Parts
*/
UENUM(BlueprintType)
enum class EDungeonRoomParts : uint8
{
	Any,			//!< どこでも良い（dungeon::Room::Parts::Unidentifiedと同等）
	Start,			//!< スタート地点
	Goal,			//!< ゴール地点
	Hall,			//!< 広間（通路が複数つながっている）
	Hanare,			//!< 離れ（通路が一つだけつながっている）
};

//! 部屋のパーツの種類数
static constexpr uint8 DungeonRoomPartsSize = 5;

/*
部屋のパーツのシンボル名を取得します
Same content as dungeon::Room::Parts
\param[in]	parts	EDungeonRoomParts
\return		EDungeonRoomPartsのシンボル名
*/
extern const FString& GetDungeonRoomPartsName(const EDungeonRoomParts parts);
