/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "MainLevel/DungeonPartition.h"
#include "DungeonGenerateActor.h"
#include "Core/Debug/Debug.h"
#include <SceneInterface.h>
#include <SceneView.h>
#include <UnrealClient.h>
#include <Misc/EngineVersionComparison.h>
#include <Engine/GameViewportClient.h>
#include <Engine/LocalPlayer.h>
#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <Kismet/GameplayStatics.h>
#include <cmath>

/**
 * Ratio from the overall size to find the visible distance
 * 0.5 means you can see half the distance of the world
 * 
 * 見える距離を求める為の全体のサイズからの比率
 * 0.5なら世界の半分の距離が見える事になる
 */
static constexpr double VisibleDistanceRatio = 0.5;

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

void ADungeonMainLevelScriptActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	mBounding.Init();
	mActiveExtents.Set(ActiveExtentHorizontalSize, ActiveExtentHorizontalSize, ActiveExtentVerticalSize);
	TArray<ADungeonGenerateActor*> dungeonGenerateActors;
	float maxGridSize = 100.f;
	if (ULevel* level = GetValid(GetLevel()))
	{
		// DungeonGenerateActorの部屋の大きさからアクティブ範囲を求める
		for (AActor* actor : level->Actors)
		{
			ADungeonGenerateActor* dungeonGenerateActor = Cast<ADungeonGenerateActor>(actor);
			if (!IsValid(dungeonGenerateActor))
				continue;
			dungeonGenerateActors.Add(dungeonGenerateActor);

			const FBox& dungeonBoundingBox = dungeonGenerateActor->CalculateBoundingBox();
			mBounding += dungeonBoundingBox;

			const FVector& dungeonRoomMaxSize = dungeonGenerateActor->GetRoomMaxSize();
			if (mActiveExtents.X < dungeonRoomMaxSize.X)
				mActiveExtents.X = dungeonRoomMaxSize.X;
			if (mActiveExtents.Y < dungeonRoomMaxSize.Y)
				mActiveExtents.Y = dungeonRoomMaxSize.Y;
			if (mActiveExtents.Z < dungeonRoomMaxSize.Z)
				mActiveExtents.Z = dungeonRoomMaxSize.Z;

			const float gridSize = dungeonGenerateActor->GetGridSize();
			if (maxGridSize < gridSize)
				maxGridSize = gridSize;
		}
		if (dungeonGenerateActors.Num() > 0)
		{
			// アクティブ範囲とパーティエーションのサイズを求める
			mPartitionSize = mActiveExtents.X;
			if (mPartitionSize < mActiveExtents.Y)
				mPartitionSize = mActiveExtents.Y;
			if (mPartitionSize < mActiveExtents.Z)
				mPartitionSize = mActiveExtents.Z;
			mPartitionSize = std::ceil(mPartitionSize);
		}
	}

	// パーティエーションを初期化
	DungeonPartitions.Reset();
	if (mBounding.IsValid)
	{
		// 外周通路の為、グリッドの幅分広げる
		mBounding = mBounding.ExpandBy(maxGridSize);
		const FVector& boundingSize = mBounding.GetSize();

		// バウンディングサイズを記録
		mBoundingSize = boundingSize.X;
		if (mBoundingSize < boundingSize.Y)
			mBoundingSize = boundingSize.Y;

		// パーティエーションを生成
		mPartitionWidth = static_cast<size_t>(std::ceil(boundingSize.X / mPartitionSize));
		mPartitionDepth = static_cast<size_t>(std::ceil(boundingSize.Y / mPartitionSize));
		DungeonPartitions.Reserve(mPartitionWidth * mPartitionDepth);
		for (size_t i = 0; i < mPartitionWidth * mPartitionDepth; ++i)
		{
			DungeonPartitions.Add(NewObject<UDungeonPartition>());
		}

		// カリング距離を求めてDungeonGenerateActorに設定する
		const FInt32Interval cullingDistanceRange(
			mBoundingSize * 0.8,
			mBoundingSize * 0.9
		);
		for (ADungeonGenerateActor* dungeonGenerateActor : dungeonGenerateActors)
		{
			dungeonGenerateActor->SetInstancedMeshCullDistance(cullingDistanceRange);
		}
	}
	else
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Place the DungeonGenerateActor on the level"));
	}
}

void ADungeonMainLevelScriptActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	mPartitionWidth = mPartitionDepth = 0;
	DungeonPartitions.Reset();
	mBounding.Init();
}

