/*
連続してAddInstanceやRemoveInstanceを行う時に、開始と終了処理をまとめるためのクラス

UHierarchicalInstancedStaticMeshComponentを継承しています

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "TransactionalHierarchicalInstancedStaticMeshComponent.h"

void UTransactionalHierarchicalInstancedStaticMeshComponent::CheckAndDisableCollision()
{
	if (!mNotDirty && !mCollisionEnableDirty && mCollisionEnableControl)
	{
		mCollisionEnableDirty = true;
		SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void UTransactionalHierarchicalInstancedStaticMeshComponent::CheckAndRevertCollision()
{
	if (mCollisionEnableDirty && mCollisionEnableControl)
	{
		SetCollisionEnabled(mCollisionEnabledType);
	}
}

void UTransactionalHierarchicalInstancedStaticMeshComponent::BeginTransaction(const bool collisionEnableControl)
{
	EndTransaction(true);
	mImmediate = false;
	mNotDirty = true;
	mCollisionEnabledType = GetCollisionEnabled();
	mCollisionEnableControl = collisionEnableControl && CollisionEnabledHasPhysics(mCollisionEnabledType);
	mCollisionEnableDirty = false;
	bAutoRebuildTreeOnInstanceChanges = false;
}

void UTransactionalHierarchicalInstancedStaticMeshComponent::EndTransaction(const bool asyncBuildTree)
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

int32 UTransactionalHierarchicalInstancedStaticMeshComponent::AddInstance(const FTransform& instanceTransforms, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstance(instanceTransforms, bWorldSpace);
}

TArray<int32> UTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices, bWorldSpace);
}

bool UTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstance(int32 instanceIndex)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstance(instanceIndex);
}

bool UTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstances(const TArray<int32>& instancesToRemove)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstances(instancesToRemove);
}

void UTransactionalHierarchicalInstancedStaticMeshComponent::ClearInstances()
{
	// トランスフォーム変更に伴う更新が必要か設定
	mNotDirty = mImmediate;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	Super::ClearInstances();
}
