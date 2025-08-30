/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 * 
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

#include "SubActor/DungeonActorSpawnDirector.h"
#include "Core/Debug/Debug.h"
#include "Core/Helper/DrawLots.h"
#include <Engine/World.h>
#include <GameFramework/Pawn.h>

ADungeonActorSpawnDirector::ADungeonActorSpawnDirector(const FObjectInitializer& objectInitializer)
    : Super(objectInitializer)
{
	// Tick Enable
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.f;

	// ルートシーンコンポーネントを作成
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	check(IsValid(Root));
	SetRootComponent(Root);
}

void ADungeonActorSpawnDirector::BeginPlay()
{
	Super::BeginPlay();

	// 変数を初期化
	mElapsedTime = SpawnIntervalTime;
	mRandom = std::make_shared<dungeon::Random>();

	// サーバーでないのなら何も処理しない
	if (HasAuthority() == false)
	{
		DUNGEON_GENERATOR_WARNING(TEXT("Destroy spawner '%s' because creating actors on the client may cause synchronization errors"), *GetName());
		Destroy();
	}
}

void ADungeonActorSpawnDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 参照先が失効していたら管理から除外する
	{
		const auto i = std::remove_if(mActors.begin(), mActors.end(), [](const TWeakObjectPtr<AActor>& actor)
			{
				return actor.IsValid() == false;
			}
		);
		mActors.erase(i, mActors.end());
	}

	// インターバル時間を超えている？
	mElapsedTime += DeltaSeconds;
	while (mElapsedTime >= SpawnIntervalTime)
	{
		mElapsedTime -= SpawnIntervalTime;

		// スポーンするパラメータがある？
		if (SpawnActorParameters.Num() > 0)
		{
			// 管理中のアクターが最大数を超えていない？
			if (mActors.size() < MaxSpawnedActorsInWorld)
			{
				if (auto* world = GetValid(GetWorld()))
				{
					// 抽選
					const auto i = dungeon::DrawLots(mRandom, SpawnActorParameters.begin(), SpawnActorParameters.end(), [](const FDungeonSpawnActorParameter& parameter)
						{
							return parameter.Probability;
						}
					);

					// 対象をスポーン
					FActorSpawnParameters parameter;
					parameter.Owner = this;
					parameter.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
					auto* actor = world->SpawnActor<AActor>((*i).ActorClass, GetActorTransform(), parameter);
					if (IsValid(actor))
					{
						mActors.emplace_back(actor);

						// コントローラーが必要なら追加でスポーン
						if (auto* pawn = Cast<APawn>(actor))
						{
							pawn->SpawnDefaultController();
						}
					}
				}
			}
		}
	}
}
