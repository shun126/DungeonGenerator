/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonPartition.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"

/*
 * スレッドセーフにして下さい
 */
void UDungeonPartition::RegisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		UE::TScopeLock lock(mActivatorAndRegisteredComponentsMutex);
		RegisteredComponents.Emplace(component);
		ActivatorComponents.Emplace(component);
	}
}

/*
 * スレッドセーフにして下さい
 */
void UDungeonPartition::UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component)
{
	if (IsValid(component))
	{
		UE::TScopeLock lock(mActivatorAndRegisteredComponentsMutex);
		RegisteredComponents.Remove(component);
		ActivatorComponents.Remove(component);
	}
}

void UDungeonPartition::UpdateRegisteredComponent()
{
	check(IsInGameThread());

	for (const auto& component : RegisteredComponents)
	{
		if (mPartitionActivate)
			component->CallPartitionActivate();
		else
			component->CallPartitionInactivate();
	}
	RegisteredComponents.Reset();
}

void UDungeonPartition::CallPartitionActivate(const bool resetPartitionInactivateRemainTimer)
{
	UpdateRegisteredComponent();

	if (resetPartitionInactivateRemainTimer)
		mPartitionInactivateRemainTimer = InactivateRemainTimer;

	if (!mPartitionActivate)
	{
		mPartitionActivate = true;

		for (const auto& component : ActivatorComponents)
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
	UpdateRegisteredComponent();

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
