/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonDoorBase.h"
#include "Core/Helper/Crc.h"
#include "Core/Math/Random.h"

ADungeonDoorBase::ADungeonDoorBase(const FObjectInitializer& initializer)
	: Super(initializer)
{
	bReplicates = true;
}

void ADungeonDoorBase::InvokeInitialize(const std::shared_ptr<dungeon::Random>& random, const EDungeonRoomProps props)
{
	if (mState != State::Initialized)
	{
		Props = props;
		mRandom.SetOwner(random);
		OnNativeInitialize(props);
		OnInitialize(props);
		mState = State::Initialized;
	}
}

void ADungeonDoorBase::InvokeFinalize(const bool finish)
{
	if (mState == State::Initialized)
	{
		OnNativeFinalize();
		OnFinalize(finish);
		mRandom.ResetOwner();
		mState = State::Finalized;
	}
}

uint32_t ADungeonDoorBase::GenerateCrc32(uint32_t crc) const noexcept
{
	if (IsValid(this))
	{
		const FTransform& transform = GetTransform();
		crc = ADungeonVerifiableActor::GenerateCrc32(transform, crc);
		crc = dungeon::GenerateCrc32FromData(&Props, sizeof(EDungeonRoomProps), crc);
	}
	return crc;
}
