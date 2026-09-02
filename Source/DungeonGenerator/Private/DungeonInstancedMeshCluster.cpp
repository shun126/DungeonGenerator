/**
 * @author      Shun Moriya
 * @copyright   2025- Shun Moriya
 * All Rights Reserved.
 */

#include "DungeonInstancedMeshCluster.h"
#include "DungeonGenerateBase.h"
#include "Core/Debug/Debug.h"
#include <Components/HierarchicalInstancedStaticMeshComponent.h>
#include <Engine/StaticMesh.h>
#include <FoliageInstancedStaticMeshComponent.h>
#include <Misc/EngineVersionComparison.h>
#include <Misc/SecureHash.h>
#include <NavigationSystem.h>
#include <UObject/UObjectGlobals.h>

namespace
{
	FName MakeDeterministicComponentName(const bool hierarchical, const uint64 generationId, const int32 seed, const FIntVector& groupCoordinate, const UStaticMesh* staticMesh, const bool affectsNavigation)
	{
		const FString meshPath = IsValid(staticMesh) ? staticMesh->GetPathName() : FString();
		const FString meshHash = FMD5::HashAnsiString(*meshPath);
		return FName(*FString::Printf(
			TEXT("DG_%s_R%016llX_S%08X_G%08X_%08X_%08X_N%d_M%s"),
			hierarchical ? TEXT("HISM") : TEXT("ISM"),
			static_cast<unsigned long long>(generationId),
			static_cast<uint32>(seed),
			static_cast<uint32>(groupCoordinate.X),
			static_cast<uint32>(groupCoordinate.Y),
			static_cast<uint32>(groupCoordinate.Z),
			affectsNavigation ? 1 : 0,
			*meshHash));
	}

	void ReportMeshNetworkStability(const UStaticMesh* staticMesh)
	{
		if (!IsValid(staticMesh))
			return;

		const UPackage* package = staticMesh->GetOutermost();
		if (staticMesh->HasAnyFlags(RF_Transient) || package == nullptr || package == GetTransientPackage() || !staticMesh->IsFullNameStableForNetworking())
		{
			DUNGEON_GENERATOR_WARNING(TEXT("Instanced terrain mesh '%s' is transient and may not provide the same deterministic component name on every client. Use a saved Static Mesh asset available at the same path on every client."), *staticMesh->GetPathName());
		}
	}

	void ConfigureGeneratedMeshComponent(AActor* actor, UInstancedStaticMeshComponent* component, UStaticMesh* staticMesh, const bool affectsNavigation)
	{
		check(IsValid(actor));
		check(IsValid(component));
		check(IsValid(staticMesh));

		component->SetMobility(EComponentMobility::Movable);
		component->SetCanEverAffectNavigation(affectsNavigation);
		component->SetStaticMesh(staticMesh);
		component->ComponentTags.AddUnique(ADungeonGenerateBase::GetDungeonGeneratorTerrainTag());
		actor->AddInstanceComponent(component);
	}

	template <typename ComponentType>
	ComponentType* FindOrCreateNamedComponent(AActor* actor, UStaticMesh* staticMesh, const FName componentName, const bool affectsNavigation)
	{
		UObject* existingObject = StaticFindObjectFast(UObject::StaticClass(), actor, componentName);
		if (existingObject != nullptr)
		{
			ComponentType* existingComponent = Cast<ComponentType>(existingObject);
			const bool exactClass = existingObject->GetClass() == ComponentType::StaticClass();
			if (!IsValid(existingComponent) || !exactClass || existingComponent->GetStaticMesh() != staticMesh || existingComponent->CanEverAffectNavigation() != affectsNavigation)
			{
				DUNGEON_GENERATOR_ERROR(TEXT("Deterministic component name collision '%s'. Existing class/mesh does not match the requested instanced terrain component; generation was aborted for this component."), *componentName.ToString());
				return nullptr;
			}
			return existingComponent;
		}

		ComponentType* component = NewObject<ComponentType>(actor, componentName, RF_Transient);
		if (!IsValid(component))
			return nullptr;

		component->SetNetAddressable();
		component->SetIsReplicated(false);
		ConfigureGeneratedMeshComponent(actor, component, staticMesh, affectsNavigation);
		return component;
	}

