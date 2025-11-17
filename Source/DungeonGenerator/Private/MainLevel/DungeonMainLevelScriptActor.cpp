/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "MainLevel/DungeonPartition.h"
#include "DungeonGenerateActor.h"
#include "Core/Debug/Debug.h"
#include <DrawDebugHelpers.h>
#include <SceneView.h>
#include <UnrealClient.h>
#include <Async/ParallelFor.h>
#include <Components/PointLightComponent.h>
#include <Engine/GameViewportClient.h>
#include <Engine/LocalPlayer.h>
#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <Kismet/GameplayStatics.h>
#include <algorithm>

#if WITH_EDITOR
#include <array>
#endif

ADungeonMainLevelScriptActor::ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, mActiveRegionExtent(0)
	, mLastEnableLoadControl(bEnableLoadControl)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	mBounding.Init();
}

void ADungeonMainLevelScriptActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	auto* level = GetLevel();
	if (!IsValid(level))
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Initialization of ADungeonMainLevelScriptActor is aborted because the level cannot be obtained"));
		return;
	}

	FVector2D dungeonMaxLongestStraightPath(0, 0);
	FVector dungeonRoomMaxSize(0, 0, 0);
	TArray<ADungeonGenerateActor*> dungeonGenerateActors;
	float maxGridSize = 0.f;

	// DungeonGenerateActorの部屋の大きさからアクティブ範囲を求める
	for (const auto& actor : level->Actors)
	{
		auto* dungeonGenerateActor = Cast<ADungeonGenerateActor>(actor);
		if (!IsValid(dungeonGenerateActor))
			continue;
		dungeonGenerateActors.Add(dungeonGenerateActor);

		// 世界全体の大きさを求める
		mBounding += dungeonGenerateActor->CalculateBoundingBox();

		// 最も長い通路の長さを求める
		const auto& dungeonLongestStraightPath = dungeonGenerateActor->GetLongestStraightPath();
		if (dungeonMaxLongestStraightPath.X < dungeonLongestStraightPath.X)
			dungeonMaxLongestStraightPath.X = dungeonLongestStraightPath.X;
		if (dungeonMaxLongestStraightPath.Y < dungeonLongestStraightPath.Y)
			dungeonMaxLongestStraightPath.Y = dungeonLongestStraightPath.Y;

		// 最も大きい部屋の大きさを求める
		const auto& dungeonRoomSize = dungeonGenerateActor->GetRoomMaxSizeWithMargin(2);
		if (dungeonRoomMaxSize.X < dungeonRoomSize.X)
			dungeonRoomMaxSize.X = dungeonRoomSize.X;
		if (dungeonRoomMaxSize.Y < dungeonRoomSize.Y)
			dungeonRoomMaxSize.Y = dungeonRoomSize.Y;
		if (dungeonRoomMaxSize.Z < dungeonRoomSize.Z)
			dungeonRoomMaxSize.Z = dungeonRoomSize.Z;

		// 最も大きいグリッドの大きさを求める
		const float gridSize = dungeonGenerateActor->GetGridSize();
		if (maxGridSize < gridSize)
			maxGridSize = gridSize;

		DUNGEON_GENERATOR_LOG(TEXT("LongestStraightPath: X=%f, Y=%f (%s)"), dungeonLongestStraightPath.X, dungeonLongestStraightPath.Y, *actor->GetName());
		DUNGEON_GENERATOR_LOG(TEXT("RoomMaxSize: X=%f, Y=%f (%s)"), dungeonRoomSize.X, dungeonRoomSize.Y, *actor->GetName());
		DUNGEON_GENERATOR_LOG(TEXT("GridSize=%f (%s)"), gridSize, *actor->GetName());
	}

	DUNGEON_GENERATOR_LOG(TEXT("DungeonRoomMaxSize (%f,%f,%f)"), dungeonRoomMaxSize.X, dungeonRoomMaxSize.Y, dungeonRoomMaxSize.Z);
	DUNGEON_GENERATOR_LOG(TEXT("DungeonMaxLongestStraightPath (%f,%f)"), dungeonMaxLongestStraightPath.X, dungeonMaxLongestStraightPath.Y);

	// 一番広い部屋の範囲を求める
	mActiveRegionExtent = dungeonRoomMaxSize;
	mActiveRegionExtent *= static_cast<double>(0.5f * ActivationRangeScale);

	// 最も大きい部屋と最も長い通路が接続した時の長さを求める
	mActiveViewRange = dungeonRoomMaxSize.Z;
	if (mActiveViewRange < dungeonRoomMaxSize.X + dungeonMaxLongestStraightPath.X)
		mActiveViewRange = dungeonRoomMaxSize.X + dungeonMaxLongestStraightPath.X;
	if (mActiveViewRange < dungeonRoomMaxSize.Y + dungeonMaxLongestStraightPath.Y)
		mActiveViewRange = dungeonRoomMaxSize.Y + dungeonMaxLongestStraightPath.Y;
	mActiveViewRange *= ActivationRangeScale;

	// パーティエーションの大きさを求める
	dungeonRoomMaxSize.X = std::min(std::max(dungeonRoomMaxSize.X, PartitionHorizontalMinSize), PartitionHorizontalMaxSize);
	dungeonRoomMaxSize.Y = std::min(std::max(dungeonRoomMaxSize.Y, PartitionHorizontalMinSize), PartitionHorizontalMaxSize);
	dungeonRoomMaxSize.Z = std::min(std::max(dungeonRoomMaxSize.Z, PartitionVerticalMinSize), PartitionVerticalMaxSize);
	mPartitionSize = dungeonRoomMaxSize.X;
	if (mPartitionSize < dungeonRoomMaxSize.Y)
		mPartitionSize = dungeonRoomMaxSize.Y;
	if (mPartitionSize < dungeonRoomMaxSize.Z)
		mPartitionSize = dungeonRoomMaxSize.Z;
	mPartitionSize = std::ceil(mPartitionSize * 0.5);

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
		const FInt32Interval cullingDistanceRange(
			mActiveViewRange,
			mActiveViewRange * 1.1f
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
			Begin();
			for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
			{
				const auto* playerController = iterator->Get();
				if (IsValid(playerController))
				{
					const auto& playerPawn = playerController->GetPawn();
					if (IsValid(playerPawn) && playerPawn->IsPlayerControlled())
					{
						// プレイヤー周辺をマークする
						Mark(playerPawn->GetActorLocation());

						// 視錐台に含まれているならマークする
						Mark(GetSceneView(playerController));
					}
				}
			}
			End(deltaSeconds);

			// ポイントライトおよびスポットライトの影を落とすか制御
			if (MaxShadowCastingPointAndSpotLights > 0)
				UpdateShadowCastingPointAndSpotLights();
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
			ForceActivateShadowCastingPointAndSpotLights();
		}
	}

