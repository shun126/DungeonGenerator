/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Containers/Set.h>
#include "DungeonPartition.generated.h"

class ADungeonMainLevelScriptActor;
class UDungeonComponentActivatorComponent;

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
	static constexpr float ActivateRemainTimer = 5.f;

public:
	explicit UDungeonPartition(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonPartition() override = default;

private:
	void RegisterActivatorComponent(UDungeonComponentActivatorComponent* component);
	void UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component);

	bool UpdateActivateRemainTimer(const float deltaSeconds);
	bool IsValidActivateRemainTimer() const noexcept;

	void Mark() noexcept;
	void Unmark() noexcept;
	bool IsMarked() const noexcept;

	void CallPartitionActivate();
	void CallPartitionInactivate();

protected:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> ActivatorComponents;

private:
	float mActivateRemainTimer = 0.f;
	bool mPartitionActivate = true;
	bool mMarked = false;

	friend class ADungeonMainLevelScriptActor;
	friend class UDungeonComponentActivatorComponent;
};

inline UDungeonPartition::UDungeonPartition(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline bool UDungeonPartition::UpdateActivateRemainTimer(const float deltaSeconds)
{
	if (mActivateRemainTimer > 0.f)
	{
		mActivateRemainTimer -= deltaSeconds;
	}
	return mActivateRemainTimer > 0.f;
}

inline bool UDungeonPartition::IsValidActivateRemainTimer() const noexcept
{
	return mActivateRemainTimer > 0.f;
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
