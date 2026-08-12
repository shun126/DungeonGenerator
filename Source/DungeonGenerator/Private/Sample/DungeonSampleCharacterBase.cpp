/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Sample/DungeonSampleCharacterBase.h"

ADungeonSampleCharacterBase::ADungeonSampleCharacterBase(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
}

void ADungeonSampleCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	HomeTransform = GetActorTransform();
}

const FTransform& ADungeonSampleCharacterBase::GetHomeTransform() const
{
	return HomeTransform;
}

FVector ADungeonSampleCharacterBase::GetHomeLocation() const
{
	return HomeTransform.GetLocation();
}
