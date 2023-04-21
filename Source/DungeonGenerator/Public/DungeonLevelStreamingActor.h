/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once

#include <GameFramework/Actor.h>
#include "DungeonLevelStreamingActor.generated.h"

class UBoxComponent;

/*
プレイヤーがOverlapVolumeに入った時に指定レベルをロードするヘルパークラス

*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonLevelStreamingActor : public AActor
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit ADungeonLevelStreamingActor(const FObjectInitializer& initializer);

	/*
	destructor
	*/
	virtual ~ADungeonLevelStreamingActor() = default;

protected:
	UFUNCTION()
		void OverlapBegins(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
		void OverlapEnds(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

protected:
	// Overlap volume to trigger level streaming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		UBoxComponent* OverlapVolume;

	// Level streaming path
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (AllowedClasses = "World"))
		FSoftObjectPath Path;
};
