/**
連続してAddInstanceやRemoveInstanceを行う時に、開始と終了処理をまとめるためのクラス

UHierarchicalInstancedStaticMeshComponentを継承しています

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Component/DungeonTransactionalHierarchicalInstancedStaticMeshComponent.h"

#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::BeginTransaction(const bool collisionEnableControl)
#else
void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::BeginTransaction()
#endif
{
	if (mImmediate == true)
	{
		mImmediate = false;
		mDirty = false;
#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
		mCollisionEnabledType = GetCollisionEnabled();
		mCollisionEnableControl = collisionEnableControl && CollisionEnabledHasPhysics(mCollisionEnabledType);
		mCollisionEnableDirty = false;
#endif
		mAutoRebuildTreeOnInstanceChanges = bAutoRebuildTreeOnInstanceChanges;
		bAutoRebuildTreeOnInstanceChanges = false;
	}
}

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::CheckAndDisableCollision()
{
#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
	if (mCollisionEnableDirty == false && mCollisionEnableControl == true)
	{
		mCollisionEnableDirty = true;
		SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
#endif
}

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::EndTransaction(const bool asyncBuildTree)
{
#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
	if (mCollisionEnableDirty == true && mCollisionEnableControl == true)
	{
		SetCollisionEnabled(mCollisionEnabledType);
	}
#endif
	if (mImmediate == false)
	{
		mImmediate = true;
		bAutoRebuildTreeOnInstanceChanges = mAutoRebuildTreeOnInstanceChanges;

		if (mDirty == true)
		{
			MarkRenderStateDirty();
			BuildTreeIfOutdated(asyncBuildTree, false);
		}
	}
}

#if UE_VERSION_OLDER_THAN(5, 0, 0)
int32 UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstance(const FTransform& instanceTransforms)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = ture;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstance(instanceTransforms);
}

TArray<int32> UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = ture;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices);
}
#else
int32 UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstance(const FTransform& instanceTransforms, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstance(instanceTransforms, bWorldSpace);
}

#if UE_VERSION_OLDER_THAN(5, 4, 0)
TArray<int32> UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices, bWorldSpace);
}
#else
TArray<int32> UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace, bool bUpdateNavigation)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::AddInstances(instanceTransforms, bShouldReturnIndices, bWorldSpace, bUpdateNavigation);
}
#endif
#endif

bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstance(int32 instanceIndex)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstance(instanceIndex);
}

#if UE_VERSION_NEWER_THAN(5, 0, 0)
bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::RemoveInstances(const TArray<int32>& instancesToRemove)
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	return Super::RemoveInstances(instancesToRemove);
}
#endif

void UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::ClearInstances()
{
	// トランスフォーム変更に伴う更新が必要か設定
	mDirty = true;

	// コリジョンを無効化して物理エンジンへの通知を抑制
	CheckAndDisableCollision();

	// 親クラスを呼び出します
	Super::ClearInstances();
}