const FSceneView* ADungeonMainLevelScriptActor::GetSceneView(const APlayerController* playerController) const
{
	ULocalPlayer* localPlayer = Cast<ULocalPlayer>(playerController->Player);
	if (localPlayer == nullptr)
		return nullptr;

	FViewport* viewport = localPlayer->ViewportClient->Viewport;
	FSceneViewFamilyContext sceneViewFamilyContext(
		FSceneViewFamily::ConstructionValues(
			localPlayer->ViewportClient->Viewport,
			GetWorld()->Scene,
			localPlayer->ViewportClient->EngineShowFlags
		).SetRealtimeUpdate(true)
	);

	FVector viewLocation;
	FRotator viewRotation;
	return localPlayer->CalcSceneView(&sceneViewFamilyContext, viewLocation, viewRotation, viewport);
}

void ADungeonMainLevelScriptActor::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (mBounding.IsValid == false)
	{
		SetActorTickEnabled(false);
		return;
	}

#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
	if (bEnableLoadControl)
#endif
	{
		if (const UWorld* world = GetValid(GetWorld()))
		{
			Begin();
			for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
			{
				const APlayerController* playerController = iterator->Get();
				if (playerController == nullptr)
					continue;

				const APawn* playerPawn = playerController->GetPawn();
				if (IsValid(playerPawn) && playerPawn->IsPlayerControlled())
				{
					// プレイヤー周辺をマークする
					const FVector& playerLocation = playerPawn->GetActorLocation();
					Mark(FBox(playerLocation - mActiveExtents, playerLocation + mActiveExtents));

					// 視錐台に含まれているならマークする
					if (const FSceneView* sceneView = GetSceneView(playerController))
					{
						Mark(*sceneView);
					}
#if WITH_EDITOR
					// 有効な範囲を黄色い線で描画します
					if (ShowDebugInformation)
					{
						UKismetSystemLibrary::DrawDebugBox(
							GetWorld(),
							playerLocation,
							mActiveExtents,
							FColor::Yellow,
							FRotator::ZeroRotator,
							0.f,
							10.f
						);
					}
#endif
				}
			}
			End(deltaSeconds);
		}
		else
		{
			ForceInactivate();
		}

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

#if WITH_EDITOR
	if (ShowDebugInformation)
		DrawDebugInformation();
#endif
}

/*
点で検索しているので、巨大なアクターは誤判定に注意してください。
*/
UDungeonPartition* ADungeonMainLevelScriptActor::Find(const FVector& worldLocation) const noexcept
{
	if (DungeonPartitions.Num() <= 0)
		return nullptr;

	const FVector localLocation = (worldLocation - mBounding.Min) / mPartitionSize;
	if (localLocation.X < 0 || mPartitionWidth <= localLocation.X)
		return nullptr;
	if (localLocation.Y < 0 || mPartitionDepth <= localLocation.Y)
		return nullptr;

	const size_t x = static_cast<size_t>(localLocation.X);
	const size_t y = static_cast<size_t>(localLocation.Y);
	const size_t index = mPartitionWidth * y + x;
	return DungeonPartitions[index];
}

void ADungeonMainLevelScriptActor::Begin()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->Unmark();
	}
}

void ADungeonMainLevelScriptActor::Mark(const FBox& activeBounds)
{
	const FVector start = (activeBounds.Min - mBounding.Min) / mPartitionSize;
	const FVector end = (activeBounds.Max - mBounding.Min) / mPartitionSize;
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
			UDungeonPartition* partition = DungeonPartitions[index];
			check(IsValid(partition));
			partition->Mark();
		}
	}
}

void ADungeonMainLevelScriptActor::Mark(const FSceneView& sceneView)
{
	const double halfPartitionSize = mPartitionSize * 0.5;
	const FVector partitionExtent(halfPartitionSize);
	const double visibleDistanceSquared = FMath::Square(mBoundingSize * VisibleDistanceRatio);

	size_t index = 0;
	FVector origin;
	origin.Y = mBounding.Min.Y + halfPartitionSize;
	origin.Z = mBounding.Min.Z + (mBounding.Max.Z - mBounding.Min.Z) * 0.5;
	while (origin.Y < mBounding.Max.Y + halfPartitionSize)
	{
		origin.X = mBounding.Min.X + halfPartitionSize;
		while (origin.X < mBounding.Max.X + halfPartitionSize)
		{
			if (FVector::DistSquared(sceneView.ViewLocation, origin) < visibleDistanceSquared)
			{
				if (sceneView.ViewFrustum.IntersectBox(origin, partitionExtent))
				{
					DungeonPartitions[index]->Mark();
				}
			}
			++index;
			origin.X += mPartitionSize;
		}
		origin.Y += mPartitionSize;
	}
}

