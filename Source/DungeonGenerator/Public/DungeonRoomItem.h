/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
�����ɒu�����ׂ��A�C�e��
Same content as dungeon::Room::Item
*/
UENUM(BlueprintType)
enum class EDungeonRoomItem : uint8
{
	Empty,
	Key,
	UniqueKey,
};

//! �����ɒu�����ׂ��A�C�e���̎�ސ�
static constexpr uint8 DungeonRoomItemSize = 3;

/*
�����ɒu�����ׂ��A�C�e���̃V���{�������擾���܂�
Same content as dungeon::Room::Parts
\param[in]	item	EDungeonRoomItem
\return		EDungeonRoomItem�̃V���{����
*/
extern const FString& GetDungeonRoomItemName(const EDungeonRoomItem item);
