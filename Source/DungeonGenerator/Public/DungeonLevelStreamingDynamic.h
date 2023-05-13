/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelStreamingDynamic.h>
#include "DungeonLevelStreamingDynamic.generated.h"

class ULevelStreamingDynamic;

/*
Class that wraps ULevelStreamingDynamic
No extended functionality
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonLevelStreamingDynamic : public ULevelStreamingDynamic
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonLevelStreamingDynamic(const FObjectInitializer& initializer);

	/*
	destructor
	*/
	virtual ~UDungeonLevelStreamingDynamic() = default;
};
