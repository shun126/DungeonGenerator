/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonPartiation.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"

void UDungeonPartiation::RegisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		if (mPartiationActivate)
			component->CallPartiationActivate();
		else
			component->CallPartiationInactivate();
		ActivatorComponents.Emplace(component);
	}
}

void UDungeonPartiation::UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		ActivatorComponents.Remove(component);
	}
}

void UDungeonPartiation::CallPartiationActivate()
{
	if (!mPartiationActivate)
	{
		mPartiationActivate = true;
		mActivateRemainTimer = ActivateRemainTimer;

		for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
		{
			if (IsValid(component))
				component->CallPartiationActivate();
		}
	}
}

void UDungeonPartiation::CallPartiationInactivate()
{
	if (mPartiationActivate)
	{
		mPartiationActivate = false;

		for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
		{
			if (IsValid(component))
				component->CallPartiationInactivate();
		}
	}
}
