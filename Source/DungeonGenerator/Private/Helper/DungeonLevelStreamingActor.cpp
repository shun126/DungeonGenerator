/**
Helper class that loads the level when the player enters the OverlapVolume
プレイヤーが OverlapVolume に入るときにレベルをロードするヘルパークラス

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Helper/DungeonLevelStreamingActor.h"
#include <Components/BoxComponent.h>
#include <GameFramework/Character.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>

ADungeonLevelStreamingActor::ADungeonLevelStreamingActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
	PrimaryActorTick.bCanEverTick = true;

	OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
	OverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ADungeonLevelStreamingActor::OverlapBegins);
	OverlapVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ADungeonLevelStreamingActor::OverlapEnds);
	RootComponent = OverlapVolume;
}

void ADungeonLevelStreamingActor::OverlapBegins(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
	if (Path.IsValid())
	{
		ACharacter* myCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (otherActor == Cast<AActor>(myCharacter))
		{
#if UE_VERSION_NEWER_THAN(5, 0, 0)
			const FName& longPackageName = Path.GetLongPackageFName();
#else
			const FName& longPackageName = FName(Path.GetLongPackageName());
#endif
			FLatentActionInfo LatentInfo;
			UGameplayStatics::LoadStreamLevel(this, longPackageName, true, true, LatentInfo);
#if 0
			ULevelStreaming* level = UGameplayStatics::GetStreamingLevel(GetWorld(), LevelToLoad);
			level->LevelTransform = GetTransform();
			level->SetShouldBeVisible(true);
#endif
		}
	}
}

void ADungeonLevelStreamingActor::OverlapEnds(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
	if (Path.IsValid())
	{
		ACharacter* myCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (otherActor == Cast<AActor>(myCharacter))
		{
#if UE_VERSION_NEWER_THAN(5, 0, 0)
			const FName& longPackageName = Path.GetLongPackageFName();
#else
			const FName& longPackageName = FName(Path.GetLongPackageName());
#endif
			FLatentActionInfo LatentInfo;
			UGameplayStatics::UnloadStreamLevel(this, longPackageName, LatentInfo, false);
		}
	}
}