	void ReportNavigationConfiguration(const UInstancedStaticMeshComponent* component)
	{
		if (!IsValid(component))
			return;
		if (!component->CanEverAffectNavigation())
			return;

		const UStaticMesh* staticMesh = component->GetStaticMesh();
		if (!IsValid(staticMesh))
			return;

		if (staticMesh->GetNavCollision() == nullptr)
			DUNGEON_GENERATOR_WARNING(TEXT("Instanced terrain mesh '%s' has no navigation collision data. Add simple collision or enable navigation data on the Static Mesh to generate a NavMesh."), *staticMesh->GetPathName());
		if (!staticMesh->IsNavigationRelevant())
			DUNGEON_GENERATOR_WARNING(TEXT("Instanced terrain mesh '%s' is not navigation relevant. Check its collision and navigation settings."), *staticMesh->GetPathName());
		if (component->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			DUNGEON_GENERATOR_WARNING(TEXT("Instanced terrain mesh '%s' has collision disabled and cannot contribute walkable geometry to the NavMesh."), *staticMesh->GetPathName());
	}
}

UInstancedStaticMeshComponent* FDungeonInstancedMeshCollection::FindOrCreateInstance(AActor* actor, UStaticMesh* staticMesh, const uint64 generationId, const int32 seed, const FIntVector& groupCoordinate, const bool affectsNavigation)
{
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsValid(component) && component->GetClass() == UInstancedStaticMeshComponent::StaticClass() && component->GetStaticMesh() == staticMesh && component->CanEverAffectNavigation() == affectsNavigation)
			return component;
	}

	ReportMeshNetworkStability(staticMesh);
	const FName componentName = MakeDeterministicComponentName(false, generationId, seed, groupCoordinate, staticMesh, affectsNavigation);
	UInstancedStaticMeshComponent* component = FindOrCreateNamedComponent<UInstancedStaticMeshComponent>(actor, staticMesh, componentName, affectsNavigation);
	if (!IsValid(component))
		return nullptr;

	component->SetVisibility(mPartitionVisible);
	mComponents.Add(component);
	return component;
}

UHierarchicalInstancedStaticMeshComponent* FDungeonInstancedMeshCollection::FindOrCreateHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh, const uint64 generationId, const int32 seed, const FIntVector& groupCoordinate, const bool affectsNavigation)
{
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsValid(component) && component->GetClass() == UHierarchicalInstancedStaticMeshComponent::StaticClass() && component->GetStaticMesh() == staticMesh && component->CanEverAffectNavigation() == affectsNavigation)
			return CastChecked<UHierarchicalInstancedStaticMeshComponent>(component);
	}

	ReportMeshNetworkStability(staticMesh);
	const FName componentName = MakeDeterministicComponentName(true, generationId, seed, groupCoordinate, staticMesh, affectsNavigation);
	UHierarchicalInstancedStaticMeshComponent* component = FindOrCreateNamedComponent<UHierarchicalInstancedStaticMeshComponent>(actor, staticMesh, componentName, affectsNavigation);
	if (!IsValid(component))
		return nullptr;

	component->bAutoRebuildTreeOnInstanceChanges = false;
	component->SetVisibility(mPartitionVisible);
	mComponents.Add(component);
	return component;
}

UFoliageInstancedStaticMeshComponent* FDungeonInstancedMeshCollection::FindOrCreateFoliageInstance(AActor* actor, UStaticMesh* staticMesh, const FInt32Interval& cullDistances)
{
	if (!IsValid(actor) || !IsValid(staticMesh))
		return nullptr;

	if (UFoliageInstancedStaticMeshComponent** cachedComponent = mFoliageComponentByStaticMesh.Find(staticMesh))
	{
		if (IsValid(*cachedComponent))
			return *cachedComponent;
		mFoliageComponentByStaticMesh.Remove(staticMesh);
	}

	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		UFoliageInstancedStaticMeshComponent* foliageComponent = Cast<UFoliageInstancedStaticMeshComponent>(component);
		if (IsValid(foliageComponent) && foliageComponent->GetStaticMesh() == staticMesh)
		{
			mFoliageComponentByStaticMesh.Add(staticMesh, foliageComponent);
			return foliageComponent;
		}
	}

	UFoliageInstancedStaticMeshComponent* component = NewObject<UFoliageInstancedStaticMeshComponent>(actor);
	if (!IsValid(component))
		return nullptr;


	actor->AddInstanceComponent(component);
	component->RegisterComponent();
	component->SetMobility(EComponentMobility::Movable);
	component->SetStaticMesh(staticMesh);
	component->SetCullDistances(cullDistances.Min, cullDistances.Max);
	component->bAutoRebuildTreeOnInstanceChanges = false;
	component->SetVisibility(mPartitionVisible);
	mComponents.Add(component);
	mFoliageComponentByStaticMesh.Add(staticMesh, component);
	return component;
}

