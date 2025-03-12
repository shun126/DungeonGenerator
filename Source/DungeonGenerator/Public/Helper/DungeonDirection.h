/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonDirection.generated.h"

/*
Enumerated type indicating the direction of the grid in the dungeon
Even numbers represent north-south and odd numbers represent east-west.

ダンジョン内のグリッドの方向を示す列挙型
偶数は南北を奇数は東西を表す。
編集した場合はdungeon::Direction,dungeon::PathFinder::SearchDirectionの並びを合わせて下さい
*/
UENUM(BlueprintType)
enum class EDungeonDirection : uint8
{
	North,
	East,
	South,
	West,
};
