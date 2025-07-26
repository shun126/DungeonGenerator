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

#if WITH_EDITOR
#include <array>
#endif

ADungeonMainLevelScriptActor::ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, mActiveExtents(0)
	, mLastEnableLoadControl(bEnableLoadControl)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ADungeonMainLevelScriptActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	mBounding.Init();
	mActiveExtents.Set(PartitionHorizontalMinSize, PartitionHorizontalMinSize, PartitionVerticalMinSize);
	FVector maxDungeonRoomMaxSize(PartitionHorizontalMinSize, PartitionHorizontalMinSize, PartitionVerticalMinSize);
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

			// 世界全体の大きさを求める
			mBounding += dungeonGenerateActor->CalculateBoundingBox();

			// 最も長い通路の長さを求める
			const auto& dungeonLongestStraightPath = dungeonGenerateActor->GetLongestStraightPath();

			// 最も大きい部屋の大きさを求める
			const FVector& dungeonRoomMaxSize = dungeonGenerateActor->GetRoomMaxSize();
			if (maxDungeonRoomMaxSize.X < dungeonRoomMaxSize.X)
				maxDungeonRoomMaxSize.X = dungeonRoomMaxSize.X;
			if (maxDungeonRoomMaxSize.Y < dungeonRoomMaxSize.Y)
				maxDungeonRoomMaxSize.Y = dungeonRoomMaxSize.Y;
			if (maxDungeonRoomMaxSize.Z < dungeonRoomMaxSize.Z)
				maxDungeonRoomMaxSize.Z = dungeonRoomMaxSize.Z;

			// 最も大きい部屋と最も長い通路が接続した時の長さを求める
			if (mActiveExtents.X < dungeonRoomMaxSize.X + dungeonLongestStraightPath.X)
				mActiveExtents.X = dungeonRoomMaxSize.X + dungeonLongestStraightPath.X;
			if (mActiveExtents.Y < dungeonRoomMaxSize.Y + dungeonLongestStraightPath.Y)
				mActiveExtents.Y = dungeonRoomMaxSize.Y + dungeonLongestStraightPath.Y;
			if (mActiveExtents.Z < dungeonRoomMaxSize.Z)
				mActiveExtents.Z = dungeonRoomMaxSize.Z;

			// 最も大きいグリッドの大きさを求める
			const float gridSize = dungeonGenerateActor->GetGridSize();
			if (maxGridSize < gridSize)
				maxGridSize = gridSize;
		}

		// パーティエーションの大きさを求める
		maxDungeonRoomMaxSize *= 0.5;
		maxDungeonRoomMaxSize.X = std::min(std::max(maxDungeonRoomMaxSize.X, PartitionHorizontalMinSize), PartitionHorizontalMaxSize);
		maxDungeonRoomMaxSize.Y = std::min(std::max(maxDungeonRoomMaxSize.Y, PartitionHorizontalMinSize), PartitionHorizontalMaxSize);
		maxDungeonRoomMaxSize.Z = std::min(std::max(maxDungeonRoomMaxSize.Z, PartitionVerticalMinSize), PartitionVerticalMaxSize);
		mPartitionSize = maxDungeonRoomMaxSize.X;
		if (mPartitionSize < maxDungeonRoomMaxSize.Y)
			mPartitionSize = maxDungeonRoomMaxSize.Y;
		if (mPartitionSize < maxDungeonRoomMaxSize.Z)
			mPartitionSize = maxDungeonRoomMaxSize.Z;
		mPartitionSize = std::ceil(mPartitionSize);
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
			DungeonPartitions.Add(NewObject<UDungeonPartition>(this));
		}

		// カリング距離を求めてDungeonGenerateActorに設定する
		const auto cullingDistance = mActiveExtents.Length();
		const FInt32Interval cullingDistanceRange(
			cullingDistance * 0.9,
			cullingDistance
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

	if (bEnableLoadControl)
	{
		if (const UWorld* world = GetValid(GetWorld()))
		{
			const auto activeHalfExtents = mActiveExtents / 2;
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
					Mark(FBox(playerLocation - activeHalfExtents, playerLocation + activeHalfExtents));

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
			ForceActivate();
		}

		mLastEnableLoadControl = true;
	}
	else
	{
		if (mLastEnableLoadControl != bEnableLoadControl)
		{
			mLastEnableLoadControl = bEnableLoadControl;
			ForceActivate();
		}
	}

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
#if WITH_EDITOR
		partition->SetActiveStateType(UDungeonPartition::ActiveStateType::Inactivate);
#endif
	}
}

