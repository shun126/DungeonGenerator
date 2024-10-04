/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGenerateBase.h"
#include "DungeonActor.generated.h"

/**
ダンジョン生成アクター
*/
UCLASS(NotPlaceable)
class DUNGEONGENERATOR_API ADungeonActor : public ADungeonGenerateBase
{
	GENERATED_BODY()

public:
	/**
	constructor
	コンストラクタ
	*/
	explicit ADungeonActor(const FObjectInitializer& initializer);

	/**
	destructor
	デストラクタ
	*/
	virtual ~ADungeonActor() override = default;

private:
	static ADungeonActor* SpawnDungeonActor(UWorld* world, const FVector& location);
	static void DestroySpawnedActors(UWorld* world);

#if WITH_EDITOR
	virtual void SyncLoadStreamLevels() override;
#endif
	virtual void UnloadStreamLevels() override;

	// friend class
	friend class FDungeonGenerateEditorModule;
};


/*
あいうえお
*/