#if WITH_EDITOR
	if (ShowDebugInformation)
		DrawDebugInformation();
#endif
}

/*
 * 点で検索しているので、巨大なアクターは誤判定に注意してください。
 * スレッドセーフにして下さい
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
	return Index(x, y);
}

/*
 * スレッドセーフにして下さい
 */
UDungeonPartition* ADungeonMainLevelScriptActor::Index(const size_t x, const size_t y) const noexcept
{
	check(x < mPartitionWidth);
	check(y < mPartitionDepth);
	const size_t index = mPartitionWidth * y + x;
	UDungeonPartition* partition = DungeonPartitions[index];
	check(IsValid(partition));
	return partition;
}

const FSceneView* ADungeonMainLevelScriptActor::GetSceneView(const APlayerController* playerController) const
{
	auto* localPlayer = Cast<ULocalPlayer>(playerController->Player);
	if (localPlayer == nullptr)
		return nullptr;

	auto* viewport = localPlayer->ViewportClient->Viewport;
	FSceneViewFamilyContext sceneViewFamilyContext(
		FSceneViewFamily::ConstructionValues(
			localPlayer->ViewportClient->Viewport,
			GetWorld()->Scene,
			localPlayer->ViewportClient->EngineShowFlags
		).SetRealtimeUpdate(true)
	);

	FVector viewLocation;
	FRotator viewRotation;
	const auto* sceneView = localPlayer->CalcSceneView(&sceneViewFamilyContext, viewLocation, viewRotation, viewport);

#if WITH_EDITOR
	if (ShowFrustumInformation)
	{
		const FIntPoint viewportSize = viewport->GetSizeXY();
		const auto AspectRatio = static_cast<float>(viewportSize.X) / static_cast<float>(viewportSize.Y);
		DrawFrustumLines(
			GetWorld(),
			sceneView->ViewLocation,
			sceneView->ViewRotation,
			sceneView->FOV,
			AspectRatio,
			sceneView->NearClippingDistance,
			mActiveViewRange,
			FColor::Green,
			0.f,
			10.f
		);
	}
#endif

	return sceneView;
}

