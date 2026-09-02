/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"
#include "UObject/Class.h"

/**
 * Deferred actor spawn request processed over multiple ticks.
 * 複数Tickに分散して処理されるActor生成要求です。
 */
struct FDungeonDeferredActorSpawnRequest
{
	/**
	 * World that receives the spawned actor.
	 * Actorを生成するワールドです。
	 */
	TWeakObjectPtr<UWorld> World;

	/**
	 * Actor class to spawn.
	 * 生成するActorクラスです。
	 */
	TWeakObjectPtr<UClass> ActorClass;

	/**
	 * Optional editor folder path assigned to the spawned actor.
	 * 生成したActorに設定する任意のエディタ用フォルダパスです。
	 */
	FString FolderPath;

	/**
	 * Spawn transform used for the actor.
	 * Actor生成に使用するTransformです。
	 */
	FTransform Transform = FTransform::Identity;

	/**
	 * Spawn parameters forwarded to SpawnActor.
	 * SpawnActorへ渡す生成パラメータです。
	 */
	FActorSpawnParameters SpawnParameters;

	/**
	 * Callback invoked with the spawned actor, or nullptr on failure/cancel.
	 * 生成したActor、または失敗・キャンセル時のnullptrを通知するコールバックです。
	 */
	TFunction<void(AActor*)> OnSpawned;

	/**
	 * Optional object that must remain valid until this request is processed.
	 * この要求が処理されるまで有効である必要がある任意の所有オブジェクトです。
	 */
	TWeakObjectPtr<UObject> RequestOwner;

	/**
	 * Whether this request must be cancelled when RequestOwner becomes invalid.
	 * RequestOwnerが無効になったとき、この要求をキャンセルするかを示します。
	 */
	bool bRequiresValidRequestOwner = false;

	/**
	 * Runtime partition that contains the requested spawn location.
	 * 生成要求位置を含む実行時パーティションです。
	 */
	int32 PartitionIndex = INDEX_NONE;
};

/**
 * Processes actor spawning over multiple frames.
 * Actor生成を複数フレームに分散して処理します。
 */
class FDungeonDeferredActorSpawnManager final
{
public:
	/**
	 * Destroys the ~FDungeonDeferredActorSpawnManager instance.
	 * マネージャを破棄し、コールバックなしで保留中の要求を破棄します。
	 */
	~FDungeonDeferredActorSpawnManager();

	FDungeonDeferredActorSpawnManager() = default;
	FDungeonDeferredActorSpawnManager(const FDungeonDeferredActorSpawnManager&) = delete;
	FDungeonDeferredActorSpawnManager& operator=(const FDungeonDeferredActorSpawnManager&) = delete;

	/**
	 * 1回の更新で生成するActor数の上限を設定します。
	 */
	void SetMaxRequestsPerUpdate(int32 InMaxRequestsPerUpdate);

	/**
	 * Sets MaxTimePerUpdate.
	 * 1回の更新で使用できる最大時間を秒単位で設定します。
	 */
	void SetMaxTimePerUpdate(double InMaxTimePerUpdate);

	/**
	 * Gets the number of queued actor spawn requests.
	 * キューに残っているActor生成要求数を取得します。
	 */
	int32 NumQueued() const;

	/**
	 * 遅延Actor生成要求を追加します。
	 */
	void RequestSpawn(
		UWorld* World,
		UClass* ActorClass,
		const FString& FolderPath,
		const FTransform& Transform,
		const FActorSpawnParameters& SpawnParameters,
		int32 PartitionIndex,
		TFunction<void(AActor*)> OnSpawned,
		UObject* RequestOwner = nullptr);

	/**
	 * Processes queued actor spawn requests on the game thread.
	 * GameThread上でキュー済みActor生成要求を処理します。
	 */
	int32 Update(const TFunctionRef<bool(int32)>& IsPartitionActive);

	/**
	 * Clears all queued actor spawn requests.
	 * キュー済みActor生成要求をすべて破棄します。
	 */
	void CancelAll(bool bNotifyCallbacks = true);

	/**
	 * Shifts queued spawn transforms by world-space delta.
	 * キュー済み生成Transformをワールド座標で平行移動します。
	 */
	void ShiftQueuedRequests(const FVector& Delta);

private:
	static AActor* TrySpawnRequest(const FDungeonDeferredActorSpawnRequest& Request);
	int32 FindNextRequestIndex(const TFunctionRef<bool(int32)>& IsPartitionActive) const;

	/**
	 * Pending actor spawn requests.
	 * 保留中のActor生成要求配列です。
	 */
	TArray<FDungeonDeferredActorSpawnRequest> PendingRequests;

	/**
	 * Maximum requests processed per update. 0 or less means unlimited.
	 * 更新ごとの最大要求処理数です。0以下は無制限です。
	 */
	int32 MaxRequestsPerUpdate = 0;

	/**
	 * Maximum seconds spent per update. 0 or less means unlimited.
	 * 更新ごとの最大処理秒数です。0以下は無制限です。
	 */
	double MaxTimePerUpdate = 0.008;
};
