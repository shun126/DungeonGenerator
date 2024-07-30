/**
@brief		HierarchicalInstancedStaticMeshComponent for collective operation
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Misc/EngineVersionComparison.h>
#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.generated.h"

/**
定義するとトランザクション中のコリジョンタイプをQueryのみにする
物理コリジョンが衝突し無くなる不具合があるので注意
*/
//#define DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL

/**
@brief		HierarchicalInstancedStaticMeshComponent for collective operation

Class for summarizing the start and end processing when performing consecutive AddInstance and RemoveInstance.
Inherits from UHierarchicalInstancedStaticMeshComponent
AddInstanceとRemoveInstanceを連続して実行する際に、開始と終了の処理をまとめるためのクラスです。
UHierarchicalInstancedStaticMeshComponent を継承しています。
*/
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonTransactionalHierarchicalInstancedStaticMeshComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()
	
public:
	/**
	constructor
	コンストラクタ
	*/
	explicit UDungeonTransactionalHierarchicalInstancedStaticMeshComponent(const FObjectInitializer& objectInitializer);

	/**
	destructor
	デストラクタ
	*/
	virtual ~UDungeonTransactionalHierarchicalInstancedStaticMeshComponent() = default;

	/**
	Call this when changing a large number of instances.
	EndTransaction handles all processing related to graphics together.
	大量のインスタンスを変更するときに呼び出します。
	EndTransactionはグラフィックスに関するすべての処理をまとめて行います。

	@param[in] collisionEnableControl collision enable only if physical collision is enabled
	*/
#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
	void BeginTransaction(const bool collisionEnableControl);
#else
	void BeginTransaction();
#endif

	/**
	Call this when changing a large number of instances.
	Processes related to graphics are grouped together in an EndTransaction.
	大量のインスタンスを変更する場合に呼び出す。
	グラフィックスに関連するプロセスは EndTransaction にまとめられます。
	@param[in]	asyncBuildTree		If true, build the tree asynchronously
	*/
	void EndTransaction(const bool asyncBuildTree);

	/**
	During transaction?
	操作中？
	*/
	bool InTransaction() const;

	/**
	Not in transaction?
	操作中ではない？
	*/
	bool IsImmediate() const;

	// overrides
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	virtual int32 AddInstance(const FTransform& instanceTransforms) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices) override;
#else
	virtual int32 AddInstance(const FTransform& instanceTransforms, bool bWorldSpace = false) override;
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace = false) override;
#else
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace = false, bool bUpdateNavigation = true) override;
#endif
#endif
	virtual bool RemoveInstance(int32 instanceIndex) override;
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	bool RemoveInstances(const TArray<int32>& instancesToRemove);
#endif
	virtual void ClearInstances() override;

private:
	// Disable collision to suppress notifications to the physical engine
	void CheckAndDisableCollision();

private:
	bool mImmediate = true;
	bool mDirty = false;
	bool mAutoRebuildTreeOnInstanceChanges = true;
#if defined(DUNGEON_TRANSACTIONAL_HIERARCHICAL_INSTANCED_STATIC_MESH_COMPONENT_USE_COLLISION_TYPE_CONTROL)
	bool mCollisionEnableControl = false;
	bool mCollisionEnableDirty = false;
	ECollisionEnabled::Type mCollisionEnabledType : 8;
#endif
};

inline UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::UDungeonTransactionalHierarchicalInstancedStaticMeshComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

inline bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::InTransaction() const
{
	return IsImmediate() == false;
}

inline bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::IsImmediate() const
{
	return mImmediate;
}
