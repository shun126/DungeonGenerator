#pragma once

#include "Containers/Queue.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"
#include "UObject/Class.h"

/**
 * @brief 遅延スポーン要求（1件分）。
 *
 * - 本structは「要求データ」を保持するだけで、GCの参照トラッキングは行いません。
 * - UWorld / UClass は所有しないため、Weak参照で保持します（破棄済みを安全に検出するため）。
 * - 生成完了通知はシングルキャスト（1回呼ばれる想定）です。
 */
struct FDungeonDeferredSpawnRequest
{
	/**
	 * スポーン先ワールド（所有しない）
	 */
	TWeakObjectPtr<UWorld> World;

	/**
	 * スポーンするActorクラス（所有しない）
	 */
	TWeakObjectPtr<UClass> ActorClass;

	/**
	 * エディタ上のフォルダパス（任意）。空なら設定しない。
	 */
	FString FolderPath;

	/**
	 * スポーン姿勢
	 */
	FTransform Transform = FTransform::Identity;

	/** SpawnActorに渡すパラメータ */
	FActorSpawnParameters SpawnParameters;

	/**
	 * @brief スポーン完了時に呼ばれるコールバック。
	 *
	 * - 成功時：生成されたActorポインタが渡されます。
	 * - 失敗/キャンセル時：nullptr が渡されます（本実装方針）。
	 */
	TFunction<void(AActor*)> OnSpawned;
};

/**
 * @brief Actor遅延スポーンを管理するシンプルなマネージャ。
 *
 * @details
 * - RequestSpawn() で要求をキューに積み、Update() で少しずつ消化してSpawnActorします。
 * - Update() は GameThread から呼ぶ前提です（SpawnActor がGT必須のため）。
 * - 予算は「最大スポーン数」または「最大処理時間(ms)」で制限できます。
 * - CancelAll() は未処理要求を破棄します（必要ならコールバックに nullptr を通知）。
 */
class FDungeonDeferredSpawnManager final
{
public:
	FDungeonDeferredSpawnManager() = default;
	~FDungeonDeferredSpawnManager();

	FDungeonDeferredSpawnManager(const FDungeonDeferredSpawnManager&) = delete;
	FDungeonDeferredSpawnManager& operator=(const FDungeonDeferredSpawnManager&) = delete;

	/**
	 * @brief 1回のUpdateで処理する最大スポーン数を設定します。
	 * @param InMaxSpawnsPerUpdate 0以下の場合は「無制限」扱い（時間制限があればそちらで止まる）。
	 */
	void SetMaxSpawnsPerUpdate(int32 InMaxSpawnsPerUpdate);

	/**
	 * @brief 1回のUpdateで使用できる最大時間(秒)を設定します。
	 * @param InMaxTimePerUpdate 0以下の場合は「時間制限なし」。
	 */
	void SetMaxTimePerUpdate(double InMaxTimePerUpdate);

	/**
	 * @brief キュー内の要求数を返します。
	 */
	int32 NumQueued() const;

	/**
	 * @brief 遅延スポーン要求を登録します。
	 *
	 * @param World スポーン先ワールド（所有しない）
	 * @param ActorClass スポーンするActorクラス（所有しない）
	 * @param FolderPath エディタ上のフォルダパス（任意）
	 * @param Transform スポーン変換
	 * @param SpawnParameters SpawnActorパラメータ
	 * @param OnSpawned 完了通知（成功: Actor*, 失敗/キャンセル: nullptr）
	 */
	void RequestSpawn(
		UWorld* World,
		UClass* ActorClass,
		const FString& FolderPath,
		const FTransform& Transform,
		const FActorSpawnParameters& SpawnParameters,
		TFunction<void(AActor*)> OnSpawned);

	/**
	 * @brief キューを消化してActorをスポーンします（GameThreadで呼ぶこと）。
	 *
	 * @return このUpdateでスポーンした数
	 */
	int32 Update();

	/**
	 * @brief 未処理のスポーン要求をすべて破棄します。
	 *
	 * @param bNotifyCallbacks trueの場合、破棄した各要求のコールバックへ nullptr を通知します。
	 */
	void CancelAll(bool bNotifyCallbacks = true);

	/**
	 * Shift queued spawn transforms by world-space delta.
	 *
	 * キュー済みスポーン要求のTransformをワールド座標で平行移動します。
	 *
	 * @param Delta Translation offset in world space.
	 */
	void ShiftQueuedRequests(const FVector& Delta);

private:
	/**
	 * 遅延スポーン要求キュー
	 */
	TQueue<FDungeonDeferredSpawnRequest> SpawnQueue;

	/**
	 * TQueueはNum()がないので、自前でカウント保持
	 */
	int32 QueueCount = 0;

	/**
	 * Update毎の最大スポーン数（0以下なら無制限）
	 */
	int32 MaxSpawnsPerUpdate = 0;

	/**
	 * Update毎の最大処理時間（ms）（0以下なら無制限）
	 */
	double MaxTimePerUpdate = 1.0 / 60.0;
};
