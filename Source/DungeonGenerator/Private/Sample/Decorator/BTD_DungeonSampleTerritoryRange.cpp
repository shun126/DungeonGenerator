/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Checks whether the sample character remains close enough to the recorded home position.
 * サンプルキャラクターが記録されたホーム位置から十分近い範囲に残っているかを判定します。
 */

#include "Sample/Decorator/BTD_DungeonSampleTerritoryRange.h"
#include "Sample/DungeonSampleCharacterBase.h"
#include <AIController.h>
#include <BehaviorTree/BehaviorTreeComponent.h>
#include <BehaviorTree/Blackboard/BlackboardKeyAllTypes.h>

bool UBTD_DungeonSampleTerritoryRange::CalculateRawConditionValue(UBehaviorTreeComponent& ownerComponent, uint8* nodeMemory) const
{
	const auto* controller = ownerComponent.GetAIOwner();
	if (IsValid(controller) == false)
		return false;

	const auto* ownerCharacter = Cast<ADungeonSampleCharacterBase>(controller->GetPawn());
	if (IsValid(ownerCharacter) == false)
		return false;

	const auto distance = FVector::Distance(ownerCharacter->GetActorLocation(), ownerCharacter->GetHomeLocation());
	constexpr double range = 50 * 100;
	return distance <= range;
}
