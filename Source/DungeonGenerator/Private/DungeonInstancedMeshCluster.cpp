/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInstancedMeshCluster.h"
#include "DungeonGenerateBase.h"
#include <Components/HierarchicalInstancedStaticMeshComponent.h>

/**
 * 登録したコンポーネントの数
 */
size_t FDungeonInstancedMeshCluster::mComponentCount = 0;

UInstancedStaticMeshComponent* FDungeonInstancedMeshCluster::FindOrCreateInstance(AActor* actor, UStaticMesh* staticMesh)
{
	for (auto& component : mComponents)
	{
		if (component && component->GetStaticMesh() == staticMesh)
		{
			check(Cast<UHierarchicalInstancedStaticMeshComponent>(component) == nullptr);
			return component;
		}
	}

    // UInstancedStaticMeshComponentを生成
	++mComponentCount;
	const FName componentName(TEXT("InstancedStaticMesh") + FString::FromInt(mComponentCount));
	auto* component = NewObject<UInstancedStaticMeshComponent>(actor, componentName);
	actor->AddInstanceComponent(component);
	component->RegisterComponent();
	component->SetStaticMesh(staticMesh);
	mComponents.Add(component);
	return component;
}

UHierarchicalInstancedStaticMeshComponent* FDungeonInstancedMeshCluster::FindOrCreateHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh)
{
	for (auto& component : mComponents)
	{
		if (component && component->GetStaticMesh() == staticMesh)
		{
			check(Cast<UHierarchicalInstancedStaticMeshComponent>(component) != nullptr);
			return Cast<UHierarchicalInstancedStaticMeshComponent>(component);
		}
	}

    // UHierarchicalInstancedStaticMeshComponentを生成
	++mComponentCount;
	const FName componentName(TEXT("InstancedStaticMesh") + FString::FromInt(mComponentCount));
	auto* component = NewObject<UHierarchicalInstancedStaticMeshComponent>(actor, componentName);
	actor->AddInstanceComponent(component);
	component->RegisterComponent();
	component->SetStaticMesh(staticMesh);
	component->bAutoRebuildTreeOnInstanceChanges = false;
	component->ComponentTags.Add(ADungeonGenerateBase::GetDungeonGeneratorTerrainTag());
	mComponents.Add(component);
	return component;
}

void FDungeonInstancedMeshCluster::BeginTransaction()
{
	for (auto& component : mComponents)
	{
		if (auto* hierarchicalInstancedStaticMeshComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(component))
		{
			hierarchicalInstancedStaticMeshComponent->bAutoRebuildTreeOnInstanceChanges = false;
		}
	}
}

void FDungeonInstancedMeshCluster::EndTransaction()
{
	mComponents.Shrink();

	for (auto& component : mComponents)
	{
		if (auto* hierarchicalInstancedStaticMeshComponent = Cast<UHierarchicalInstancedStaticMeshComponent>(component))
		{
			hierarchicalInstancedStaticMeshComponent->bAutoRebuildTreeOnInstanceChanges = true;
			hierarchicalInstancedStaticMeshComponent->BuildTreeIfOutdated(true, false);
		}
	}
}

void FDungeonInstancedMeshCluster::DestroyAll()
{
	for (auto& component : mComponents)
	{
		if (IsValid(component))
		{
			component->UnregisterComponent();
			component->ConditionalBeginDestroy();
			//component->DestroyComponent(true);
		}
	}
	mComponents.Reset();
}

void FDungeonInstancedMeshCluster::SetCullDistance(const FInt32Interval& cullDistances)
{
	for (auto& component : mComponents)
	{
		if (IsValid(component))
		{
			component->SetCullDistances(cullDistances.Min, cullDistances.Max);
		}
	}
}
