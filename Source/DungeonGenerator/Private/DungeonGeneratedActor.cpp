/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.

ADungeonGeneratedActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
ADungeonGenerateActorは配置可能(Placeable)、ADungeonGeneratedActorは配置不可能(NotPlaceable)にするため、
継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
*/

#include "DungeonGeneratedActor.h"

ADungeonGeneratedActor::ADungeonGeneratedActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

ADungeonGeneratedActor* ADungeonGeneratedActor::SpawnDungeonActor(UWorld* world, const FVector& location)
{
	if (IsValid(world) == false)
		return nullptr;

	const FTransform transform(location);
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.ObjectFlags |= RF_Transient;
	AActor* actor = SpawnActorImpl(
		world,
		ADungeonGeneratedActor::StaticClass(),
		TEXT("Actors"),
		transform,
		actorSpawnParameters
	);
	return Cast<ADungeonGeneratedActor>(actor);
}

void ADungeonGeneratedActor::DestroySpawnedActors(UWorld* world)
{
	Super::DestroySpawnedActors(world);	
}

