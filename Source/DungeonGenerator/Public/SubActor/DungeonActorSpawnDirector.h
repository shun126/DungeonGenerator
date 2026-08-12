/**
 * @author      Shun Moriya
 * @copyright   2025- Shun Moriya
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

	/**
	 * Actor class that can be spawned by this rule.
	 *
	 * このルールでスポーン対象にするアクタークラスです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ToolTip = "Actor class that can be spawned by this rule.", AllowedClasses = "/Script/Engine.Actor"))
	TObjectPtr<UClass> ActorClass = nullptr;

	/**
	 * Spawn weight used for random selection among candidate actor classes.
	 *
	 * 候補アクタークラス間のランダム選択に使う出現ウェイトです。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = "1", ToolTip = "Relative selection weight among candidate actor classes. This is not a percentage."))
	int32 SelectionWeight = 10;

	/** Version 1 selection weight retained only for migration. 移行専用に保持するバージョン1の選択Weightです。 */
	UPROPERTY(meta = (DeprecatedProperty))
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
	 * Represents ADungeonActorSpawnDirector.
	 * コンストラクタ
	 */
	explicit ADungeonActorSpawnDirector(const FObjectInitializer& objectInitializer);

	/**
	 * Destroys the ~ADungeonActorSpawnDirector instance.
	 * デストラクタ
	 */
	virtual ~ADungeonActorSpawnDirector() override = default;

	// overrides
	/** Migrates legacy Blueprint defaults after loading. ロード後に旧Blueprintの既定値を移行します。 */
	virtual void PostLoad() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/**
	 * Represents Root.
	 * ルートシーンコンポーネント
	 */
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	/**
	 * Specify the class type of the actor to be generated and its probability of occurrence.
	 *
	 * 生成対象となるアクターのクラス型とその発生確率を指定して下さい。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Specify the class type of the actor to be generated and its probability of occurrence."))
	TArray<FDungeonSpawnActorParameter> SpawnActorParameters;

	/**
	 * Interval to spawn actors
	 *
	 * アクターをスポーンする間隔
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Interval to spawn actors", ClampMin = "1"))
	float SpawnIntervalTime = 60.f;

	/**
	 * Maximum number of actors to spawn.
	 * If this number is exceeded, the spawner will not spawn actors
	 *
	 * アクターをスポーンする最大人数
	 * この人数を超えるとスポナーはアクターをスポーンしません
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Maximum number of actors to spawn. If this number is exceeded, the spawner will not spawn actors"))
	uint8 MaxSpawnedActorsInWorld = 10;

private:
	std::shared_ptr<dungeon::Random> mRandom;
	std::vector<TWeakObjectPtr<AActor>> mActors;
	float mElapsedTime = 0.f;
};