void ADungeonMainLevelScriptActor::Begin() const
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->Unmark();
	}
}

void ADungeonMainLevelScriptActor::Mark(const FVector& playerLocation) const
{
	// mBounding.Minを原点にした座標系に変換
	const FBox activeBounds(playerLocation - mActiveRegionExtent, playerLocation + mActiveRegionExtent);
	const FVector start = (activeBounds.Min - mBounding.Min) / mPartitionSize;
	const FVector end = (activeBounds.Max - mBounding.Min) / mPartitionSize;
	const int32_t sx = std::max<int32_t>(0.0, start.X);
	const int32_t sy = std::max<int32_t>(0.0, start.Y);
	const int32_t ex = std::min<int32_t>(std::ceil(end.X), mPartitionWidth);
	const int32_t ey = std::min<int32_t>(std::ceil(end.Y), mPartitionDepth);
	for (int32_t y = sy; y < ey; ++y)
	{
		for (int32_t x = sx; x < ex; ++x)
		{
			Index(x, y)->Mark();
		}
	}
}

void ADungeonMainLevelScriptActor::Mark(const FSceneView* sceneView) const
{
	if (sceneView == nullptr)
		return;

	const FVector partitionExtent(mPartitionSize * 0.5);
	ParallelFor(DungeonPartitions.Num(), [this, sceneView, partitionExtent](const size_t index)
		{
			const double x = index % mPartitionWidth * mPartitionSize;
			const double y = index / mPartitionWidth * mPartitionSize;
			const FVector origin(
				mBounding.Min.X + x + partitionExtent.X,
				mBounding.Min.Y + y + partitionExtent.Y,
				mBounding.Min.Z + (mBounding.Max.Z - mBounding.Min.Z) * 0.5
			);

			// 視点から遠すぎる場合は計算をスキップ
			const double distance = FVector::Dist2D(sceneView->ViewLocation, origin);
			if (distance < mActiveViewRange)
			{
				// パーティエーションが視錐台と交差している
				if (sceneView->ViewFrustum.IntersectBox(origin, partitionExtent))
				{
					DungeonPartitions[index]->Mark();
				}
			}
		},
		EParallelForFlags::None
	);
}

void ADungeonMainLevelScriptActor::End(const float deltaSeconds) const
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));

		if (partition->IsMarked())
		{
			partition->CallPartitionActivate(true);
		}
		else
		{
			if (partition->UpdatePartitionInactivateRemainTimer(deltaSeconds))
				partition->CallPartitionActivate(false);
			else
				partition->CallPartitionInactivate();
		}
	}
}

void ADungeonMainLevelScriptActor::ForceActivate()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->CallPartitionActivate(true);
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

void ADungeonMainLevelScriptActor::UpdateShadowCastingPointAndSpotLights()
{
	const auto* playerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(playerController))
		return;

	// ポイントライト継承クラスを回収するコンテナ
	using PointLightPair = std::pair<float, UPointLightComponent*>;
	std::vector<PointLightPair> pointLightComponents;

	// カメラの位置と向きを取得
	FVector cameraLocation;
	FRotator cameraRotation;
	playerController->GetPlayerViewPoint(cameraLocation, cameraRotation);
	const FVector cameraDirection = cameraRotation.Vector();

	// 有効な全てのパーティエーションを調べる
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));

		if (!partition->IsMarked())
		{
			partition->EachDungeonComponentActivatorComponent([](const UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
				{
					dungeonComponentActivatorComponent->EachControlledLightCastShadow([](UPointLightComponent* pointLightComponent)
						{
							// 強制的に影を落とさない
							pointLightComponent->SetCastShadows(false);
						}
					);
				}
			);
		}
		else
		{
			partition->EachDungeonComponentActivatorComponent([&pointLightComponents, &cameraLocation, &cameraDirection](const UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
				{
					dungeonComponentActivatorComponent->EachControlledLightCastShadow([&pointLightComponents, &cameraLocation, &cameraDirection](UPointLightComponent* pointLightComponent)
						{
							const auto toTarget = pointLightComponent->GetComponentLocation() - cameraLocation;
							const float distance = toTarget.Size();
							if (!FMath::IsNearlyZero(distance))
							{
								const auto direction = toTarget / distance;
								const float dot = FVector::DotProduct(cameraDirection, direction);
								// 正面に近い（dot が大きい）ほど値が大きい
								// 距離が短い（distance が小さい）ほど値が大きい
								const float score = FMath::Max(FMath::Cos(FMath::DegreesToRadians(60.f)), dot) / FMath::Sqrt(distance);
								pointLightComponents.emplace_back(score, pointLightComponent);
							}
						}
					);
				}
			);
		}
	}

	// 降順に並び替える
	std::sort(pointLightComponents.begin(), pointLightComponents.end(), [](const PointLightPair& l, const PointLightPair& r)
		{
			return l.first > r.first;
		}
	);
	
	// UPointLightComponentへ反映する
	{
		size_t i = 0;
		const auto enablePointLightComponentSize = std::min<size_t>(MaxShadowCastingPointAndSpotLights, pointLightComponents.size());
		while (i < enablePointLightComponentSize)
		{
			pointLightComponents[i].second->SetCastShadows(true);
			++i;
		}
		while (i < pointLightComponents.size())
		{
			pointLightComponents[i].second->SetCastShadows(false);
			++i;
		}
	}
}

