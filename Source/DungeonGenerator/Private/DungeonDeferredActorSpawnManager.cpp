/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "DungeonDeferredActorSpawnManager.h"
#include "DungeonGenerateBase.h"
#include "Core/Debug/Debug.h"
#include <Misc/EngineVersionComparison.h>

FDungeonDeferredActorSpawnManager::~FDungeonDeferredActorSpawnManager()
{
	CancelAll(/*bNotifyCallbacks=*/false);
}

void FDungeonDeferredActorSpawnManager::SetMaxRequestsPerUpdate(const int32 InMaxRequestsPerUpdate)
{
	MaxRequestsPerUpdate = InMaxRequestsPerUpdate;
}

void FDungeonDeferredActorSpawnManager::SetMaxTimePerUpdate(const double InMaxTimePerUpdate)
{
	MaxTimePerUpdate = InMaxTimePerUpdate;
}

int32 FDungeonDeferredActorSpawnManager::NumQueued() const
{
	return PendingRequests.Num();
}

void FDungeonDeferredActorSpawnManager::RequestSpawn(
	UWorld* World,
	UClass* ActorClass,
	const FString& FolderPath,
	const FTransform& Transform,
	const FActorSpawnParameters& SpawnParameters,
	const int32 PartitionIndex,
	TFunction<void(AActor*)> OnSpawned,
	UObject* RequestOwner)
{
	FDungeonDeferredActorSpawnRequest Request;
	Request.World = World;
	Request.ActorClass = ActorClass;
	Request.FolderPath = FolderPath;
	Request.Transform = Transform;
	Request.SpawnParameters = SpawnParameters;
	Request.PartitionIndex = PartitionIndex;
	Request.OnSpawned = MoveTemp(OnSpawned);
	Request.RequestOwner = RequestOwner;
	Request.bRequiresValidRequestOwner = RequestOwner != nullptr;
	PendingRequests.Add(MoveTemp(Request));
}

int32 FDungeonDeferredActorSpawnManager::Update(const TFunctionRef<bool(int32)>& IsPartitionActive)
{
	const double StartSecond = FPlatformTime::Seconds();
	int32 ProcessedThisUpdate = 0;

	while (NumQueued() > 0)
	{
		const int32 RequestIndex = FindNextRequestIndex(IsPartitionActive);
		if (!PendingRequests.IsValidIndex(RequestIndex))
		{
			break;
		}

		FDungeonDeferredActorSpawnRequest Request = MoveTemp(PendingRequests[RequestIndex]);
#if UE_VERSION_NEWER_THAN(5, 4, 0)
		PendingRequests.RemoveAt(RequestIndex, 1, EAllowShrinking::No);
#else
		PendingRequests.RemoveAt(RequestIndex, 1, false);
#endif

		const bool bCancelledForInvalidOwner = Request.bRequiresValidRequestOwner && !Request.RequestOwner.IsValid();
		AActor* Spawned = TrySpawnRequest(Request);
		if (!Spawned && !bCancelledForInvalidOwner)
		{
			const UClass* RequestedClass = Request.ActorClass.Get();
			DUNGEON_GENERATOR_WARNING(TEXT("Deferred Actor spawn failed for '%s'."), RequestedClass ? *RequestedClass->GetPathName() : TEXT("Invalid Class"));
		}

		if (Request.OnSpawned)
		{
			Request.OnSpawned(Spawned);
		}

		++ProcessedThisUpdate;

		if (MaxRequestsPerUpdate > 0 && ProcessedThisUpdate >= MaxRequestsPerUpdate)
		{
			break;
		}

		if (MaxTimePerUpdate > 0.0)
		{
			const double ElapsedSecond = FPlatformTime::Seconds() - StartSecond;
			if (ElapsedSecond >= MaxTimePerUpdate)
			{
				break;
			}
		}
	}

	if (ProcessedThisUpdate > 0)
	{
		PendingRequests.Shrink();
	}

	return ProcessedThisUpdate;
}

void FDungeonDeferredActorSpawnManager::CancelAll(const bool bNotifyCallbacks)
{
	for (FDungeonDeferredActorSpawnRequest& Request : PendingRequests)
	{
		if (bNotifyCallbacks && Request.OnSpawned)
		{
			Request.OnSpawned(nullptr);
		}
	}

	PendingRequests.Reset();
}

void FDungeonDeferredActorSpawnManager::ShiftQueuedRequests(const FVector& Delta)
{
	if (Delta.IsNearlyZero() || NumQueued() <= 0)
	{
		return;
	}

	for (FDungeonDeferredActorSpawnRequest& Request : PendingRequests)
	{
		Request.Transform.AddToTranslation(Delta);
	}
}

/**
 * Spawns one queued actor request.
 * キュー済みActor生成要求を1件生成します。
 */
AActor* FDungeonDeferredActorSpawnManager::TrySpawnRequest(const FDungeonDeferredActorSpawnRequest& Request)
{
	if (Request.bRequiresValidRequestOwner && !Request.RequestOwner.IsValid())
	{
		return nullptr;
	}

	UWorld* World = Request.World.Get();
	UClass* ActorClass = Request.ActorClass.Get();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	return ADungeonGenerateBase::SpawnActorWithFolderPath(
		World,
		ActorClass,
		Request.FolderPath,
		Request.Transform,
		Request.SpawnParameters
	);
}

/**
 * Finds the next request, preferring active partitions before inactive or unknown partitions.
 * アクティブなパーティションを非アクティブまたは不明なパーティションより優先して次の要求を検索します。
 */
int32 FDungeonDeferredActorSpawnManager::FindNextRequestIndex(const TFunctionRef<bool(int32)>& IsPartitionActive) const
{
	for (int32 RequestIndex = 0; RequestIndex < PendingRequests.Num(); ++RequestIndex)
	{
		if (IsPartitionActive(PendingRequests[RequestIndex].PartitionIndex))
		{
			return RequestIndex;
		}
	}

	return PendingRequests.IsEmpty() ? INDEX_NONE : 0;
}
