/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "SubActor/DungeonDoor.h"

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
