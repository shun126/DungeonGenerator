/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <BehaviorTree/Tasks/BTTask_MoveTo.h>
#include "BTT_DungeonSampleMoveTo.generated.h"

 /**
  * MoveTo task that can finish when the owner reaches an attack distance band around the move target or a separate target actor.
  * 移動先または別指定の対象アクターとの距離が攻撃用の距離範囲に入ったときに完了できるMoveToタスクです。
  */
UCLASS(meta = (ToolTip = "Moves like the standard MoveTo task, but can finish when the owner reaches the configured attack distance band from the move target or target actor."))
class DUNGEONGENERATOR_API UBTT_DungeonSampleMoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	/**
	 * サンプル用MoveToタスクを作成し、Blackboardキー選択を対応する対象型に制限します。
	 */
	explicit UBTT_DungeonSampleMoveTo(const FObjectInitializer& objectInitializer);

	/**
	 * サンプル用MoveToタスクを破棄します。
	 */
	virtual ~UBTT_DungeonSampleMoveTo() override = default;

protected:
	/**
	 * Initializes runtime state and clamps the MoveTo acceptable radius to the attack distance band.
	 * 実行時状態を初期化し、MoveToの許容半径を攻撃用の距離範囲に合わせて制限します。
	 */
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	/**
	 * Starts movement unless the owner already satisfies the attack distance condition.
	 * オーナーがすでに攻撃用の距離条件を満たしていない場合に移動を開始します。
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/**
	 * Represents OnTaskFinished.
	 * タスク完了後に一時的な完了状態をリセットします。
	 */
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	/**
	 * 親のMoveToタスクを更新し、攻撃用の距離条件を満たしたら完了します。
	 */
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	/**
	 * Optional actor key used as the attack-distance reference when it differs from the MoveTo blackboard key.
	 * MoveToのBlackboardキーとは別の対象を攻撃距離の基準にしたい場合に使う任意のアクターキーです。
	 */
	UPROPERTY(EditAnywhere, Category = "Blackboard|AttackMoveTo", meta = (ToolTip = "Optional actor key used as the attack-distance reference when it differs from the MoveTo blackboard key. Leave unset to rely on the MoveTo target."))
	FBlackboardKeySelector TargetActorBlackboardKey;

	/**
	 * Maximum allowed duration for a movement action. If this time is exceeded,
	 * the movement command will be interrupted.
	 * 移動アクションに許可される最大時間。この時間を超えると、移動コマンドは中断されます。
	 */
	UPROPERTY(EditAnywhere, Category = "Blackboard|AttackMoveTo", meta = (ClampMin = "0", ToolTip = "Time limit for movement execution. The movement command will stop if this duration is exceeded."))
	float MoveTimeLimit = 5;

	/**
	 * Distance band from the target where movement finishes; collision radii are subtracted before this check.
	 * 移動を完了する対象との距離範囲です。この判定前に双方のコリジョン半径が差し引かれます。
	 */
	UPROPERTY(EditAnywhere, Category = "Blackboard|AttackMoveTo", meta = (ToolTip = "Distance band from the target where movement finishes. The owner and target collision radii are subtracted before checking this range."))
	FFloatInterval DistanceToCancelMovement = { 400.f, 800.f };

	/**
	 * Requires the owner to move outside the maximum distance once before the task can finish inside the distance band.
	 * 距離範囲内で完了できるようにする前に、オーナーが一度最大距離より外へ離れることを要求します。
	 */
	UPROPERTY(EditAnywhere, Category = "Blackboard|AttackMoveTo", meta = (ToolTip = "When enabled, the owner must move outside the maximum distance once before this task can finish inside the configured distance band."))
	bool OnceAwayFromOpponent = false;

private:
	/**
	 * オーナーが現在、設定された距離完了条件のいずれかを満たしている場合にtrueを返します。
	 */
	bool Reached(UBehaviorTreeComponent& OwnerComp);

	/**
	 * Checks whether the owner-target distance is inside the completion band after collision-radius adjustment.
	 * コリジョン半径を考慮したオーナーと対象の距離が完了範囲内にあるかを確認します。
	 */
	bool CompletionCheck(const FVector& ownerPawnLocation, const FVector& attackingTargetLocation, const double distanceBetweenOpponents) noexcept;

	/**
	 * If the movement time is greater than or equal to MoveTimeLimit, the movement will be interrupted.
	 * 移動時間がMoveTimeLimit以上なら移動を中断します。
	 */
	float mMoveTime = 0;

	/**
	 * True when distance-band completion is currently allowed.
	 * 距離範囲による完了判定が現在許可されている場合にtrueです。
	 */
	bool mAccept = false;

	/**
	 * Stores whether the task has already reached its completion distance in the current run.
	 * 現在の実行でタスクがすでに完了距離に到達したかを保持します。
	 */
	bool mFinished = false;
};
