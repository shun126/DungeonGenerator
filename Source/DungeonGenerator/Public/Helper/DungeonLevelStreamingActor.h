/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "DungeonLevelStreamingActor.generated.h"

class UBoxComponent;

/**
 * Helper class that loads the level when the player enters the OverlapVolume
 * プレイヤーが OverlapVolume に入るときにレベルをロードするヘルパークラス
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonLevelStreamingActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * constructor
	 * ADungeonLevelStreamingActor を表します。
	 */
	explicit ADungeonLevelStreamingActor(const FObjectInitializer& initializer);

	/**
	 * destructor
	 * ~A Du ng eo nL ev el St re am in gA ct or インスタンスを破棄します。
	 */
	virtual ~ADungeonLevelStreamingActor() override = default;

protected:
	/**
	 * Handles the beginning of overlap with the streaming trigger volume.
	 * ストリーミング用トリガー範囲への侵入を処理します。
	 */
	UFUNCTION()
	void OverlapBegins(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);

	/**
	 * Handles the end of overlap with the streaming trigger volume.
	 * ストリーミング用トリガー範囲からの退出を処理します。
	 */
	UFUNCTION()
	void OverlapEnds(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

protected:
	/**
	 * Overlap volume to trigger level streaming
	 * OverlapVolume を表します。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Overlap volume to trigger level streaming"))
	TObjectPtr<UBoxComponent> OverlapVolume;

	/**
	 * Level streaming path
	 * Path を表します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Level streaming path", AllowedClasses = "/Script/Engine.World"))
	FSoftObjectPath Path;
};
