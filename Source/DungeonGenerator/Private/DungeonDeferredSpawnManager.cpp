/**
 * @brief Actor遅延スポーンを管理するシンプルなマネージャ。
 *
 * @details
 * - RequestSpawn() で要求をキューに積み、Update() で少しずつ消化してSpawnActorします。
 * - Update() は GameThread から呼ぶ前提です（SpawnActor がGT必須のため）。
 * - 予算は「最大スポーン数」または「最大処理時間(ms)」で制限できます。
 * - CancelAll() は未処理要求を破棄します（必要ならコールバックに nullptr を通知）。
 */

#include "DungeonDeferredSpawnManager.h"

#include "DungeonGenerateBase.h"

FDungeonDeferredSpawnManager::~FDungeonDeferredSpawnManager()
{
	CancelAll(/*bNotifyCallbacks=*/false);
}

void FDungeonDeferredSpawnManager::SetMaxSpawnsPerUpdate(int32 InMaxSpawnsPerUpdate)
{
	MaxSpawnsPerUpdate = InMaxSpawnsPerUpdate;
}

void FDungeonDeferredSpawnManager::SetMaxTimePerUpdate(double InMaxTimePerUpdate)
{
	MaxTimePerUpdate = InMaxTimePerUpdate;
}

int32 FDungeonDeferredSpawnManager::NumQueued() const
{
	return QueueCount;
}

void FDungeonDeferredSpawnManager::RequestSpawn(
	UWorld* World,
	UClass* ActorClass,
	const FString& FolderPath,
	const FTransform& Transform,
	const FActorSpawnParameters& SpawnParameters,
	TFunction<void(AActor*)> OnSpawned)
{
	FDungeonDeferredSpawnRequest Req;
	Req.World = World;
	Req.ActorClass = ActorClass;
	Req.FolderPath = FolderPath;
	Req.Transform = Transform;
	Req.SpawnParameters = SpawnParameters;
	Req.OnSpawned = MoveTemp(OnSpawned);

	SpawnQueue.Enqueue(MoveTemp(Req));
	++QueueCount;
}

int32 FDungeonDeferredSpawnManager::Update()
{
	const double StartSecond = FPlatformTime::Seconds();
	int32 SpawnedThisUpdate = 0;

	while (QueueCount > 0)
	{
		FDungeonDeferredSpawnRequest Req;
		if (!SpawnQueue.Dequeue(Req))
		{
			QueueCount = 0;
			break;
		}
		--QueueCount;

		AActor* Spawned = nullptr;
		UWorld* World = Req.World.Get();
		UClass* ActorClass = Req.ActorClass.Get();

		if (World && ActorClass)
		{
			Spawned = ADungeonGenerateBase::SpawnActorWithFolderPath(
				World,
				ActorClass,
				Req.FolderPath,
				Req.Transform,
				Req.SpawnParameters
			);
		}

		// 通知（失敗/キャンセルは nullptr）
		if (Req.OnSpawned)
		{
			Req.OnSpawned(Spawned);
		}

		if (Spawned)
		{
			++SpawnedThisUpdate;
		}

		// 数量制限
		if (MaxSpawnsPerUpdate > 0 && SpawnedThisUpdate >= MaxSpawnsPerUpdate)
		{
			break;
		}

		// 時間制限
		if (MaxTimePerUpdate > 0.0)
		{
			const double ElapsedSecond = FPlatformTime::Seconds() - StartSecond;
			if (ElapsedSecond >= MaxTimePerUpdate)
			{
				break;
			}
		}
	}

	return SpawnedThisUpdate;
}

void FDungeonDeferredSpawnManager::CancelAll(bool bNotifyCallbacks)
{
	FDungeonDeferredSpawnRequest Req;
	while (SpawnQueue.Dequeue(Req))
	{
		--QueueCount;
		if (bNotifyCallbacks && Req.OnSpawned)
		{
			Req.OnSpawned(nullptr);
		}
	}
	QueueCount = 0;
}

void FDungeonDeferredSpawnManager::ShiftQueuedRequests(const FVector& Delta)
{
	if (Delta.IsNearlyZero() || QueueCount <= 0)
	{
		return;
	}

	TArray<FDungeonDeferredSpawnRequest> PendingRequests;
	PendingRequests.Reserve(QueueCount);

	FDungeonDeferredSpawnRequest Req;
	while (SpawnQueue.Dequeue(Req))
	{
		Req.Transform.AddToTranslation(Delta);
		PendingRequests.Add(MoveTemp(Req));
	}

	QueueCount = 0;
	for (FDungeonDeferredSpawnRequest& PendingRequest : PendingRequests)
	{
		SpawnQueue.Enqueue(MoveTemp(PendingRequest));
		++QueueCount;
	}
}
