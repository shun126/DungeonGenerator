/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGenerateBase.h"
#include "DungeonGeneratedActor.generated.h"

/**
ダンジョン生成アクター
*/
UCLASS(NotPlaceable)
class DUNGEONGENERATOR_API ADungeonGeneratedActor : public ADungeonGenerateBase
{
	GENERATED_BODY()

public:
	/**
	constructor
	コンストラクタ
	*/
	explicit ADungeonGeneratedActor(const FObjectInitializer& initializer);

	/**
	destructor
	デストラクタ
	*/
	virtual ~ADungeonGeneratedActor() override = default;

private:
	static ADungeonGeneratedActor* SpawnDungeonActor(UWorld* world, const FVector& location);
	static void DestroySpawnedActors(UWorld* world);


	// friend class
	friend class FDungeonGenerateEditorModule;
};


/*
あいうえお
*/
