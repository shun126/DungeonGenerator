/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 *
 * Selects a random patrol location reachable from the home location and writes it to the selected blackboard key.
 * ホーム位置から到達可能なランダム巡回地点を選択し、選択されたBlackboardキーへ書き込みます。
 */

#include "Sample/Task/BTT_DungeonSampleSetRandomPatrolLocation.h"
#include "Sample/DungeonSampleCharacterBase.h"

#include <AIController.h>
#include <BehaviorTree/BTTaskNode.h>
#include <BehaviorTree/BehaviorTreeComponent.h>
#include <BehaviorTree/BlackboardComponent.h>
#include <BehaviorTree/Blackboard/BlackboardKeyAllTypes.h>
#include <Engine/World.h>
#include <NavigationPath.h>
#include <NavigationSystem.h>

namespace
{
	/*
	 * Returns true when a complete synchronous path can be built from the pawn's current location to the target location.
	 * Pawnの現在位置から目標位置まで、完全な同期経路を作成できる場合にtrueを返します。
	 */
	bool IsReachableFromCurrentLocation(const AAIController& controller, const ADungeonSampleCharacterBase& ownerCharacter, const FVector& targetLocation)
	{
		auto* const world = ownerCharacter.GetWorld();
		if (IsValid(world) == false)
			return false;

		APawn* pawn = controller.GetPawn();
		if (IsValid(pawn) == false)
			return false;

		const auto* const path = UNavigationSystemV1::FindPathToLocationSynchronously(world, ownerCharacter.GetNavAgentLocation(), targetLocation, pawn);
		return IsValid(path) && path->IsValid() && path->IsPartial() == false;
	}

	/*
	 * Finds the next patrol target by sampling a random navigation point reachable from the home location.
	 * ホーム位置から到達可能なランダムナビゲーション地点を選び、次の巡回目標にします。
	 */
	bool FindRandomPatrolLocation(const AAIController& controller, const ADungeonSampleCharacterBase& ownerCharacter, const float patrolRange, const bool validatePathFromCurrentLocation, FVector& nextTargetLocation)
	{
		auto* const world = ownerCharacter.GetWorld();
		if (IsValid(world) == false)
			return false;

		auto* const navigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
		if (IsValid(navigationSystem) == false)
			return false;

		const auto homeLocation = ownerCharacter.GetHomeLocation();
		auto* const navigationData = navigationSystem->GetNavDataForProps(ownerCharacter.GetNavAgentPropertiesRef(), homeLocation);
		if (IsValid(navigationData) == false)
			return false;

		FNavLocation randomLocation;
		if (navigationSystem->GetRandomReachablePointInRadius(homeLocation, FMath::Max(0.f, patrolRange), randomLocation, navigationData) == false)
			return false;

		if (validatePathFromCurrentLocation && IsReachableFromCurrentLocation(controller, ownerCharacter, randomLocation.Location) == false)
			return false;

		nextTargetLocation = randomLocation.Location;
		return true;
	}
}

UBTT_DungeonSampleSetRandomPatrolLocation::UBTT_DungeonSampleSetRandomPatrolLocation()
{
	NodeName = "SetRandomPatrolLocation";

	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTT_DungeonSampleSetRandomPatrolLocation, BlackboardKey));
}

EBTNodeResult::Type UBTT_DungeonSampleSetRandomPatrolLocation::ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory)
{
	const AAIController* controller = ownerComp.GetAIOwner();
	if (IsValid(controller) == false)
		return EBTNodeResult::Failed;

	const ADungeonSampleCharacterBase* ownerCharacter = Cast<ADungeonSampleCharacterBase>(controller->GetPawn());
	if (IsValid(ownerCharacter) == false)
		return EBTNodeResult::Failed;

	UBlackboardComponent* blackboard = ownerComp.GetBlackboardComponent();
	if (IsValid(blackboard) == false)
		return EBTNodeResult::Failed;

	FVector nextTargetLocation;
	if (FindRandomPatrolLocation(*controller, *ownerCharacter, PatrolRange, bValidatePathFromCurrentLocation, nextTargetLocation) == false)
		nextTargetLocation = ownerCharacter->GetHomeLocation();

	if (!blackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), nextTargetLocation))
		return EBTNodeResult::Failed;

	return EBTNodeResult::Succeeded;
}