void ADungeonMainLevelScriptActor::Mark(const FBox& activeBounds)
{
	// mBounding.Minを原点にした座標系に変換
	const FVector start = (activeBounds.Min - mBounding.Min) / mPartitionSize;
	const FVector end = (activeBounds.Max - mBounding.Min) / mPartitionSize;
	const int32_t sx = std::max<int32_t>(0.0, start.X);
	const int32_t sy = std::max<int32_t>(0.0, start.Y);
	const int32_t ex = std::min<int32_t>(std::ceil(end.X), mPartitionWidth);
	const int32_t ey = std::min<int32_t>(std::ceil(end.Y), mPartitionDepth);
	const int32_t cx = start.X + (end.X - start.X) / 2;
	const int32_t cy = start.Y + (end.Y - start.Y) / 2;
	for (int32_t y = sy; y < ey; ++y)
	{
		for (int32_t x = sx; x < ex; ++x)
		{
			const size_t index = mPartitionWidth * y + x;
			UDungeonPartition* partition = DungeonPartitions[index];
			check(IsValid(partition));
			partition->Mark(
				x == cx && y == cy ?
				UDungeonPartition::ActiveState::ActivateNear : 
				UDungeonPartition::ActiveState::ActivateFar
			);
#if WITH_EDITOR
			partition->SetActiveStateType(UDungeonPartition::ActiveStateType::Bounding);
#endif
		}
	}
}

void ADungeonMainLevelScriptActor::Mark(const FSceneView& sceneView)
{
	const FVector partitionExtent(mPartitionSize);
	const double halfPartitionSize = mPartitionSize * 0.5;
	const double visibleDistanceSquared = mActiveExtents.SquaredLength();

	size_t index = 0;
	FVector origin;
	origin.Y = mBounding.Min.Y + halfPartitionSize;
	origin.Z = mBounding.Min.Z + (mBounding.Max.Z - mBounding.Min.Z) * 0.5;
	while (origin.Y < mBounding.Max.Y + halfPartitionSize)
	{
		origin.X = mBounding.Min.X + halfPartitionSize;
		while (origin.X < mBounding.Max.X + halfPartitionSize)
		{
			const double distanceSquared = FVector::DistSquared(sceneView.ViewLocation, origin);
			if (distanceSquared < visibleDistanceSquared)
			{
				// UDungeonPartition::ActiveState::ActivateNearは計算をスキップ
				if (DungeonPartitions[index]->IsMarked() < UDungeonPartition::ActiveState::ActivateNear)
				{
					// 視点から遠すぎる場合は計算をスキップ
					if (sceneView.ViewFrustum.IntersectBox(origin, partitionExtent))
					{
						DungeonPartitions[index]->Mark(
							UDungeonPartition::ActiveState::ActivateNear
						);
#if WITH_EDITOR
						DungeonPartitions[index]->SetActiveStateType(
							UDungeonPartition::ActiveStateType::ViewFrustum
						);
#endif
					}
				}
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

		switch (partition->IsMarked())
		{
		case UDungeonPartition::ActiveState::ActivateNear:
			partition->CallPartitionActivate();
			partition->CallCastShadowActivate();
			break;

		case UDungeonPartition::ActiveState::ActivateFar:
			partition->CallPartitionActivate();
			partition->CallCastShadowInactivate();
			break;

		case UDungeonPartition::ActiveState::Inactivate:
			if (partition->UpdateInactivateRemainTimer(deltaSeconds))
				partition->CallPartitionActivate();
			else
				partition->CallPartitionInactivate();
			partition->CallCastShadowInactivate();
			break;
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

bool ADungeonMainLevelScriptActor::IsEnableLoadControl() const noexcept
{
	return bEnableLoadControl;
}

void ADungeonMainLevelScriptActor::EnableLoadControl(const bool enable) noexcept
{
	bEnableLoadControl = enable;
}

#if WITH_EDITOR
void ADungeonMainLevelScriptActor::DrawDebugInformation() const
{
	// パーティエーションの状態を線で描画します
	const double h = (mBounding.Max.Z - mBounding.Min.Z) * 0.5;
	const double z = mBounding.Min.Z;
	constexpr double margin = 10;
	const FVector halfSize(mPartitionSize * 0.5 - margin, mPartitionSize * 0.5 - margin, h - margin);
	for (double y = mBounding.Min.Y; y < mBounding.Max.Y; y += mPartitionSize)
	{
		for (double x = mBounding.Min.X; x < mBounding.Max.X; x += mPartitionSize)
		{
			if (const auto* partition = Find(FVector(x + 1, y + 1, z)))
			{
				static const std::array<FColor, 3> colors = {
					{
						FColor::Blue,
						FColor::Green,
						FColor::Red,
					} };
				auto color = colors.at(static_cast<uint8>(partition->IsMarked()));
				if (partition->GetActiveStateType() == UDungeonPartition::ActiveStateType::ViewFrustum)
				{
					color.R -= color.R / 16;
					color.G -= color.G / 16;
					color.B -= color.B / 16;
				}
				const FVector location(x, y, z);
				UKismetSystemLibrary::DrawDebugBox(
					GetWorld(),
					location + halfSize,
					halfSize,
					color,
					FRotator::ZeroRotator,
					0.f,
					10.f
				);
			}
		}
	}
}
#endif
