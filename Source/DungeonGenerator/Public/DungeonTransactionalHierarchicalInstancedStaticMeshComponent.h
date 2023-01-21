/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.generated.h"

/*
連続してAddInstanceやRemoveInstanceを行う時に、開始と終了処理をまとめるためのクラス

UHierarchicalInstancedStaticMeshComponentを継承しています
*/
UCLASS(meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonTransactionalHierarchicalInstancedStaticMeshComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()
	
public:
	/*
	大量のインスタンスを変更する場合に呼び出して下さい。
	グラフィックに関する処理をEndTransactionでまとめて処理します。
	\param[in]	collisionEnableControl	物理コリジョンが有効の場合のみ、コリジョンの有効性を制御します
	*/
	void BeginTransaction(const bool collisionEnableControl);

	/*
	大量のインスタンスを変更する場合に呼び出して下さい。
	グラフィックに関する処理をEndTransactionでまとめて処理します。
	*/
	void EndTransaction(const bool asyncBuildTree);

	/*
	トランザクション中？
	*/
	bool InTransaction() const;

	/*
	トランザクション中ではない？
	*/
	bool IsImmediate() const;

	virtual int32 AddInstance(const FTransform& instanceTransforms, bool bWorldSpace = false) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace = false) override;
	virtual bool RemoveInstance(int32 instanceIndex) override;
	bool RemoveInstances(const TArray<int32>& instancesToRemove);
	virtual void ClearInstances() override;

private:
	// コリジョンを無効化して物理エンジンへの通知を抑制
	void CheckAndDisableCollision();

	// 抑制した設定を復元
	void CheckAndRevertCollision();

protected:
	bool mImmediate = true;
	bool mNotDirty = true;
	bool mCollisionEnableControl = false;
	bool mCollisionEnableDirty = false;
	ECollisionEnabled::Type mCollisionEnabledType : 8;
};

inline bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::InTransaction() const
{
	return IsImmediate() == false;
}

inline bool UDungeonTransactionalHierarchicalInstancedStaticMeshComponent::IsImmediate() const
{
	return mImmediate;
}
