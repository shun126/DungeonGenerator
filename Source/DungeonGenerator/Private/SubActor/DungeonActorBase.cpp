/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonActorBase.h"
#include "Core/Helper/Crc.h"
#include <Math/TransformNonVectorized.h>

// 定義するとアライメント部分をCRC32の計算に含めない
#define EXCLUDE_ALIGNMENT_FROM_CRC32

ADungeonActorBase::ADungeonActorBase(const FObjectInitializer& initializer)
	: Super(initializer)
{
	bReplicates = false;
}

uint32_t ADungeonActorBase::GenerateCrc32(uint32_t crc) const noexcept
{
	return GenerateCrc32(this, crc);
}

uint32_t ADungeonActorBase::GenerateCrc32(const AActor* actor, uint32_t crc) noexcept
{
	if (IsValid(actor))
	{
		const auto& transform = actor->GetTransform();
		crc = GenerateCrc32(transform, crc);
	}
	return crc;
}

uint32_t ADungeonActorBase::GenerateCrc32(const FBox& box, uint32_t crc) noexcept
{
	/*
	Padding area by alignment is indefinite, so it is calculated by Min and Max.
	アライメントによるパディング領域が不定値なのでMinとMaxで計算している
	*/
	crc = GenerateCrc32(box.Min, crc);
	crc = GenerateCrc32(box.Max, crc);
	return crc;
}

uint32_t ADungeonActorBase::GenerateCrc32(const FRotator& rotator, uint32_t crc) noexcept
{
#if defined(EXCLUDE_ALIGNMENT_FROM_CRC32)
	crc = dungeon::GenerateCrc32FromData(&rotator.Pitch, sizeof(rotator.Pitch), crc);
	crc = dungeon::GenerateCrc32FromData(&rotator.Yaw, sizeof(rotator.Yaw), crc);
	crc = dungeon::GenerateCrc32FromData(&rotator.Roll, sizeof(rotator.Roll), crc);
	return crc;
#else
	return dungeon::GenerateCrc32FromData(&rotator, sizeof(FRotator), crc);
#endif
}

uint32_t ADungeonActorBase::GenerateCrc32(const FQuat& rotator, uint32_t crc) noexcept
{
#if defined(EXCLUDE_ALIGNMENT_FROM_CRC32)
	crc = dungeon::GenerateCrc32FromData(&rotator.X, sizeof(rotator.X), crc);
	crc = dungeon::GenerateCrc32FromData(&rotator.Y, sizeof(rotator.Y), crc);
	crc = dungeon::GenerateCrc32FromData(&rotator.Z, sizeof(rotator.Z), crc);
	crc = dungeon::GenerateCrc32FromData(&rotator.W, sizeof(rotator.W), crc);
	return crc;
#else
	return dungeon::GenerateCrc32FromData(&rotator, sizeof(FQuat), crc);
#endif
}

uint32_t ADungeonActorBase::GenerateCrc32(const FTransform& transform, uint32_t crc) noexcept
{
#if defined(EXCLUDE_ALIGNMENT_FROM_CRC32)
	crc = GenerateCrc32(transform.GetRotation(), crc);
	crc = GenerateCrc32(transform.GetTranslation(), crc);
	crc = GenerateCrc32(transform.GetScale3D(), crc);
	return crc;
#else
	return dungeon::GenerateCrc32FromData(&transform, sizeof(FTransform), crc);
#endif
}

uint32_t ADungeonActorBase::GenerateCrc32(const FVector& vector, uint32_t crc) noexcept
{
#if defined(EXCLUDE_ALIGNMENT_FROM_CRC32)
	crc = dungeon::GenerateCrc32FromData(&vector.X, sizeof(vector.X), crc);
	crc = dungeon::GenerateCrc32FromData(&vector.Y, sizeof(vector.Y), crc);
	crc = dungeon::GenerateCrc32FromData(&vector.Z, sizeof(vector.Z), crc);
	return crc;
#else
	return dungeon::GenerateCrc32FromData(&vector, sizeof(FVector), crc);
#endif
}
