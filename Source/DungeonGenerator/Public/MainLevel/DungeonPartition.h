/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include <CoreMinimal.h>
#include <Containers/Set.h>
#include <Math/Box.h>
#include <Misc/SpinLock.h>
#include <functional>
#include "DungeonPartition.generated.h"

class ADungeonMainLevelScriptActor;

/**
 * This class represents the area delimited by the dungeon.
 * ActivateDuneComponentActivatorComponent if it is a partition around a player.
 * If the partition is away from the player, deactivate it.
 *
 * ダンジョンを区切った領域を表すクラスです。
 * プレイヤー周辺のパーティションならDungeonComponentActivatorComponentをアクティブ化します。
 * プレイヤーから離れているパーティションなら非アクティブ化します。
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonPartition : public UObject
{
	GENERATED_BODY()

	/**
	 * Time from inactivity target to deactivation.
	 * 非アクティブ化対象になってから実際に非アクティブ化するまでの秒数。
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
	void ResetPartitionInactivateRemainTimer() noexcept;
	bool IsPartitionActivate() const noexcept;
	void FlushRegisteredComponents();

	void Mark() noexcept;
	void Unmark() noexcept;
	bool IsMarked() const noexcept;
	void CallPartitionActivate(const bool resetPartitionInactivateRemainTimer);
	void CallPartitionInactivate();

	bool IsEmpty() const;
	void ResetGraphData();
	void SetCellCoordinate(const FIntVector& cellCoordinate) noexcept;
	const FIntVector& GetCellCoordinate() const noexcept;
	void SetBounds(const FBox& bounds) noexcept;
	const FBox& GetBounds() const noexcept;
	void AddNeighborIndex(const int32 neighborIndex);
	const TArray<int32>& GetNeighborIndices() const noexcept;
	void EachDungeonComponentActivatorComponent(const std::function<void(UDungeonComponentActivatorComponent*)>& function) const;

protected:
	/**
	 * Runtime list of activator components currently associated with this partition.
	 *
	 * このパーティションに現在紐づいているアクティベータコンポーネントの実行時リストです。
	 */
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> ActivatorComponents;

	/**
	 * Components registered under this partition for activation management.
	 *
	 * アクティブ制御のためにこのパーティションへ登録されたコンポーネント一覧です。
	 */
	UPROPERTY(Transient)
	TSet<TObjectPtr<UDungeonComponentActivatorComponent>> RegisteredComponents;

private:
	UE::FSpinLock mActivatorAndRegisteredComponentsMutex;
	FIntVector mCellCoordinate = FIntVector::ZeroValue;
	FBox mBounds;
	TArray<int32> mNeighborIndices;
	float mPartitionInactivateRemainTimer = 0.f;
	bool mPartitionActivate = true;
	bool mMarked = false;

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

inline void UDungeonPartition::ResetPartitionInactivateRemainTimer() noexcept
{
	mPartitionInactivateRemainTimer = InactivateRemainTimer;
}

inline bool UDungeonPartition::IsPartitionActivate() const noexcept
{
	return mPartitionActivate;
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


