/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>

/*
�����ɒu�����ׂ��z�u��
Same content as dungeon::Grid::Props
*/
UENUM(BlueprintType)
enum class EDungeonRoomProps : uint8
{
	None,
	Lock,
	UniqueLock,
};

//! �����ɒu�����ׂ��z�u���̎�ސ�
static constexpr uint8 DungeonRoomPropsSize = 3;

/*
�����ɒu�����ׂ��z�u���̃V���{�������擾���܂�
Same content as dungeon::Room::Parts
\param[in]	props	EDungeonRoomProps
\return		EDungeonRoomProps�̃V���{����
*/
extern const FString& GetDungeonRoomPropsName(const EDungeonRoomProps props);
