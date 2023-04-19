#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelStreamingDynamic.h>
#include "DungeonLevelStreamingDynamic.generated.h"

class ULevelStreamingDynamic;

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

	//virtual bool ShouldBeAlwaysLoaded() const override { return true; }
};
