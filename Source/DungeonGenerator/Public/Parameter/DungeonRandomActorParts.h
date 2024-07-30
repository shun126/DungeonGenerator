/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonActorPartsWithDirection.h"
#include "DungeonRandomActorParts.generated.h"

/**
Actor parts with Probability
ランダム選択するアクターのパーツ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRandomActorParts : public FDungeonActorPartsWithDirection
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif

public:
	/*
	Production frequency
	配置する頻度
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0.", ClampMax = "1."))
	float Frequency = 1.f;
};
