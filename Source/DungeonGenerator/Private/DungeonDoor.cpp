/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonDoor.h"

ADungeonDoor::ADungeonDoor(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

void ADungeonDoor::Initialize(const EDungeonRoomProps props)
{
	Finalize();

	Props = props;
	OnInitialize(props);

	mState = State::Initialized;
}

void ADungeonDoor::Finalize()
{
	if (mState == State::Initialized)
	{
		OnFinalize();
		mState = State::Finalized;
	}
}

void ADungeonDoor::Reset()
{
	OnReset();
}

EDungeonRoomProps ADungeonDoor::GetRoomProps() const
{
	return Props;
}

void ADungeonDoor::SetRoomProps(const EDungeonRoomProps props)
{
	Props = props;
}
