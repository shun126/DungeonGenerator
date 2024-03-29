/*
連続してAddInstanceやRemoveInstanceを行う時に、開始と終了処理をまとめるためのクラス

UHierarchicalInstancedStaticMeshComponentを継承しています

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.h"

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::CheckAndDisableCollision()
{
	if (!mNotDirty && !mCollisionEnableDirty && mCollisionEnableControl)
	{
		mCollisionEnableDirty = true;
		SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::CheckAndRevertCollision()
{
	if (mCollisionEnableDirty && mCollisionEnableControl)
	{
		SetCollisionEnabled(mCollisionEnabledType);
	}
}

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::BeginTransaction(const bool collisionEnableControl)
{
	EndTransaction(true);
	mImmediate = false;
	mNotDirty = true;
	mCollisionEnabledType = GetCollisionEnabled();
	mCollisionEnableControl = collisionEnableControl && CollisionEnabledHasPhysics(mCollisionEnabledType);
	mCollisionEnableDirty = false;
	bAutoRebuildTreeOnInstanceChanges = false;
}

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::EndTransaction(const bool asyncBuildTree)
{
	if (mImmediate == false && mNotDirty == false)
	{
		CheckAndRevertCollision();

		mImmediate = true;

		bAutoRebuildTreeOnInstanceChanges = true;
		MarkRenderStateDirty();
		BuildTreeIfOutdated(asyncBuildTree, false);
	}
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
int32 UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstance(const FTransform& instanceTransforms)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstance(instanceTransforms);
}

TArray<int32> UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices);
}
#else
int32 UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstance(const FTransform& instanceTransforms, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstance(instanceTransforms, bWorldSpace);
}

TArray<int32> UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices, bWorldSpace);
}
#endif

bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstance(int32 instanceIndex)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstance(instanceIndex);
}

#if UE_VERSION_NEWER_THAN(5, 0, 0)
bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstances(const TArray<int32>& instancesToRemove)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstances(instancesToRemove);
}
#endif

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::ClearInstances()
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	Super::ClearInstances();
}