void FDungeonInstancedMeshCollection::BeginTransaction(const EDungeonInstancedMeshCategory category)
{
	bool& transactionActive = category == EDungeonInstancedMeshCategory::GeneratedMesh
		? mGeneratedMeshTransactionActive
		: mVegetationTransactionActive;
	if (transactionActive)
		return;
	transactionActive = true;

	if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
		mPendingTransforms.Reset();

	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (!IsComponentInCategory(component, category))
			continue;
		if (UHierarchicalInstancedStaticMeshComponent* hierarchicalComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(component))
			hierarchicalComponent->bAutoRebuildTreeOnInstanceChanges = false;
	}
}

/**
 * Restarts vegetation batching after a partially completed foliage tree build.
 * Foliage Treeの構築が一部完了した後に植物の一括更新を再開します。
 */
void FDungeonInstancedMeshCollection::ResumeVegetationTransaction()
{
	mVegetationTransactionActive = true;
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (!IsComponentInCategory(component, EDungeonInstancedMeshCategory::Vegetation))
			continue;
		if (UHierarchicalInstancedStaticMeshComponent* hierarchicalComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(component))
			hierarchicalComponent->bAutoRebuildTreeOnInstanceChanges = false;
	}
}

void FDungeonInstancedMeshCollection::AddInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, const uint64 generationId, const int32 seed, const FIntVector& groupCoordinate, const bool affectsNavigation)
{
	if (!IsValid(actor) || !IsValid(staticMesh))
		return;

	mGeneratedMeshTransactionActive = true;
	UInstancedStaticMeshComponent* component = FindOrCreateInstance(actor, staticMesh, generationId, seed, groupCoordinate, affectsNavigation);
	if (IsValid(component))
	{
		mPendingTransforms.FindOrAdd(component).Add(transform);
		ExpandInstanceBounds(EDungeonInstancedMeshCategory::GeneratedMesh, staticMesh, transform);
	}
}

void FDungeonInstancedMeshCollection::AddHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, const uint64 generationId, const int32 seed, const FIntVector& groupCoordinate, const bool affectsNavigation)
{
	if (!IsValid(actor) || !IsValid(staticMesh))
		return;

	mGeneratedMeshTransactionActive = true;
	UHierarchicalInstancedStaticMeshComponent* component = FindOrCreateHierarchicalInstance(actor, staticMesh, generationId, seed, groupCoordinate, affectsNavigation);
	if (IsValid(component))
	{
		mPendingTransforms.FindOrAdd(component).Add(transform);
		ExpandInstanceBounds(EDungeonInstancedMeshCategory::GeneratedMesh, staticMesh, transform);
	}
}

bool FDungeonInstancedMeshCollection::AddFoliageInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, const FInt32Interval& cullDistances)
{
	UFoliageInstancedStaticMeshComponent* component = FindOrCreateFoliageInstance(actor, staticMesh, cullDistances);
	if (!IsValid(component))
		return false;

	mVegetationTransactionActive = true;
	component->AddInstance(transform);
	ExpandInstanceBounds(EDungeonInstancedMeshCategory::Vegetation, staticMesh, transform);
	return true;
}

void FDungeonInstancedMeshCollection::ExpandInstanceBounds(const EDungeonInstancedMeshCategory category, UStaticMesh* staticMesh, const FTransform& transform)
{
	if (!IsValid(staticMesh))
		return;

	FBox& bounds = category == EDungeonInstancedMeshCategory::GeneratedMesh
		? mGeneratedMeshInstanceBounds
		: mVegetationInstanceBounds;
	bounds += staticMesh->GetBoundingBox().TransformBy(transform.ToMatrixWithScale());
}

