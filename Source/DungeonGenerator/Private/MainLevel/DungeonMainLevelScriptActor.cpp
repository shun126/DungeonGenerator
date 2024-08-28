/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "MainLevel/DungeonPartiation.h"
#include "DungeonGenerateActor.h"
#include "Core/Debug/Debug.h"
#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <cmath>

ADungeonMainLevelScriptActor::ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, mActiveExtents(0)
#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	, mLastEnableLoadControl(bEnableLoadControl)
#endif
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADungeonMainLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();

	mBounding.Init();
	mActiveExtents.Set(ActiveExtentHorizontalSize, ActiveExtentHorizontalSize, ActiveExtentVerticalSize);

	ULevel* level = GetLevel();
	if (IsValid(level))
	{
		// DungeonGenerateActorの部屋の大きさからアクティブ範囲を求める
		TArray<ADungeonGenerateActor*> dungeonGenerateActors;
		for (AActor* actor : level->Actors)
		{
			ADungeonGenerateActor* dungeonGenerateActor = Cast<ADungeonGenerateActor>(actor);
			if (!IsValid(dungeonGenerateActor))
				continue;
			dungeonGenerateActors.Add(dungeonGenerateActor);

			const FBox& dungeonBoundingBox = dungeonGenerateActor->CalculateBoundingBox();
			mBounding += dungeonBoundingBox;

			// 現在と隣の部屋ともう少し含めるので二倍にする
			FVector dungeonRoomMaxSize = dungeonGenerateActor->GetRoomMaxSize();
			dungeonRoomMaxSize *= 3;

			if (mActiveExtents.X < dungeonRoomMaxSize.X)
				mActiveExtents.X = dungeonRoomMaxSize.X;
			if (mActiveExtents.Y < dungeonRoomMaxSize.Y)
				mActiveExtents.Y = dungeonRoomMaxSize.Y;
			if (mActiveExtents.Z < dungeonRoomMaxSize.Z)
				mActiveExtents.Z = dungeonRoomMaxSize.Z;
		}

		// カリング距離を求めてDungeonGenerateActorに設定する
		double cullDistance = mActiveExtents.X;
		if (cullDistance < mActiveExtents.Y)
			cullDistance = mActiveExtents.Y;
		if (cullDistance < mActiveExtents.Z)
			cullDistance = mActiveExtents.Z;
		for (ADungeonGenerateActor* dungeonGenerateActor : dungeonGenerateActors)
		{
			// 辺の半分なので2倍にして辺の長さにする
			dungeonGenerateActor->SetInstancedMeshCullDistance(cullDistance * 2);
		}
	}
	
	// パーティエーションを初期化
	DungeonPartitions.Reset();

	if (mBounding.IsValid)
	{
		const FVector& boundingSize = mBounding.GetSize();
		mPartitionWidth = static_cast<size_t>(std::ceil(boundingSize.X / PartitionSize));
		mPartitionDepth = static_cast<size_t>(std::ceil(boundingSize.Y / PartitionSize));
		DungeonPartitions.Reserve(mPartitionWidth * mPartitionDepth);
		for (size_t i = 0; i < mPartitionWidth * mPartitionDepth; ++i)
		{
			DungeonPartitions.Add(NewObject<UDungeonPartiation>());
		}
	}
	else
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Place the DungeonGenerateActor on the level"));
		mPartitionWidth = 0;
		mPartitionDepth = 0;
	}
}

void ADungeonMainLevelScriptActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	DungeonPartitions.Reset();
	mBounding.Init();
}

void ADungeonMainLevelScriptActor::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (!mBounding.IsValid)
		return;

#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
	if (bEnableLoadControl)