void ADungeonMainLevelScriptActor::ForceActivateShadowCastingPointAndSpotLights()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));

		if (!partition->IsMarked())
		{
			partition->EachDungeonComponentActivatorComponent([](const UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
				{
					dungeonComponentActivatorComponent->EachControlledLightCastShadow([](UPointLightComponent* pointLightComponent)
						{
							// 強制的に影を落とす
							pointLightComponent->SetCastShadows(true);
						}
					);
				}
			);
		}
	}
}

#if WITH_EDITOR
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

void ADungeonMainLevelScriptActor::DrawFrustumLines(
	UWorld* world, const FVector& viewLocation, const FRotator& viewRotation,
	float FovY, float aspect, float nearDistance, float farDistance,
	const FColor& color, float lifeTime, float thickness)
{
	const FRotationMatrix R(viewRotation);
	const FVector F = R.GetScaledAxis(EAxis::X); // UEは+Xが前
	const FVector Rv = R.GetScaledAxis(EAxis::Y);
	const FVector U = R.GetScaledAxis(EAxis::Z);

	const float tanHalf = FMath::Tan(FMath::DegreesToRadians(FovY) * 0.5f);
	const float nH = nearDistance * tanHalf;
	const float nW = nH * aspect;
	const float fH = farDistance * tanHalf;
	const float fW = fH * aspect;

	const FVector nC = viewLocation + F * nearDistance;
	const FVector fC = viewLocation + F * farDistance;

	const FVector N[4] = {
		nC + U * nH - Rv * nW, nC + U * nH + Rv * nW, nC - U * nH + Rv * nW, nC - U * nH - Rv * nW
	};
	const FVector Fp[4] = {
		fC + U * fH - Rv * fW, fC + U * fH + Rv * fW, fC - U * fH + Rv * fW, fC - U * fH - Rv * fW
	};

	auto L = [&](const FVector& A, const FVector& B) {
		DrawDebugLine(world, A, B, color, false, lifeTime, 0, thickness);
		};

	// ニア面・ファー面
	L(N[0], N[1]); L(N[1], N[2]); L(N[2], N[3]); L(N[3], N[0]);
	L(Fp[0], Fp[1]); L(Fp[1], Fp[2]); L(Fp[2], Fp[3]); L(Fp[3], Fp[0]);
	// 側面
	for (int i = 0; i < 4; ++i) L(N[i], Fp[i]);
}

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
				const auto color = partition->IsMarked() ? FColor::Red : FColor::Blue;
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

	// 有効な範囲を黄色い線で描画します
	if (const UWorld* world = GetValid(GetWorld()))
	{
		for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
		{
			if (const auto* playerController = iterator->Get())
			{
				const auto& playerPawn = playerController->GetPawn();
				if (IsValid(playerPawn) && playerPawn->IsPlayerControlled())
				{
					UKismetSystemLibrary::DrawDebugBox(
						GetWorld(),
						playerPawn->GetActorLocation(),
						mActiveRegionExtent,
						FColor::Yellow,
						FRotator::ZeroRotator,
						0.f,
						10.f
					);
				}
			}
		}
	}
}
#endif