void FDungeonInstancedMeshCollection::EndTransaction(const EDungeonInstancedMeshCategory category)
{
	bool& transactionActive = category == EDungeonInstancedMeshCategory::GeneratedMesh
		? mGeneratedMeshTransactionActive
		: mVegetationTransactionActive;
	if (!transactionActive)
		return;
	transactionActive = false;

	mComponents.Shrink();
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (!IsComponentInCategory(component, category))
			continue;

		const bool wasRegistered = component->IsRegistered();
		if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
		{
			if (TArray<FTransform>* pendingTransforms = mPendingTransforms.Find(component))
			{
				if (!pendingTransforms->IsEmpty())
				{
#if UE_VERSION_NEWER_THAN(5, 4, 0)
					component->AddInstances(*pendingTransforms, false, true, false);
#else
					component->AddInstances(*pendingTransforms, false, true);
#endif
				}
			}
		}

		if (UHierarchicalInstancedStaticMeshComponent* hierarchicalComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(component))
		{
			hierarchicalComponent->bAutoRebuildTreeOnInstanceChanges = true;
			hierarchicalComponent->BuildTreeIfOutdated(category == EDungeonInstancedMeshCategory::Vegetation, category == EDungeonInstancedMeshCategory::GeneratedMesh);
		}

		component->UpdateBounds();
		if (!wasRegistered)
			component->RegisterComponent();
		else if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
			FNavigationSystem::UpdateComponentData(*component);

		if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
			ReportNavigationConfiguration(component);
	}

	if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
		mPendingTransforms.Reset();
}

bool FDungeonInstancedMeshCollection::BuildFoliageTreesBudgeted(int32& nextComponentIndex, int32& remainingBuildCount, const double startSecond, const double maxTimeSecond)
{
	while (nextComponentIndex < mComponents.Num())
	{
		UInstancedStaticMeshComponent* component = mComponents[nextComponentIndex];
		UFoliageInstancedStaticMeshComponent* foliageComponent = Cast<UFoliageInstancedStaticMeshComponent>(component);
		if (!IsValid(foliageComponent))
		{
			++nextComponentIndex;
			continue;
		}

		if (remainingBuildCount == 0)
			return false;

		++nextComponentIndex;
		foliageComponent->bAutoRebuildTreeOnInstanceChanges = true;
		foliageComponent->BuildTreeIfOutdated(true, false);
		if (remainingBuildCount > 0)
			--remainingBuildCount;

		if (maxTimeSecond > 0.0 && FPlatformTime::Seconds() - startSecond >= maxTimeSecond)
			return nextComponentIndex >= mComponents.Num();
	}

	mVegetationTransactionActive = false;
	return true;
}

bool FDungeonInstancedMeshCollection::IsComponentInCategory(const UInstancedStaticMeshComponent* component, const EDungeonInstancedMeshCategory category)
{
	if (!IsValid(component))
		return false;
	const bool foliage = Cast<UFoliageInstancedStaticMeshComponent>(component) != nullptr;
	return foliage == (category == EDungeonInstancedMeshCategory::Vegetation);
}

/**
 * Immediately unregisters one managed component category from navigation while preserving the components for disposal.
 * 管理中の1カテゴリーを、破棄処理用のコンポーネントを維持したままナビゲーションから即時登録解除します。
 */
void FDungeonInstancedMeshCollection::DisableNavigation(const EDungeonInstancedMeshCategory category)
{
	if (category != EDungeonInstancedMeshCategory::GeneratedMesh)
		return;

	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (!IsComponentInCategory(component, category))
			continue;
		if (IsValid(component) && component->CanEverAffectNavigation())
			component->SetCanEverAffectNavigation(false);
	}
}

void FDungeonInstancedMeshCollection::Destroy(const EDungeonInstancedMeshCategory category)
{
	if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
		DisableNavigation(category);

	for (int32 componentIndex = mComponents.Num() - 1; componentIndex >= 0; --componentIndex)
	{
		UInstancedStaticMeshComponent* component = mComponents[componentIndex];
		if (!IsComponentInCategory(component, category))
			continue;

		mPendingTransforms.Remove(component);
		if (IsValid(component))
		{
			/*
			 * Destroy every component the same way, through DestroyComponent.
			 * It unregisters the component and then removes it from the owner's instance and owned
			 * component lists, which UnregisterComponent followed by ConditionalBeginDestroy does not.
			 * The owner here outlives a single generation, so a component left in those lists is kept
			 * alive and can still be reached after its destruction has begun. Starting the destruction
			 * outside the garbage collector also races with the deferred unregistration of the physics
			 * body, which the physics thread processes later.
			 * 破棄はDestroyComponentへ統一します。
			 * この関数は登録解除に加えて、所有アクターのインスタンス／所有コンポーネント一覧からも
			 * 取り除きます。UnregisterComponentとConditionalBeginDestroyの組み合わせでは行われません。
			 * ここでの所有アクターは1回の生成より長く生きるため、一覧に残ったコンポーネントは
			 * 破棄を始めた後も参照され続けます。またGCの管理外で破棄を始めるため、
			 * 物理ボディの登録解除が物理スレッドで後から処理されるのと競合します。
			 */
			component->DestroyComponent();
		}
		mComponents.RemoveAt(componentIndex);
	}

	if (category == EDungeonInstancedMeshCategory::GeneratedMesh)
	{
		mGeneratedMeshTransactionActive = false;
		mGeneratedMeshInstanceBounds.Init();
	}
	else
	{
		mVegetationTransactionActive = false;
		mVegetationInstanceBounds.Init();
		mFoliageComponentByStaticMesh.Reset();
	}

	if (mComponents.IsEmpty())
		mPartitionVisible = true;
}

