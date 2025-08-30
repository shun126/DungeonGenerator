/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include <CoreMinimal.h>
#include <Containers/Set.h>
#include <Misc/SpinLock.h>
#include <functional>
#include "DungeonPartition.generated.h"

class ADungeonMainLevelScriptActor;

/**
This class represents the area delimited by the dungeon.
ActivateDuneComponentActivatorComponent if it is a partition around a player.
If the partition is away from the player, deactivate it.

ダンジョンを区切った領域を表すクラスです。
プレイヤー周辺のパーティションならDungeonComponentActivatorComponentをアクティブ化します。
プレイヤーから離れているパーティションなら非アクティブ化します。
*/
UCLASS()
class DUNGEONGENERATOR_API UDungeonPartition : public UObject
{
	GENERATED_BODY()

	/**
	Time from inactivity target to deactivation.
	非アクティブ化対象になってから実際に非アクティブ化するまでの秒数。
	*/
	static constexpr float InactivateRemainTimer = 3.f;

public:
	explicit UDungeonPartition(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonPartition() override = default;

private:
	void RegisterActivatorComponent(UDungeonComponentActivatorComponent* component);
	void UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component);
	void UpdateRegisteredComponent();

	bool UpdatePartitionInactivateRemainTimer(const float deltaSeconds);
	bool IsValidPartitionInactivateRemainTimer() const noexcept;

	void Mark() noexcept;
	void Unmark() noexcept;
	bool IsMarked() const noexcept;
	void CallPartitionActivate(const bool resetPartitionInactivateRemainTimer);
	void CallPartitionInactivate();

	bool IsEmpty() const;
	void EachDungeonComponentActivatorComponent(const std::function<void(UDungeonComponentActivatorComponent*)>& function) const
	{
		for (UDungeonComponentActivatorComponent* component : ActivatorComponents)
		{
			if (IsValid(component))
				function(component);
		}
	}

protected:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> ActivatorComponents;

	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> RegisteredComponents;

private:
	float mPartitionInactivateRemainTimer = 0.f;
	bool mPartitionActivate = true;
	bool mMarked = false;
	UE::FSpinLock mActivatorAndRegisteredComponentsMutex;

	friend class ADungeonMainLevelScriptActor;
	friend class UDungeonComponentActivatorComponent;
};

inline UDungeonPartition::UDungeonPartition(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline bool UDungeonPartition::UpdatePartitionInactivateRemainTimer(const float deltaSeconds)
{
	if (mPartitionInactivateRemainTimer > 0.f)
	{
		mPartitionInactivateRemainTimer -= deltaSeconds;
	}
	return mPartitionInactivateRemainTimer > 0.f;
}

inline bool UDungeonPartition::IsValidPartitionInactivateRemainTimer() const noexcept
{
	return mPartitionInactivateRemainTimer > 0.f;
}

inline void UDungeonPartition::Mark() noexcept
{
	mMarked = true;
}

inline void UDungeonPartition::Unmark() noexcept
{
	mMarked = false;
}

inline bool UDungeonPartition::IsMarked() const noexcept
{
	return mMarked;
}
