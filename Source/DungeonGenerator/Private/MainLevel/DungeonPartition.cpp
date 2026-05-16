/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "MainLevel/DungeonPartition.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"

/*
 * UDungeonComponentActivatorComponentのTickComponentは
 * GameThread以外からも実行されるのでスレッドセーフにして下さい
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
 * UDungeonComponentActivatorComponentのTickComponentは
 * GameThread以外からも実行されるのでスレッドセーフにして下さい
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

void UDungeonPartition::FlushRegisteredComponents()
{
	UpdateRegisteredComponent();
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

void UDungeonPartition::ResetGraphData()
{
	mCellCoordinate = FIntVector::ZeroValue;
	mBounds.Init();
	mNeighborIndices.Reset();
}

void UDungeonPartition::SetCellCoordinate(const FIntVector& cellCoordinate) noexcept
{
	mCellCoordinate = cellCoordinate;
}

const FIntVector& UDungeonPartition::GetCellCoordinate() const noexcept
{
	return mCellCoordinate;
}

void UDungeonPartition::SetBounds(const FBox& bounds) noexcept
{
	mBounds = bounds;
}

const FBox& UDungeonPartition::GetBounds() const noexcept
{
	return mBounds;
}

void UDungeonPartition::AddNeighborIndex(const int32 neighborIndex)
{
	if (neighborIndex >= 0)
		mNeighborIndices.AddUnique(neighborIndex);
}

const TArray<int32>& UDungeonPartition::GetNeighborIndices() const noexcept
{
	return mNeighborIndices;
}

void UDungeonPartition::EachDungeonComponentActivatorComponent(const std::function<void(UDungeonComponentActivatorComponent*)>& function) const
{
	for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
	{
		if (IsValid(component))
			function(component);
	}
}
