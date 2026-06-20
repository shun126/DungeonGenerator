/**
* @author		Shun Moriya
* @copyright	2023 - Shun Moriya
*/

#include "Sample/Task/BTT_DungeonSampleMoveTo.h"
#include <AIController.h>
#include <BehaviorTree/BlackboardComponent.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Object.h>
#include <BehaviorTree/Blackboard/BlackboardKeyType_Vector.h>
#include <Misc/EngineVersionComparison.h>

UBTT_DungeonSampleMoveTo::UBTT_DungeonSampleMoveTo(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	NodeName = "Move and Attack";
	bNotifyTick = true;

	/*
	 * Accept only actor and vector targets for movement, and actor targets for attack-distance checks.
	 * 移動対象はActorとVectorのみ、攻撃距離判定の対象はActorのみに制限します。
	 */
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_DungeonSampleMoveTo, BlackboardKey), AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_DungeonSampleMoveTo, BlackboardKey));
	TargetActorBlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_DungeonSampleMoveTo, TargetActorBlackboardKey), AActor::StaticClass());
}

void UBTT_DungeonSampleMoveTo::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	mMoveTime = 0;
	mAccept = OnceAwayFromOpponent == false;

#if UE_VERSION_NEWER_THAN(5, 7, 0)
	if (AcceptableRadius.GetValue(static_cast<const UBehaviorTreeComponent*>(nullptr)) > DistanceToCancelMovement.Min)
		AcceptableRadius = DistanceToCancelMovement.Min;
#else
	if (AcceptableRadius > DistanceToCancelMovement.Min)
		AcceptableRadius = DistanceToCancelMovement.Min;
#endif
}

EBTNodeResult::Type UBTT_DungeonSampleMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (Reached(OwnerComp) == true)
	{
		return EBTNodeResult::Type::Succeeded;
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

void UBTT_DungeonSampleMoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	mAccept = OnceAwayFromOpponent == false;
	mFinished = false;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTT_DungeonSampleMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	if (Reached(OwnerComp))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		mMoveTime += DeltaSeconds;
		if (mMoveTime >= MoveTimeLimit)
			FinishLatentTask(OwnerComp, EBTNodeResult::Aborted);
	}
}

/*
 * Returns true when the owner satisfies the completion distance against the move target or the optional target actor.
 * 移動対象または任意指定の対象アクターとの距離が完了条件を満たしている場合にtrueを返します。
 */
bool UBTT_DungeonSampleMoveTo::Reached(UBehaviorTreeComponent& OwnerComp)
{
	const auto* ownerController = OwnerComp.GetAIOwner();
	if (ownerController == nullptr)
		return false;

	const APawn* ownerPawn = ownerController->GetPawn();
	if (ownerPawn == nullptr)
		return true;

	const auto& ownerPawnLocation = ownerPawn->GetActorLocation();

	auto distanceBetweenOpponents = ownerPawn->GetSimpleCollisionRadius();

	const auto* myBlackboard = OwnerComp.GetBlackboardComponent();
	if (myBlackboard == nullptr)
		return true;

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		const auto* targetActor = Cast<AActor>(
			myBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID()));
		if (targetActor)
		{
			distanceBetweenOpponents += targetActor->GetSimpleCollisionRadius();

			if (CompletionCheck(ownerPawnLocation, targetActor->GetActorLocation(), distanceBetweenOpponents) == true)
				return true;
		}
	}
	else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const auto& attackingTargetLocation =
			myBlackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		if (CompletionCheck(ownerPawnLocation, attackingTargetLocation, distanceBetweenOpponents) == true)
			return true;
	}

	if (const auto* targetObject = myBlackboard->GetValue<UBlackboardKeyType_Object>(TargetActorBlackboardKey.GetSelectedKeyID()))
	{
		if (const auto* targetActor = Cast<AActor>(targetObject))
		{
			if (CompletionCheck(ownerPawnLocation, targetActor->GetActorLocation(), distanceBetweenOpponents) == true)
				return true;
		}
	}

	return false;
}

/*
 * Applies collision-radius adjustment and checks whether the distance is inside the configured completion band.
 * コリジョン半径による補正を適用し、距離が設定された完了範囲内にあるかを確認します。
 */
bool UBTT_DungeonSampleMoveTo::CompletionCheck(const FVector& ownerPawnLocation, const FVector& attackingTargetLocation, const double distanceBetweenOpponents) noexcept
{
	auto distance = FVector::Distance(ownerPawnLocation, attackingTargetLocation);

	distance -= distanceBetweenOpponents;

	if (mAccept)
	{
		mFinished = DistanceToCancelMovement.Min <= distance && distance <= DistanceToCancelMovement.Max;
		return mFinished;
	}

	if (distance > DistanceToCancelMovement.Max)
		mAccept = true;

	return false;
}