#endif
	{
		Begin();

		const UWorld* world = GetWorld();
		if (IsValid(world))
		{
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			const int32 numberOfPlayers = world->GetNumPlayerControllers();
#else
			const int32 numberOfPlayers = UGameplayStatics::GetNumPlayerControllers(world);
#endif
			for (int32 i = 0; i < numberOfPlayers; ++i)
			{
				const APawn* playerPawn = UGameplayStatics::GetPlayerPawn(world, i);
				if (!IsValid(playerPawn))
					continue;

				const FVector& playerLocation = playerPawn->GetActorLocation();
#if 0
				const FVector extents(2 * 100);
				Mark(FBox(playerLocation - extents, playerLocation + extents));
#else
				Mark(FBox(playerLocation - mActiveExtents, playerLocation + mActiveExtents));
#endif
			}

		}

		End(deltaSeconds);

#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
		mLastEnableLoadControl = true;
#endif
	}
#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
	else
	{
		if (mLastEnableLoadControl != bEnableLoadControl)
		{
			mLastEnableLoadControl = bEnableLoadControl;
			ForceActivate();
		}
	}
#endif
}

/*
点で検索しているので、巨大なアクターは誤判定に注意してください。
*/
UDungeonPartiation* ADungeonMainLevelScriptActor::Find(const FVector& worldLocation) const noexcept
{
	//if (DungeonPartiations.IsEmpty())
	if (DungeonPartitions.Num() <= 0)
		return nullptr;
	const FVector localLocation = (worldLocation - mBounding.Min) / PartitionSize;
	const double tx = std::max(0.0, std::floor(static_cast<double>(localLocation.X)));
	const double ty = std::max(0.0, std::floor(static_cast<double>(localLocation.Y)));
	const size_t x = std::min(static_cast<size_t>(tx), mPartitionWidth - 1);
	const size_t y = std::min(static_cast<size_t>(ty), mPartitionDepth - 1);
	const size_t index = mPartitionWidth * y + x;
	return DungeonPartitions[index];
}

void ADungeonMainLevelScriptActor::Begin()
{
	for (UDungeonPartiation* partiations : DungeonPartitions)
	{
		if (IsValid(partiations))
			partiations->Unmark();
	}
}

void ADungeonMainLevelScriptActor::Mark(const FBox& activeBounds)
{
	const FVector start = (activeBounds.Min - mBounding.Min) / PartitionSize;
	const FVector end = (activeBounds.Max - mBounding.Min) / PartitionSize;
	const double sx = std::max(0.0, static_cast<double>(std::floor(start.X)));
	const double sy = std::max(0.0, static_cast<double>(std::floor(start.Y)));
	const double ex = std::max(0.0, static_cast<double>(std::ceil(end.X)));
	const double ey = std::max(0.0, static_cast<double>(std::ceil(end.Y)));
	const size_t iex = std::min(static_cast<size_t>(ex), mPartitionWidth);
	const size_t iey = std::min(static_cast<size_t>(ey), mPartitionDepth);
	for (size_t y = static_cast<size_t>(sy); y < iey; ++y)
	{
		for (size_t x = static_cast<size_t>(sx); x < iex; ++x)
		{
			const size_t index = mPartitionWidth * y + x;
			UDungeonPartiation* partiation = DungeonPartitions[index];
			if (IsValid(partiation))
				partiation->Mark();
		}
	}
}

void ADungeonMainLevelScriptActor::End(const float deltaSeconds)
{
	for (UDungeonPartiation* partiations : DungeonPartitions)
	{
		if (!IsValid(partiations))
			continue;

		if (partiations->IsMarked())
		{
			partiations->CallPartiationActivate();
		}
		else if (partiations->UpdateActivateRemainTimer(deltaSeconds))
		{
			partiations->CallPartiationActivate();
		}
		else
		{
			partiations->CallPartiationInactivate();
		}
	}
}

void ADungeonMainLevelScriptActor::ForceActivate()
{
	for (UDungeonPartiation* partiations : DungeonPartitions)
	{
		if (!IsValid(partiations))
			continue;

		partiations->CallPartiationActivate();
	}
}

void ADungeonMainLevelScriptActor::ForceInactivate()
{
	for (UDungeonPartiation* partiations : DungeonPartitions)
	{
		if (!IsValid(partiations))
			continue;

		partiations->CallPartiationInactivate();
	}
}

#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
bool ADungeonMainLevelScriptActor::IsEnableLoadControl() const noexcept
{
	return bEnableLoadControl;
}
#endif

