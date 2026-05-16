/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 *
 * ADungeonGeneratedActorはエディターからの静的生成時にFDungeonGenerateEditorModuleからスポーンします。
 * ADungeonGenerateActorは配置可能(Placeable)、ADungeonGeneratedActorは配置不可能(NotPlaceable)にするため、
 * 継承元であるADungeonGenerateBaseをAbstract指定して共通機能をまとめています。
 */

#include "DungeonGeneratedActor.h"
#include "Core/Debug/Debug.h"

ADungeonGeneratedActor::ADungeonGeneratedActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

bool ADungeonGeneratedActor::Generate(const UDungeonGenerateParameter* parameter)
{
	if (mIsGeneratingDungeon)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Generate ignored because dungeon generation is already in progress."));
		return false;
	}

	TGuardValue<bool> generatingGuard(mIsGeneratingDungeon, true);

	const bool generated = BeginDungeonGeneration(parameter, true);
	if (generated)
	{
		TArray<APlayerStart*> startPoints;
		CollectPlayerStartExceptPlayerStartPIE(startPoints);
		MovePlayerStart(startPoints);
	}
	EndDungeonGeneration();

	return generated;
}

ADungeonGeneratedActor* ADungeonGeneratedActor::SpawnDungeonActor(UWorld* world, const FVector& location)
{
	if (IsValid(world) == false)
		return nullptr;

	const FTransform transform(location);
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.ObjectFlags |= RF_Transient;
	AActor* actor = SpawnActorWithFolderPath(
		world,
		ADungeonGeneratedActor::StaticClass(),
		TEXT("Actors"),
		transform,
		actorSpawnParameters
	);
	return Cast<ADungeonGeneratedActor>(actor);
}
