/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Misc/EngineVersionComparison.h>
#include "DungeonTransactionalHierarchicalInstancedStaticMeshComponent.generated.h"

/*
Class for summarizing the start and end processing when performing consecutive AddInstance and RemoveInstance.

Inherits from UHierarchicalInstancedStaticMeshComponent
*/
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonTransactionalHierarchicalInstancedStaticMeshComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()
	
public:
	/*
	Call this when changing a large number of instances.
	EndTransaction handles all processing related to graphics together.
	\param[in] collisionEnableControl collision enable only if physical collision is enabled
	*/
	void BeginTransaction(const bool collisionEnableControl);

	/*
	Call this when changing a large number of instances.
	Processes related to graphics are grouped together in an EndTransaction.
	*/
	void EndTransaction(const bool asyncBuildTree);

	/*
	During transaction?
	*/
	bool InTransaction() const;

	/*
	Not in transaction?
	*/
	bool IsImmediate() const;

	// overrides
#if UE_VERSION_OLDER_THAN(5, 0, 0)
	virtual int32 AddInstance(const FTransform& instanceTransforms) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices) override;
#else
	virtual int32 AddInstance(const FTransform& instanceTransforms, bool bWorldSpace = false) override;
	virtual TArray<int32> AddInstances(const TArray<FTransform>& instanceTransforms, bool bShouldReturnIndices, bool bWorldSpace = false) override;
#endif
	virtual bool RemoveInstance(int32 instanceIndex) override;
#if UE_VERSION_NEWER_THAN(5, 0, 0)
	bool RemoveInstances(const TArray<int32>& instancesToRemove);
#endif
	virtual void ClearInstances() override;

private:
	// Disable collision to suppress notifications to the physical engine
	void CheckAndDisableCollision();

	// Restore suppressed settings
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
