/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "DungeonLevelStreamingActor.generated.h"

class UBoxComponent;

/**
Helper class that loads the level when the player enters the OverlapVolume
プレイヤーが OverlapVolume に入るときにレベルをロードするヘルパークラス
*/
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonLevelStreamingActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	constructor
	*/
	explicit ADungeonLevelStreamingActor(const FObjectInitializer& initializer);

	/**
	destructor
	*/
	virtual ~ADungeonLevelStreamingActor() override = default;

protected:
	UFUNCTION()
	void OverlapBegins(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	UFUNCTION()
	void OverlapEnds(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

protected:
	/**
	 * Overlap volume to trigger level streaming
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TObjectPtr<UBoxComponent> OverlapVolume;

	/**
	 * Level streaming path
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (AllowedClasses = "World"))
	FSoftObjectPath Path;
};