void ADungeonMainLevelScriptActor::Mark(const FRotator& viewRotator, const FVector& viewLocation)
{
	const FVector segmentEnd = viewLocation + viewRotator.Vector() * (mBoundingSize * VisibleDistanceRatio);

	const double halfPartitionSize = mPartitionSize * 0.5;
	const FVector partitionExtent(halfPartitionSize, halfPartitionSize, (mBounding.Max.Z - mBounding.Min.Z) * 0.5);

	size_t index = 0;
	FVector origin;
	origin.Y = mBounding.Min.Y + halfPartitionSize;
	origin.Z = mBounding.Min.Z + (mBounding.Max.Z - mBounding.Min.Z) * 0.5;
	while (origin.Y < mBounding.Max.Y + halfPartitionSize)
	{
		origin.X = mBounding.Min.X + halfPartitionSize;
		while (origin.X < mBounding.Max.X + halfPartitionSize)
		{
			if (TestSegmentAABB(viewLocation, segmentEnd, origin, partitionExtent))
			{
				DungeonPartitions[index]->Mark();
			}
			++index;
			origin.X += mPartitionSize;
		}
		origin.Y += mPartitionSize;
	}
}

bool ADungeonMainLevelScriptActor::TestSegmentAABB(const FVector& segmentStart, const FVector& segmentEnd, const FVector& aabbCenter, const FVector& aabbExtent)
{
	FVector m = (segmentStart + segmentEnd) * 0.5;
	FVector d = segmentEnd - m;

	m = m - aabbCenter;

	double adx = std::abs(d.X);
	if (std::abs(m.X) > aabbExtent.X + adx) return false;
	double ady = std::abs(d.Y);
	if (std::abs(m.Y) > aabbExtent.Y + ady) return false;
	double adz = std::abs(d.Z);
	if (std::abs(m.Z) > aabbExtent.Z + adz) return false;

	adx += std::numeric_limits<double>::epsilon();
	ady += std::numeric_limits<double>::epsilon();
	adz += std::numeric_limits<double>::epsilon();

	if (std::abs(m.Y * d.Z - m.Z * d.Y) > aabbExtent.Y * adz + aabbExtent.Z * ady) return false;
	if (std::abs(m.Z * d.X - m.X * d.Z) > aabbExtent.X * adz + aabbExtent.Z * adx) return false;
	if (std::abs(m.X * d.Y - m.Y * d.X) > aabbExtent.X * ady + aabbExtent.Y * adx) return false;

	return true;
}

void ADungeonMainLevelScriptActor::End(const float deltaSeconds)
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		if (partition->IsMarked())
		{
			partition->CallPartitionActivate();
		}
		else if (partition->UpdateInactivateRemainTimer(deltaSeconds))
		{
			partition->CallPartitionActivate();
		}
		else
		{
			partition->CallPartitionInactivate();
		}
	}
}

void ADungeonMainLevelScriptActor::ForceActivate()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->CallPartitionActivate();
	}
}

void ADungeonMainLevelScriptActor::ForceInactivate()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->CallPartitionInactivate();
	}
}

#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
bool ADungeonMainLevelScriptActor::IsEnableLoadControl() const noexcept
{
	return bEnableLoadControl;
}
#endif

#if WITH_EDITOR
void ADungeonMainLevelScriptActor::DrawDebugInformation() const
{
	// パーティエーションの状態を線で描画します
	const double h = (mBounding.Max.Z - mBounding.Min.Z) * 0.5;
	const double z = mBounding.Min.Z;
	const FVector halfSize(mPartitionSize * 0.5, mPartitionSize * 0.5, h);
	for (double y = mBounding.Min.Y; y < mBounding.Max.Y; y += mPartitionSize)
	{
		for (double x = mBounding.Min.X; x < mBounding.Max.X; x += mPartitionSize)
		{
			if (const auto* partition = Find(FVector(x + 1, y + 1, z)))
			{
				const FVector location(x, y, z);
				UKismetSystemLibrary::DrawDebugBox(
					GetWorld(),
					location + halfSize,
					halfSize,
					partition->IsMarked() ? FColor::Red : FColor::Blue,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);
			}
		}
	}
}
#endif
