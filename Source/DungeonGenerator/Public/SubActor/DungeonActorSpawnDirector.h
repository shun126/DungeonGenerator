/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <memory>
#include <vector>
#include "DungeonActorSpawnDirector.generated.h"

namespace dungeon
{
	class Random;
}

/**
 * A parameter structure that defines the type and probability of the actor to be spawned.
 * FDungeonSpawnActorParameter is used by Spawner to specify the class type of the actor to be spawned and
 * the class type of the actor to be spawned and its probability of occurrence.
 * 
 * スポーンするアクターの型と確率を定義するパラメータ構造体。
 * FDungeonSpawnActorParameter は Spawner によって利用され、生成対象となるアクターのクラス型と
 * その発生確率を指定します。
 */
USTRUCT(BlueprintType)
struct FDungeonSpawnActorParameter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (AllowedClasses = "/Script/Engine.Actor"))
	TObjectPtr<UClass> ActorClass = nullptr;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = "1"))
	uint8 Probability = 10;
};

/**
 * A spawner class that spawns actors at regular intervals.
 * ADungeonActorSpawnDirector provides the ability to spawn actors in the game at specified intervals.
 * The type of actor to be spawned and the probability of its occurrence are defined by the FDungeonSpawnActorParameter structure.
 *
 * This class is used for spawn control with randomness and in situations where multiple types of actors are handled simultaneously.
 * The spawn interval and parameters can be changed dynamically according to game progression or difficulty adjustment.
 *
 * 一定間隔でアクターをスポーンするスポナークラス。
 * ADungeonActorSpawnDirector は、指定された間隔でゲーム内にアクターを生成する機能を提供します。
 * スポーン対象となるアクターの種類や発生確率は、FDungeonSpawnActorParameter 構造体によって定義されます。
 *
 * 本クラスはランダム性を持つスポーン制御や、複数タイプのアクターを同時に扱う場面で使用されます。
 * ゲーム進行や難易度調整に応じて、スポーン間隔やパラメータを動的に変更することも可能です。
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonActorSpawnDirector : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * コンストラクタ
	 */
	explicit ADungeonActorSpawnDirector(const FObjectInitializer& objectInitializer);

	/**
	 * デストラクタ
	 */
	virtual ~ADungeonActorSpawnDirector() override = default;


	// overrides
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/**
	 * ルートシーンコンポーネント
	 */
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	/**
	 * Specify the class type of the actor to be generated and its probability of occurrence.
	 * 
	 * 生成対象となるアクターのクラス型とその発生確率を指定して下さい。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	TArray<FDungeonSpawnActorParameter> SpawnActorParameters;

	/**
	 * Interval to spawn actors
	 *
	 * アクターをスポーンする間隔
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
	float SpawnIntervalTime = 60.f;

	/**
	 * Maximum number of actors to spawn.
	 * If this number is exceeded, the spawner will not spawn actors
	 *
	 * アクターをスポーンする最大人数
	 * この人数を超えるとスポナーはアクターをスポーンしません
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	uint8 MaxSpawnedActorsInWorld = 10;

private:
	std::shared_ptr<dungeon::Random> mRandom;
	std::vector<TWeakObjectPtr<AActor>> mActors;
	float mElapsedTime = 0.f;
};