void FDungeonInstancedMeshCollection::DestroyAll()
{
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsValid(component))
			component->DestroyComponent();
	}
	mComponents.Reset();
	mPendingTransforms.Reset();
	mFoliageComponentByStaticMesh.Reset();
	mGeneratedMeshInstanceBounds.Init();
	mVegetationInstanceBounds.Init();
	mGeneratedMeshTransactionActive = false;
	mVegetationTransactionActive = false;
	mPartitionVisible = true;
}

void FDungeonInstancedMeshCollection::SetGeneratedMeshCullDistance(const FInt32Interval& cullDistances)
{
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsComponentInCategory(component, EDungeonInstancedMeshCategory::GeneratedMesh))
			component->SetCullDistances(cullDistances.Min, cullDistances.Max);
	}
}

const FBox& FDungeonInstancedMeshCollection::GetInstanceBounds(const EDungeonInstancedMeshCategory category) const noexcept
{
	return category == EDungeonInstancedMeshCategory::GeneratedMesh
		? mGeneratedMeshInstanceBounds
		: mVegetationInstanceBounds;
}

FBox FDungeonInstancedMeshCollection::GetCombinedInstanceBounds() const noexcept
{
	FBox bounds = mGeneratedMeshInstanceBounds;
	bounds += mVegetationInstanceBounds;
	return bounds;
}

void FDungeonInstancedMeshCollection::ShiftWorldOffset(const FVector& delta)
{
	if (delta.IsNearlyZero())
		return;
	if (mGeneratedMeshInstanceBounds.IsValid)
		mGeneratedMeshInstanceBounds = mGeneratedMeshInstanceBounds.ShiftBy(delta);
	if (mVegetationInstanceBounds.IsValid)
		mVegetationInstanceBounds = mVegetationInstanceBounds.ShiftBy(delta);
}

void FDungeonInstancedMeshCollection::SetPartitionVisible(const bool visible)
{
	if (mPartitionVisible == visible)
		return;

	mPartitionVisible = visible;
	for (UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsValid(component))
			component->SetVisibility(visible);
	}
}

bool FDungeonInstancedMeshCollection::IsPartitionVisible() const noexcept
{
	return mPartitionVisible;
}

bool FDungeonInstancedMeshCollection::IsEmpty(const EDungeonInstancedMeshCategory category) const noexcept
{
	for (const UInstancedStaticMeshComponent* component : mComponents)
	{
		if (IsComponentInCategory(component, category))
			return false;
	}
	return category != EDungeonInstancedMeshCategory::GeneratedMesh || mPendingTransforms.IsEmpty();
}

FBox FDungeonInstancedMeshCluster::GetPartitionBounds() const noexcept
{
	FBox bounds = mInstances.GetCombinedInstanceBounds();
	return bounds;
}

void FDungeonInstancedMeshCluster::SetPartitionVisible(const bool visible)
{
	mInstances.SetPartitionVisible(visible);
}

void FDungeonInstancedMeshCluster::ShiftWorldOffset(const FVector& delta)
{
	mInstances.ShiftWorldOffset(delta);
}

bool FDungeonInstancedMeshCluster::IsEmpty() const noexcept
{
	return mInstances.IsEmpty(EDungeonInstancedMeshCategory::GeneratedMesh) &&
		mInstances.IsEmpty(EDungeonInstancedMeshCategory::Vegetation)
		;
}
