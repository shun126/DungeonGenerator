/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonPartsTransform.h"
#include "DungeonActorParts.generated.h"

/**
 * Actor Parts
 * アクターのパーツ
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	/**
	 * Class of actor to spawn
	 * スポーンするアクターのクラス
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Class of actor to spawn", AllowedClasses = "/Script/Engine.Actor"))
	TObjectPtr<UClass> ActorClass = nullptr;
};