/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Containers/Set.h>
#include <functional>
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

	/**
	 * パーティエーションの状態
	 * 大きいほど優先
	 */
	enum class ActiveState : uint8
	{
		Inactivate,		//!< 非アクティブ
		ActivateFar,	//!< アクティブ（遠景）
		ActivateNear,	//!< アクティブ（近景）
	};

public:
	/**
	 * パーティエーションの状態になった理由
	 * 大きいほど優先
	 */
	enum class ActiveReason : uint8
	{
		Unknown,		//!< 未定
		Bounding,		//!< バウンディングボックス
		ViewFrustum,	//!< 視錐台
	};

	explicit UDungeonPartition(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonPartition() override = default;

private:
	void RegisterActivatorComponent(UDungeonComponentActivatorComponent* component);
	void UnregisterActivatorComponent(UDungeonComponentActivatorComponent* component);

	bool UpdateInactivateRemainTimer(const float deltaSeconds);
	bool IsValidInactivateRemainTimer() const noexcept;

	void Mark(const ActiveState activeState, const ActiveReason activeReason) noexcept;
	void Unmark() noexcept;
	ActiveState IsMarked() const noexcept;
	ActiveReason GetActiveStateType() const;

	void CallPartitionActivate();
	void CallPartitionInactivate();

	bool IsEmpty() const;

	void EachDungeonComponentActivatorComponent(const std::function<void(UDungeonComponentActivatorComponent*)>& function) const
	{
		for (const auto& dungeonComponentActivatorComponent : ActivatorComponents)
		{
			function(dungeonComponentActivatorComponent);
		}
	}

protected:
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> ActivatorComponents;

private:
	float mInactivateRemainTimer = 0.f;
	bool mPartitionActivate = true;
	ActiveState mMarked = ActiveState::Inactivate;
	ActiveReason mActiveStateType = ActiveReason::Unknown;

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

inline void UDungeonPartition::Mark(const ActiveState activeState, const ActiveReason activeReason) noexcept
{
	if (mMarked < activeState)
		mMarked = activeState;
	mActiveStateType = activeReason;
}

inline void UDungeonPartition::Unmark() noexcept
{
	mMarked = ActiveState::Inactivate;
	mActiveStateType = ActiveReason::Unknown;
}

inline UDungeonPartition::ActiveState UDungeonPartition::IsMarked() const noexcept
{
	return mMarked;
}

inline UDungeonPartition::ActiveReason UDungeonPartition::GetActiveStateType() const
{
	return mActiveStateType;
}
