/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonPartsTransform.h"
#include "DungeonDoorActorParts.generated.h"

/**
 * Door actor Parts
 * ドアアクターのパーツ
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonDoorActorParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	/**
	 * Class of actor to spawn
	 * スポーンするアクターのクラス
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Class of actor to spawn", AllowedClasses = "/Script/DungeonGenerator.DungeonDoorBase"))
	TObjectPtr<UClass> ActorClass = nullptr;
};
