/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <BehaviorTree/BTDecorator.h>
#include "BTD_DungeonSampleTerritoryRange.generated.h"

/**
 * Behavior Tree decorator that checks whether the sample AI is still inside its territory around the home position.
 * サンプルAIがホーム位置を中心とした縄張り範囲内にいるかを判定するBehavior Treeデコレーターです。
 */
UCLASS(meta = (ToolTip = "Checks whether the sample AI is inside its territory around the recorded home position."))
class DUNGEONGENERATOR_API UBTD_DungeonSampleTerritoryRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	/**
	 * Represents UBTD_DungeonSampleTerritoryRange.
	 * 縄張り範囲デコレーターを作成します。
	 */
	UBTD_DungeonSampleTerritoryRange() = default;

	/**
	 * Destroys the ~UBTD_DungeonSampleTerritoryRange instance.
	 * 縄張り範囲デコレーターを破棄します。
	 */
	virtual ~UBTD_DungeonSampleTerritoryRange() override = default;

protected:
	/**
	 * 操作中のキャラクターがサンプルの縄張り範囲内に残っている場合にtrueを返します。
	 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& ownerComponent, uint8* nodeMemory) const override;
};
