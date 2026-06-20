/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <BehaviorTree/Tasks/BTTask_BlackboardBase.h>
#include "BTT_DungeonSampleSetRandomPatrolLocation.generated.h"

 /*
  * Behavior Tree task that writes a random patrol location reachable from the home location to the selected blackboard key.
  * ホーム位置から到達可能なランダム巡回地点を、選択されたBlackboardキーへ書き込むBehavior Treeタスクです。
  */
UCLASS(meta = (ToolTip = "Writes a random patrol location reachable from the home location. Enable current-location validation only when the pawn must also have a complete path from its current position to the selected point."))
class DUNGEONGENERATOR_API UBTT_DungeonSampleSetRandomPatrolLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	/*
	 * Creates the patrol location task and restricts the blackboard key selector to vector keys.
	 * 巡回地点タスクを作成し、Blackboardキー選択をVectorキーに制限します。
	 */
	UBTT_DungeonSampleSetRandomPatrolLocation();

	/*
	 * Destroys the patrol location task.
	 * 巡回地点タスクを破棄します。
	 */
	virtual ~UBTT_DungeonSampleSetRandomPatrolLocation() override = default;

	/*
	 * Chooses the next patrol location and stores it in the selected blackboard key.
	 * 次の巡回地点を選び、選択されたBlackboardキーへ保存します。
	 */
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;

protected:
	/*
	 * Radius used to select a random navigation point reachable from the home location.
	 * ホーム位置から到達可能なランダムナビゲーション地点を選ぶための半径です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample|Patrol", meta = (ClampMin = "0.0", ToolTip = "Radius for choosing a random navigation point reachable from the home location. Larger values spread patrol destinations farther across the connected NavMesh."))
	float PatrolRange = 2000.f;

	/*
	 * If true, also validates the selected point with a synchronous path from the pawn's current location.
	 * trueの場合、選択した地点に対してPawnの現在位置から同期経路を追加検証します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sample|Patrol", meta = (ToolTip = "When enabled, the task also calls FindPathToLocationSynchronously from the pawn's current location and rejects partial or invalid paths. Leave disabled when home reachability is enough."))
	bool bValidatePathFromCurrentLocation = false;
};
