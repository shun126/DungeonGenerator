/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonPartsTransform.h"
#include "DungeonDoorActorParts.generated.h"

/**
Door actor Parts
ドアアクターのパーツ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonDoorActorParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif

public:
	/**
	Class of actor to spawn
	スポーンするアクターのクラス
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "/Script/DungeonGenerator.DungeonDoorBase"))
	TObjectPtr<UClass> ActorClass = nullptr;
};
