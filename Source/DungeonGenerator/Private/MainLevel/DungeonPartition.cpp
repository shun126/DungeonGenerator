/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonPartition.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"

void UDungeonPartition::RegisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		if (mPartitionActivate)
			component->CallPartitionActivate();
		else
			component->CallPartitionInactivate();
		ActivatorComponents.Emplace(component);
	}
}

void UDungeonPartition::UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		ActivatorComponents.Remove(component);
	}
}

void UDungeonPartition::CallPartitionActivate()
{
	if (!mPartitionActivate)
	{
		mPartitionActivate = true;
		mInactivateRemainTimer = InactivateRemainTimer;

		for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
		{
			if (IsValid(component))
			{
				component->CallPartitionActivate();
			}
		}
	}
}

void UDungeonPartition::CallPartitionInactivate()
{
	if (mPartitionActivate)
	{
		mPartitionActivate = false;

		for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
		{
			if (IsValid(component))
			{
				component->CallPartitionInactivate();
			}
		}
	}
}

bool UDungeonPartition::IsEmpty() const
{
	return ActivatorComponents.Num() == 0;
}
