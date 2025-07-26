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
	static constexpr float InactivateRemainTimer = 3.f;

	enum class ActiveState : uint8
	{
		Inactivate,
		ActivateFar,
		ActivateNear,
	};

public:
#if WITH_EDITORONLY_DATA
	enum class ActiveStateType : uint8
	{
		Inactivate,
		Bounding,
		ViewFrustum,
		Segment,
	};
#endif

	explicit UDungeonPartition(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonPartition() override = default;

private:
	void RegisterActivatorComponent(UDungeonComponentActivatorComponent* component);
	void UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component);

	bool UpdateInactivateRemainTimer(const float deltaSeconds);
	bool IsValidInactivateRemainTimer() const noexcept;

	void Mark(const ActiveState activeState) noexcept;
	void Unmark() noexcept;
	ActiveState IsMarked() const noexcept;

#if WITH_EDITOR
	ActiveStateType GetActiveStateType() const;
	void SetActiveStateType(const ActiveStateType activeStateType);
#endif

	void CallPartitionActivate();
	void CallPartitionInactivate();

	void CallCastShadowActivate();
	void CallCastShadowInactivate();

	bool IsEmpty() const;

protected:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> ActivatorComponents;

private:
	float mInactivateRemainTimer = 0.f;
	bool mPartitionActivate = true;
	bool mCallCastShadowActivate = true;
	ActiveState mMarked = ActiveState::Inactivate;

#if WITH_EDITORONLY_DATA
	ActiveStateType mActiveStateType = ActiveStateType::Inactivate;
#endif

	friend class ADungeonMainLevelScriptActor;
	friend class UDungeonComponentActivatorComponent;
};

inline UDungeonPartition::UDungeonPartition(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline bool UDungeonPartition::UpdateInactivateRemainTimer(const float deltaSeconds)
{
	if (mInactivateRemainTimer > 0.f)
	{
		mInactivateRemainTimer -= deltaSeconds;
	}
	return mInactivateRemainTimer > 0.f;
}

inline bool UDungeonPartition::IsValidInactivateRemainTimer() const noexcept
{
	return mInactivateRemainTimer > 0.f;
}

inline void UDungeonPartition::Mark(const ActiveState activeState) noexcept
{
	if (mMarked < activeState)
		mMarked = activeState;
}

inline void UDungeonPartition::Unmark() noexcept
{
	mMarked = ActiveState::Inactivate;
}

inline UDungeonPartition::ActiveState UDungeonPartition::IsMarked() const noexcept
{
	return mMarked;
}

#if WITH_EDITOR
inline UDungeonPartition::ActiveStateType UDungeonPartition::GetActiveStateType() const
{
	return mActiveStateType;
}

inline void UDungeonPartition::SetActiveStateType(const ActiveStateType activeStateType)
{
	mActiveStateType = activeStateType;
}
#endif
