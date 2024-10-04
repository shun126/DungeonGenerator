/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.

ADungeonActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
ADungeonGenerateActorは配置可能(Placeable)、ADungeonActorは配置不可能(NotPlaceable)にするため、
継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
*/

#include "DungeonActor.h"

ADungeonActor::ADungeonActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

ADungeonActor* ADungeonActor::SpawnDungeonActor(UWorld* world, const FVector& location)
{
	if (IsValid(world) == false)
		return nullptr;

	const FTransform transform(location);
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.ObjectFlags |= RF_Transient;
	AActor* actor = SpawnActorImpl(world, ADungeonActor::StaticClass(), TEXT("Actors"), transform, actorSpawnParameters);

	return Cast<ADungeonActor>(actor);
}

void ADungeonActor::DestroySpawnedActors(UWorld* world)
{
	Super::DestroySpawnedActors(world);	
}

#if WITH_EDITOR
void ADungeonActor::SyncLoadStreamLevels()
{
	Super::SyncLoadStreamLevels();
}
#endif

void ADungeonActor::UnloadStreamLevels()
{
	Super::UnloadStreamLevels();
}
