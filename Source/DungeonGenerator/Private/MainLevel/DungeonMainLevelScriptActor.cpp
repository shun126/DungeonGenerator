/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "MainLevel/DungeonPartition.h"
#include "DungeonGenerateActor.h"
#include "Core/Generator.h"
#include "Core/Debug/Debug.h"
#include "Core/Voxelization/Grid.h"
#include "Core/Voxelization/Voxel.h"
#include "Parameter/DungeonGenerateParameter.h"
#include <DrawDebugHelpers.h>
#include <Async/ParallelFor.h>
#include <Components/PointLightComponent.h>
#include <Engine/Level.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <GameFramework/Pawn.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <array>
#include <algorithm>
#include <functional>
#include <limits>

namespace
{
	bool IsTraversableGrid(const dungeon::Grid& grid) noexcept
	{
		return
			grid.IsKindOfRoomType() ||
			grid.IsKindOfAisleType() ||
			grid.IsKindOfSlopeType();
	}

	const std::array<FIntVector, 6> NeighborOffsets = {
		FIntVector(1, 0, 0),
		FIntVector(-1, 0, 0),
		FIntVector(0, 1, 0),
		FIntVector(0, -1, 0),
		FIntVector(0, 0, 1),
		FIntVector(0, 0, -1),
	};

	constexpr int32 SparsePartitionFallbackSearchRadius = 1;

	int32 GreatestCommonDivisor(int32 left, int32 right) noexcept
	{
		left = FMath::Abs(left);
		right = FMath::Abs(right);
		while (right != 0)
		{
			const int32 remain = left % right;
			left = right;
			right = remain;
		}
		return FMath::Max(left, 1);
	}

	int32 LeastCommonMultiple(const int32 left, const int32 right) noexcept
	{
		if (left == 0 || right == 0)
			return FMath::Max(left, right);
		return FMath::Abs(left / GreatestCommonDivisor(left, right) * right);
	}

	int32 ComputeAutoPartitionGridCount(const int32 minimumRoomSpanInGrid, const int32 minimumGridCount, const int32 maximumGridCount) noexcept
	{
		const int32 safeRoomSpanInGrid = FMath::Max(minimumRoomSpanInGrid, 1);
		return FMath::Clamp(
			FMath::FloorToInt(static_cast<float>(safeRoomSpanInGrid) * 0.5f),
			minimumGridCount,
			maximumGridCount
		);
	}

	double ComputeSquaredDistanceToBounds(const FBox& bounds, const FVector& worldLocation) noexcept
	{
		const FVector nearestPoint(
			FMath::Clamp(worldLocation.X, bounds.Min.X, bounds.Max.X),
			FMath::Clamp(worldLocation.Y, bounds.Min.Y, bounds.Max.Y),
			FMath::Clamp(worldLocation.Z, bounds.Min.Z, bounds.Max.Z)
		);
		return FVector::DistSquared(nearestPoint, worldLocation);
	}

	double ComputeSquaredDistanceBetweenBounds(const FBox& left, const FBox& right) noexcept
	{
		const FVector delta(
			left.Min.X > right.Max.X ? left.Min.X - right.Max.X : (right.Min.X > left.Max.X ? right.Min.X - left.Max.X : 0.f),
			left.Min.Y > right.Max.Y ? left.Min.Y - right.Max.Y : (right.Min.Y > left.Max.Y ? right.Min.Y - left.Max.Y : 0.f),
			left.Min.Z > right.Max.Z ? left.Min.Z - right.Max.Z : (right.Min.Z > left.Max.Z ? right.Min.Z - left.Max.Z : 0.f)
		);
		return delta.SizeSquared();
	}
}

ADungeonMainLevelScriptActor::ADungeonMainLevelScriptActor(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
	, mLastEnableLoadControl(false)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	mBounding.Init();
}

void ADungeonMainLevelScriptActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	ExecutePartitionBuild(FPartitionBuildOptions{ EPartitionBuildReason::InitialLoad });
}

void ADungeonMainLevelScriptActor::RebuildSparsePartitionGraphAndRefresh()
{
	ExecutePartitionBuild(FPartitionBuildOptions{ EPartitionBuildReason::RuntimeRebuild });
}

/*
 * Executes the shared partition build pipeline and applies mode-specific post processing.
 * 共通のパーティション構築パイプラインを実行し、実行モードごとの後処理を適用します。
 */
bool ADungeonMainLevelScriptActor::ExecutePartitionBuild(const FPartitionBuildOptions& options)
{
	ResetPartitionBuildState();

	FPartitionBuildContext context;
	if (!TryPreparePartitionBuildContext(options, context))
	{
		if (options.ShouldUpdateTickState())
			SetActorTickEnabled(false);
		return false;
	}

	CollectPartitionBuildActors(options, context);
	if (!ValidateCollectedPartitionBuildActors(options, context))
	{
		FinalizePartitionBuild(options, false);
		return false;
	}

	ComputePartitionBuildMetrics(context);
	if (!ValidatePartitionBuildContext(context))
	{
		FinalizePartitionBuild(options, false);
		return false;
	}

	CommitPartitionBuildContext(context);
	BuildPartitionRuntimeData(context);
	ApplyDungeonActorCullDistances(context);
	FinalizePartitionBuild(options, true);
	return true;
}

/*
 * Validates the level and seeds the build context before actor collection starts.
 * actor 収集開始前にレベルを検証し、構築コンテキストの初期値を設定します。
 */
bool ADungeonMainLevelScriptActor::TryPreparePartitionBuildContext(const FPartitionBuildOptions& options, FPartitionBuildContext& context) const
{
	context.Level = GetLevel();
	if (!IsValid(context.Level))
	{
		const TCHAR* message = options.Reason == EPartitionBuildReason::RuntimeRebuild
			? TEXT("Sparse partition graph rebuild was aborted because the level cannot be obtained")
			: TEXT("Initialization of ADungeonMainLevelScriptActor is aborted because the level cannot be obtained");
		DUNGEON_GENERATOR_ERROR(TEXT("%s"), message);
		return false;
	}

	return true;
}

/*
 * Collects dungeon actors that satisfy the current build policy.
 * 現在の構築ポリシーを満たすダンジョン actor を収集します。
 */
void ADungeonMainLevelScriptActor::CollectPartitionBuildActors(const FPartitionBuildOptions& options, FPartitionBuildContext& context)
{
	for (const auto& actor : context.Level->Actors)
	{
		auto* dungeonGenerateActor = Cast<ADungeonGenerateActor>(actor);
		if (!IsValid(dungeonGenerateActor))
			continue;
		if (options.RequiresGeneratedDungeon())
		{
			if (!IsValid(dungeonGenerateActor->mParameter))
				continue;
			if (dungeonGenerateActor->GetGenerator() == nullptr)
				continue;
		}

		context.DungeonGenerateActors.Add(dungeonGenerateActor);
		AccumulatePartitionBuildActor(dungeonGenerateActor, context);
	}
}

/*
 * Adds the metrics contributed by a single dungeon actor into the shared build context.
 * 1 つのダンジョン actor が持つ集計値を共通コンテキストへ加算します。
 */
void ADungeonMainLevelScriptActor::AccumulatePartitionBuildActor(const ADungeonGenerateActor* dungeonGenerateActor, FPartitionBuildContext& context)
{
	context.Bounding += dungeonGenerateActor->CalculateBoundingBox();

	const auto& dungeonLongestStraightPath = dungeonGenerateActor->GetLongestStraightPath();
	context.DungeonMaxLongestStraightPath.X = FMath::Max(context.DungeonMaxLongestStraightPath.X, dungeonLongestStraightPath.X);
	context.DungeonMaxLongestStraightPath.Y = FMath::Max(context.DungeonMaxLongestStraightPath.Y, dungeonLongestStraightPath.Y);

	const auto& dungeonRoomSize = dungeonGenerateActor->GetRoomMaxSizeWithMargin(2);
	context.DungeonRoomMaxSize.X = FMath::Max(context.DungeonRoomMaxSize.X, dungeonRoomSize.X);
	context.DungeonRoomMaxSize.Y = FMath::Max(context.DungeonRoomMaxSize.Y, dungeonRoomSize.Y);
	context.DungeonRoomMaxSize.Z = FMath::Max(context.DungeonRoomMaxSize.Z, dungeonRoomSize.Z);

	const float gridSize = dungeonGenerateActor->GetGridSize();
	context.MaxGridSize = FMath::Max(context.MaxGridSize, gridSize);
	if (IsValid(dungeonGenerateActor->mParameter))
	{
		const FDungeonGridSize dungeonGridSize = dungeonGenerateActor->mParameter->GetGridSize();
		context.MaxGridSize = FMath::Max(context.MaxGridSize, FMath::Max(dungeonGridSize.HorizontalSize, dungeonGridSize.VerticalSize));

		const int32 horizontalGridSize = FMath::Max(1, FMath::RoundToInt(dungeonGridSize.HorizontalSize));
		const int32 verticalGridSize = FMath::Max(1, FMath::RoundToInt(dungeonGridSize.VerticalSize));
		context.CommonHorizontalGridSize = context.CommonHorizontalGridSize == 0 ? horizontalGridSize : LeastCommonMultiple(context.CommonHorizontalGridSize, horizontalGridSize);
		context.CommonVerticalGridSize = context.CommonVerticalGridSize == 0 ? verticalGridSize : LeastCommonMultiple(context.CommonVerticalGridSize, verticalGridSize);
		context.MinimumRoomWidthInGrid = FMath::Min(context.MinimumRoomWidthInGrid, dungeonGenerateActor->mParameter->GetRoomWidth().Min);
		context.MinimumRoomDepthInGrid = FMath::Min(context.MinimumRoomDepthInGrid, dungeonGenerateActor->mParameter->GetRoomDepth().Min);
		context.MinimumRoomHeightInGrid = FMath::Min(context.MinimumRoomHeightInGrid, dungeonGenerateActor->mParameter->GetRoomHeight().Min);
	}

	DUNGEON_GENERATOR_LOG(TEXT("LongestStraightPath: X=%f, Y=%f (%s)"), dungeonLongestStraightPath.X, dungeonLongestStraightPath.Y, *dungeonGenerateActor->GetName());
	DUNGEON_GENERATOR_LOG(TEXT("RoomMaxSize: X=%f, Y=%f (%s)"), dungeonRoomSize.X, dungeonRoomSize.Y, *dungeonGenerateActor->GetName());
	DUNGEON_GENERATOR_LOG(TEXT("GridSize=%f (%s)"), gridSize, *dungeonGenerateActor->GetName());
}

/*
 * Rejects rebuild requests that require already-generated dungeon actors but found none.
 * 生成済みダンジョン actor が必須なのに 1 つも見つからない再構築要求を弾きます。
 */
bool ADungeonMainLevelScriptActor::ValidateCollectedPartitionBuildActors(const FPartitionBuildOptions& options, const FPartitionBuildContext& context)
{
	if (options.RequiresGeneratedDungeon() && context.DungeonGenerateActors.IsEmpty())
	{
		DUNGEON_GENERATOR_LOG(TEXT("Sparse partition graph rebuild skipped because no generated dungeon actors are available"));
		return false;
	}

	return true;
}

/*
 * Computes the derived visibility and partition sizing metrics from the collected actors.
 * 収集済み actor から visibility とパーティション寸法の派生値を計算します。
 */
void ADungeonMainLevelScriptActor::ComputePartitionBuildMetrics(FPartitionBuildContext& context) const
{
	DUNGEON_GENERATOR_LOG(TEXT("DungeonRoomMaxSize (%f,%f,%f)"), context.DungeonRoomMaxSize.X, context.DungeonRoomMaxSize.Y, context.DungeonRoomMaxSize.Z);
	DUNGEON_GENERATOR_LOG(TEXT("DungeonMaxLongestStraightPath (%f,%f)"), context.DungeonMaxLongestStraightPath.X, context.DungeonMaxLongestStraightPath.Y);

	context.TheoreticalMaxVisibilityDistance = context.DungeonRoomMaxSize.Z;
	if (context.TheoreticalMaxVisibilityDistance < context.DungeonRoomMaxSize.X + context.DungeonMaxLongestStraightPath.X)
		context.TheoreticalMaxVisibilityDistance = context.DungeonRoomMaxSize.X + context.DungeonMaxLongestStraightPath.X;
	if (context.TheoreticalMaxVisibilityDistance < context.DungeonRoomMaxSize.Y + context.DungeonMaxLongestStraightPath.Y)
		context.TheoreticalMaxVisibilityDistance = context.DungeonRoomMaxSize.Y + context.DungeonMaxLongestStraightPath.Y;
	context.TheoreticalMaxVisibilityDistance *= ActivationRangeScale;

	if (context.MinimumRoomWidthInGrid == TNumericLimits<int32>::Max())
		context.MinimumRoomWidthInGrid = AutoPartitionHorizontalMinGridCount * 2;
	if (context.MinimumRoomDepthInGrid == TNumericLimits<int32>::Max())
		context.MinimumRoomDepthInGrid = AutoPartitionHorizontalMinGridCount * 2;
	if (context.MinimumRoomHeightInGrid == TNumericLimits<int32>::Max())
		context.MinimumRoomHeightInGrid = AutoPartitionVerticalMinGridCount;
	if (context.CommonHorizontalGridSize <= 0)
		context.CommonHorizontalGridSize = FMath::Max(1, FMath::RoundToInt(context.MaxGridSize));
	if (context.CommonVerticalGridSize <= 0)
		context.CommonVerticalGridSize = FMath::Max(1, FMath::RoundToInt(context.MaxGridSize));

	const int32 autoHorizontalPartitionGridCountX = ComputeAutoPartitionGridCount(
		context.MinimumRoomWidthInGrid,
		AutoPartitionHorizontalMinGridCount,
		AutoPartitionHorizontalMaxGridCount
	);
	const int32 autoHorizontalPartitionGridCountY = ComputeAutoPartitionGridCount(
		context.MinimumRoomDepthInGrid,
		AutoPartitionHorizontalMinGridCount,
		AutoPartitionHorizontalMaxGridCount
	);
	const int32 autoVerticalPartitionGridCount = ComputeAutoPartitionGridCount(
		context.MinimumRoomHeightInGrid,
		AutoPartitionVerticalMinGridCount,
		AutoPartitionVerticalMaxGridCount
	);

	context.PartitionGridCount = FIntVector(
		PartitionGridCountOverride.X > 0 ? PartitionGridCountOverride.X : autoHorizontalPartitionGridCountX,
		PartitionGridCountOverride.Y > 0 ? PartitionGridCountOverride.Y : autoHorizontalPartitionGridCountY,
		PartitionGridCountOverride.Z > 0 ? PartitionGridCountOverride.Z : autoVerticalPartitionGridCount
	);
	context.PartitionGridCount.X = FMath::Max(1, context.PartitionGridCount.X);
	context.PartitionGridCount.Y = FMath::Max(1, context.PartitionGridCount.Y);
	context.PartitionGridCount.Z = FMath::Max(1, context.PartitionGridCount.Z);

	context.PartitionWorldSize = FVector(
		static_cast<float>(context.CommonHorizontalGridSize * context.PartitionGridCount.X),
		static_cast<float>(context.CommonHorizontalGridSize * context.PartitionGridCount.Y),
		static_cast<float>(context.CommonVerticalGridSize * context.PartitionGridCount.Z)
	);
	DUNGEON_GENERATOR_LOG(TEXT("MinimumRoomGridSize (%d,%d,%d)"), context.MinimumRoomWidthInGrid, context.MinimumRoomDepthInGrid, context.MinimumRoomHeightInGrid);
	DUNGEON_GENERATOR_LOG(TEXT("CommonGridSize (%d,%d)"), context.CommonHorizontalGridSize, context.CommonVerticalGridSize);
	DUNGEON_GENERATOR_LOG(TEXT("PartitionGridCount (%d,%d,%d)"), context.PartitionGridCount.X, context.PartitionGridCount.Y, context.PartitionGridCount.Z);
	DUNGEON_GENERATOR_LOG(TEXT("PartitionWorldSize (%f,%f,%f)"), context.PartitionWorldSize.X, context.PartitionWorldSize.Y, context.PartitionWorldSize.Z);
}

/*
 * Confirms that the computed build context can be committed into runtime state.
 * 計算済みコンテキストを runtime 状態へ確定反映できるか確認します。
 */
bool ADungeonMainLevelScriptActor::ValidatePartitionBuildContext(const FPartitionBuildContext& context)
{
	if (!context.Bounding.IsValid)
	{
		DUNGEON_GENERATOR_ERROR(TEXT("Place the DungeonGenerateActor on the level"));
		return false;
	}

	return true;
}

/*
 * Clears the current partition-related runtime state before a rebuild starts.
 * 再構築開始前に現在のパーティション関連 runtime 状態を消去します。
 */
void ADungeonMainLevelScriptActor::ResetPartitionBuildState()
{
	mBounding.Init();
	mTheoreticalMaxVisibilityDistance = 0.f;
	mPartitionWorldSize = FVector::OneVector;
	mPartitionGridCount = FIntVector(AutoPartitionHorizontalMinGridCount, AutoPartitionHorizontalMinGridCount, AutoPartitionVerticalMinGridCount);
	DungeonPartitions.Reset();
	mPartitionIndexByCell.Reset();
	ResetPrecomputedPartitionVisibility();
	ResetPartitionTransitionQueue();
}

/*
 * Copies the computed build context into the actor members used at runtime.
 * 計算済みコンテキストを runtime で使う actor member へ反映します。
 */
void ADungeonMainLevelScriptActor::CommitPartitionBuildContext(const FPartitionBuildContext& context)
{
	mBounding = context.Bounding.ExpandBy(context.MaxGridSize);
	mTheoreticalMaxVisibilityDistance = context.TheoreticalMaxVisibilityDistance;
	mPartitionWorldSize = context.PartitionWorldSize;
	mPartitionGridCount = context.PartitionGridCount;
}

/*
 * Rebuilds graph, visibility, and transition data using the committed runtime members.
 * 確定反映済み runtime member を使って graph、visibility、遷移情報を再構築します。
 */
void ADungeonMainLevelScriptActor::BuildPartitionRuntimeData(const FPartitionBuildContext& context)
{
	BuildSparsePartitionGraph(context.DungeonGenerateActors);
	BuildPartitionVisibilitySamples(context.DungeonGenerateActors);
	BuildPartitionConnectedComponents();
	BuildPrecomputedPartitionVisibility();
	ResetPartitionTransitionQueue();
}

/*
 * Pushes the newly computed culling distance range back to all collected dungeon actors.
 * 新しく計算したカリング距離範囲を収集済みダンジョン actor へ反映します。
 */
void ADungeonMainLevelScriptActor::ApplyDungeonActorCullDistances(const FPartitionBuildContext& context) const
{
	const FInt32Interval cullingDistanceRange = ComputeTerrainCullingDistanceRange();
	for (ADungeonGenerateActor* dungeonGenerateActor : context.DungeonGenerateActors)
	{
		dungeonGenerateActor->SetInstancedMeshCullDistance(cullingDistanceRange);
	}
}

/*
 * Applies runtime synchronization that differs between initial load and runtime rebuild.
 * 初期化時と再構築時で異なる runtime 同期処理を適用します。
 */
void ADungeonMainLevelScriptActor::FinalizePartitionBuild(const FPartitionBuildOptions& options, const bool buildSucceeded)
{
	if (options.ShouldSyncRuntimeState())
	{
		RefreshActivatorComponentRegistrations();
		ApplyCurrentPartitionActivationState();
	}

	if (options.ShouldUpdateTickState())
		SetActorTickEnabled(buildSucceeded);
}

void ADungeonMainLevelScriptActor::RefreshActivatorComponentRegistrations()
{
	UWorld* world = GetWorld();
	if (!IsValid(world))
		return;

	for (TActorIterator<AActor> iterator(world); iterator; ++iterator)
	{
		AActor* actor = *iterator;
		if (!IsValid(actor))
			continue;

		TInlineComponentArray<UDungeonComponentActivatorComponent*> activatorComponents;
		actor->GetComponents<UDungeonComponentActivatorComponent>(activatorComponents);
		for (UDungeonComponentActivatorComponent* activatorComponent : activatorComponents)
		{
			if (IsValid(activatorComponent))
				activatorComponent->RefreshPartitionRegistration(this);
		}
	}
}

void ADungeonMainLevelScriptActor::ApplyCurrentPartitionActivationState()
{
	if (!IsPartitionLoadControlAvailable())
	{
		mLastEnableLoadControl = false;
		ForceActivate();
		ForceActivateShadowCastingPointAndSpotLights();
		return;
	}

	if (UWorld* world = GetWorld())
	{
		Begin();
		for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
		{
			const auto* playerController = iterator->Get();
			if (!IsValid(playerController))
				continue;

			const auto& playerPawn = playerController->GetPawn();
			if (IsValid(playerPawn) && playerPawn->IsPlayerControlled())
				Mark(playerPawn->GetActorLocation());
		}
		End(0.f);

		if (MaxShadowCastingPointAndSpotLights > 0)
			UpdateShadowCastingPointAndSpotLights();
	}
	else
	{
		ForceActivate();
		ForceActivateShadowCastingPointAndSpotLights();
	}
}

void ADungeonMainLevelScriptActor::ResetPartitionTransitionQueue()
{
	mPendingPartitionTransitions.Reset();
	mPendingPartitionTransitionReadIndex = 0;
	mDesiredPartitionActivation.Reset(DungeonPartitions.Num());
	mQueuedPartitionTransitions.Reset(DungeonPartitions.Num());

	for (UDungeonPartition* partition : DungeonPartitions)
	{
		const bool isActive = IsValid(partition) && partition->IsPartitionActivate();
		mDesiredPartitionActivation.Add(isActive ? 1 : 0);
		mQueuedPartitionTransitions.Add(0);
	}
}

void ADungeonMainLevelScriptActor::EnqueuePartitionTransition(const int32 partitionIndex)
{
	if (!DungeonPartitions.IsValidIndex(partitionIndex))
		return;
	if (!mQueuedPartitionTransitions.IsValidIndex(partitionIndex))
		return;
	if (mQueuedPartitionTransitions[partitionIndex] != 0)
		return;

	mPendingPartitionTransitions.Add(partitionIndex);
	mQueuedPartitionTransitions[partitionIndex] = 1;
}

void ADungeonMainLevelScriptActor::ProcessPartitionTransitionQueue()
{
	if (mDesiredPartitionActivation.Num() != DungeonPartitions.Num() || mQueuedPartitionTransitions.Num() != DungeonPartitions.Num())
	{
		ResetPartitionTransitionQueue();
		return;
	}

	int32 remainingActivations = MaxPartitionActivationsPerFrame > 0 ? MaxPartitionActivationsPerFrame : TNumericLimits<int32>::Max();
	int32 remainingInactivations = MaxPartitionInactivationsPerFrame > 0 ? MaxPartitionInactivationsPerFrame : TNumericLimits<int32>::Max();
	const int32 pendingCount = mPendingPartitionTransitions.Num() - mPendingPartitionTransitionReadIndex;
	for (int32 processedCount = 0; processedCount < pendingCount; ++processedCount)
	{
		if (!mPendingPartitionTransitions.IsValidIndex(mPendingPartitionTransitionReadIndex))
			break;

		const int32 partitionIndex = mPendingPartitionTransitions[mPendingPartitionTransitionReadIndex++];
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !mQueuedPartitionTransitions.IsValidIndex(partitionIndex))
			continue;

		UDungeonPartition* partition = DungeonPartitions[partitionIndex];
		if (!IsValid(partition))
		{
			mQueuedPartitionTransitions[partitionIndex] = 0;
			continue;
		}

		const bool desiredActive = mDesiredPartitionActivation[partitionIndex] != 0;
		const bool currentActive = partition->IsPartitionActivate();
		if (desiredActive == currentActive)
		{
			mQueuedPartitionTransitions[partitionIndex] = 0;
			partition->FlushRegisteredComponents();
			continue;
		}

		if (desiredActive)
		{
			if (remainingActivations <= 0)
			{
				mPendingPartitionTransitions.Add(partitionIndex);
				continue;
			}

			partition->CallPartitionActivate(false);
			--remainingActivations;
		}
		else
		{
			if (remainingInactivations <= 0)
			{
				mPendingPartitionTransitions.Add(partitionIndex);
				continue;
			}

			partition->CallPartitionInactivate();
			--remainingInactivations;
		}

		mQueuedPartitionTransitions[partitionIndex] = 0;
	}

	if (mPendingPartitionTransitionReadIndex >= mPendingPartitionTransitions.Num())
	{
		mPendingPartitionTransitions.Reset();
		mPendingPartitionTransitionReadIndex = 0;
	}
	else if (mPendingPartitionTransitionReadIndex > 0 && mPendingPartitionTransitionReadIndex * 2 >= mPendingPartitionTransitions.Num())
	{
#if UE_VERSION_NEWER_THAN(5, 6, 0)
		mPendingPartitionTransitions.RemoveAt(0, mPendingPartitionTransitionReadIndex, EAllowShrinking::No);
#else
		mPendingPartitionTransitions.RemoveAt(0, mPendingPartitionTransitionReadIndex);
#endif
		mPendingPartitionTransitionReadIndex = 0;
	}
}

void ADungeonMainLevelScriptActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	DungeonPartitions.Reset();
	mPartitionIndexByCell.Reset();
	ResetPrecomputedPartitionVisibility();
	mBounding.Init();
	ResetPartitionTransitionQueue();
}

void ADungeonMainLevelScriptActor::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (mBounding.IsValid == false)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (IsPartitionLoadControlAvailable())
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
		if (mLastEnableLoadControl)
		{
			mLastEnableLoadControl = false;
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
 */
UDungeonPartition* ADungeonMainLevelScriptActor::Find(const FVector& worldLocation) const noexcept
{
	const int32 partitionIndex = FindPartitionIndex(worldLocation);
	if (!DungeonPartitions.IsValidIndex(partitionIndex))
		return nullptr;
	return DungeonPartitions[partitionIndex];
}

/*
 * 点で検索しているので、巨大なアクターは誤判定に注意してください。
 */
int32 ADungeonMainLevelScriptActor::FindPartitionIndex(const FVector& worldLocation) const noexcept
{
	if (!mBounding.IsValid || DungeonPartitions.IsEmpty())
		return INDEX_NONE;
	if (!mBounding.IsInsideOrOn(worldLocation))
		return INDEX_NONE;

	const FIntVector partitionCell = ToPartitionCell(worldLocation);
	if (const int32* partitionIndex = mPartitionIndexByCell.Find(partitionCell))
		return *partitionIndex;

	return FindNearestPartitionIndex(partitionCell, worldLocation);
}

/*
 * 点で検索しているので、巨大なアクターは誤判定に注意してください。
 */
FIntVector ADungeonMainLevelScriptActor::ToPartitionCell(const FVector& worldLocation) const noexcept
{
	if (mPartitionWorldSize.X <= 0.0f || mPartitionWorldSize.Y <= 0.0f || mPartitionWorldSize.Z <= 0.0f)
		return FIntVector::ZeroValue;

	const FVector localLocation(
		(worldLocation.X - mBounding.Min.X) / mPartitionWorldSize.X,
		(worldLocation.Y - mBounding.Min.Y) / mPartitionWorldSize.Y,
		(worldLocation.Z - mBounding.Min.Z) / mPartitionWorldSize.Z
	);
	return FIntVector(
		FMath::FloorToInt(localLocation.X),
		FMath::FloorToInt(localLocation.Y),
		FMath::FloorToInt(localLocation.Z)
	);
}

FBox ADungeonMainLevelScriptActor::MakePartitionBounds(const FIntVector& partitionCell) const noexcept
{
	const FVector min = mBounding.Min + FVector(
		static_cast<float>(partitionCell.X) * mPartitionWorldSize.X,
		static_cast<float>(partitionCell.Y) * mPartitionWorldSize.Y,
		static_cast<float>(partitionCell.Z) * mPartitionWorldSize.Z
	);
	return FBox(min, min + mPartitionWorldSize);
}

int32 ADungeonMainLevelScriptActor::FindNearestPartitionIndex(const FIntVector& partitionCell, const FVector& worldLocation) const noexcept
{
	int32 result = INDEX_NONE;
	double minimumDistance = TNumericLimits<double>::Max();
	for (int32 z = partitionCell.Z - SparsePartitionFallbackSearchRadius; z <= partitionCell.Z + SparsePartitionFallbackSearchRadius; ++z)
	{
		for (int32 y = partitionCell.Y - SparsePartitionFallbackSearchRadius; y <= partitionCell.Y + SparsePartitionFallbackSearchRadius; ++y)
		{
			for (int32 x = partitionCell.X - SparsePartitionFallbackSearchRadius; x <= partitionCell.X + SparsePartitionFallbackSearchRadius; ++x)
			{
				const int32* partitionIndex = mPartitionIndexByCell.Find(FIntVector(x, y, z));
				if (partitionIndex == nullptr || !DungeonPartitions.IsValidIndex(*partitionIndex))
					continue;

				const double distance = ComputeSquaredDistanceToBounds(DungeonPartitions[*partitionIndex]->GetBounds(), worldLocation);
				if (distance < minimumDistance)
				{
					minimumDistance = distance;
					result = *partitionIndex;
				}
			}
		}
	}

	if (result != INDEX_NONE)
		return result;

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		const UDungeonPartition* partition = DungeonPartitions[partitionIndex];
		if (!IsValid(partition))
			continue;

		const double distance = ComputeSquaredDistanceToBounds(partition->GetBounds(), worldLocation);
		if (distance < minimumDistance)
		{
			minimumDistance = distance;
			result = partitionIndex;
		}
	}

	return result;
}

int32 ADungeonMainLevelScriptActor::FindOrAddPartition(const FIntVector& partitionCell)
{
	if (const int32* partitionIndex = mPartitionIndexByCell.Find(partitionCell))
		return *partitionIndex;

	UDungeonPartition* partition = NewObject<UDungeonPartition>(this);
	check(IsValid(partition));
	partition->ResetGraphData();
	partition->SetCellCoordinate(partitionCell);
	partition->SetBounds(MakePartitionBounds(partitionCell));

	const int32 partitionIndex = DungeonPartitions.Add(partition);
	mPartitionIndexByCell.Add(partitionCell, partitionIndex);
	return partitionIndex;
}

void ADungeonMainLevelScriptActor::LinkPartitions(const int32 partitionIndex0, const int32 partitionIndex1)
{
	if (partitionIndex0 == partitionIndex1)
		return;
	if (!DungeonPartitions.IsValidIndex(partitionIndex0) || !DungeonPartitions.IsValidIndex(partitionIndex1))
		return;

	DungeonPartitions[partitionIndex0]->AddNeighborIndex(partitionIndex1);
	DungeonPartitions[partitionIndex1]->AddNeighborIndex(partitionIndex0);
}

void ADungeonMainLevelScriptActor::BuildSparsePartitionGraph(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors)
{
	DungeonPartitions.Reset();
	mPartitionIndexByCell.Reset();

	for (ADungeonGenerateActor* dungeonGenerateActor : dungeonGenerateActors)
	{
		if (!IsValid(dungeonGenerateActor) || !IsValid(dungeonGenerateActor->mParameter))
			continue;

		const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
		if (generator == nullptr)
			continue;

		const auto& voxel = generator->GetVoxel();
		if (voxel == nullptr)
			continue;

		const FVector gridHalfSize = dungeonGenerateActor->mParameter->GetGridSize().To3D() * 0.5f;
		voxel->Each([this, voxel, dungeonGenerateActor, gridHalfSize](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (!IsTraversableGrid(grid))
					return true;

				const FVector worldLocation = dungeonGenerateActor->mParameter->ToWorld(location) + dungeonGenerateActor->GetActorLocation() + gridHalfSize;
				const int32 partitionIndex = FindOrAddPartition(ToPartitionCell(worldLocation));
				for (const FIntVector& neighborOffset : NeighborOffsets)
				{
					const FIntVector neighborLocation = location + neighborOffset;
					if (!voxel->Contain(neighborLocation))
						continue;

					const dungeon::Grid& neighborGrid = voxel->Get(neighborLocation);
					if (!IsTraversableGrid(neighborGrid))
						continue;

					const FVector neighborWorldLocation = dungeonGenerateActor->mParameter->ToWorld(neighborLocation) + dungeonGenerateActor->GetActorLocation() + gridHalfSize;
					const int32 neighborPartitionIndex = FindOrAddPartition(ToPartitionCell(neighborWorldLocation));
					LinkPartitions(partitionIndex, neighborPartitionIndex);
				}

				return true;
			}
		);
	}
}

void ADungeonMainLevelScriptActor::ResetPrecomputedPartitionVisibility()
{
	mPartitionVisibilitySamples.Reset();
	mPartitionPotentialVisibilityMasks.Reset();
	mPartitionConnectedComponents.Reset();
}

void ADungeonMainLevelScriptActor::BuildPartitionVisibilitySamples(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors)
{
	mPartitionVisibilitySamples.Reset();
	mPartitionVisibilitySamples.SetNum(DungeonPartitions.Num());
	if (DungeonPartitions.IsEmpty())
		return;

	struct FSampleSlot
	{
		bool bValid = false;
		double Metric = 0.0;
		FPartitionVisibilitySample Sample;
	};

	struct FPartitionVisibilityAccumulator
	{
		FSampleSlot Center;
		FSampleSlot MinX;
		FSampleSlot MaxX;
		FSampleSlot MinY;
		FSampleSlot MaxY;
		FSampleSlot MinZ;
		FSampleSlot MaxZ;
	};

	TArray<FPartitionVisibilityAccumulator> accumulators;
	accumulators.SetNum(DungeonPartitions.Num());

	auto assignSample = [](FSampleSlot& slot, const FPartitionVisibilitySample& sample, const double metric, const bool preferLower)
	{
		if (!slot.bValid || (preferLower ? metric < slot.Metric : metric > slot.Metric))
		{
			slot.bValid = true;
			slot.Metric = metric;
			slot.Sample = sample;
		}
	};

	for (ADungeonGenerateActor* dungeonGenerateActor : dungeonGenerateActors)
	{
		if (!IsValid(dungeonGenerateActor) || !IsValid(dungeonGenerateActor->mParameter))
			continue;

		const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
		if (generator == nullptr)
			continue;

		const auto& voxel = generator->GetVoxel();
		if (voxel == nullptr)
			continue;

		const FVector gridHalfSize = dungeonGenerateActor->mParameter->GetGridSize().To3D() * 0.5f;
		voxel->Each([this, &accumulators, dungeonGenerateActor, gridHalfSize, &assignSample](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (!IsTraversableGrid(grid))
					return true;

				const FVector worldLocation = dungeonGenerateActor->mParameter->ToWorld(location) + dungeonGenerateActor->GetActorLocation() + gridHalfSize;
				const int32 partitionIndex = FindPartitionIndex(worldLocation);
				if (!DungeonPartitions.IsValidIndex(partitionIndex))
					return true;

				FPartitionVisibilitySample sample;
				sample.DungeonGenerateActor = dungeonGenerateActor;
				sample.GridLocation = location;
				sample.WorldLocation = worldLocation;

				const FBox& bounds = DungeonPartitions[partitionIndex]->GetBounds();
				const FVector center = bounds.GetCenter();
				FPartitionVisibilityAccumulator& accumulator = accumulators[partitionIndex];
				assignSample(accumulator.Center, sample, FVector::DistSquared(center, worldLocation), true);
				assignSample(accumulator.MinX, sample, worldLocation.X, true);
				assignSample(accumulator.MaxX, sample, worldLocation.X, false);
				assignSample(accumulator.MinY, sample, worldLocation.Y, true);
				assignSample(accumulator.MaxY, sample, worldLocation.Y, false);
				assignSample(accumulator.MinZ, sample, worldLocation.Z, true);
				assignSample(accumulator.MaxZ, sample, worldLocation.Z, false);
				return true;
			}
		);
	}

	auto addUniqueSample = [](TArray<FPartitionVisibilitySample>& destination, const FPartitionVisibilitySample& sample)
	{
		if (!IsValid(sample.DungeonGenerateActor))
			return;

		for (const FPartitionVisibilitySample& existing : destination)
		{
			if (existing.DungeonGenerateActor == sample.DungeonGenerateActor && existing.GridLocation == sample.GridLocation)
				return;
		}

		destination.Add(sample);
	};

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		TArray<FPartitionVisibilitySample>& samples = mPartitionVisibilitySamples[partitionIndex];
		const FPartitionVisibilityAccumulator& accumulator = accumulators[partitionIndex];
		if (accumulator.Center.bValid)
			addUniqueSample(samples, accumulator.Center.Sample);
		if (accumulator.MinX.bValid)
			addUniqueSample(samples, accumulator.MinX.Sample);
		if (accumulator.MaxX.bValid)
			addUniqueSample(samples, accumulator.MaxX.Sample);
		if (accumulator.MinY.bValid)
			addUniqueSample(samples, accumulator.MinY.Sample);
		if (accumulator.MaxY.bValid)
			addUniqueSample(samples, accumulator.MaxY.Sample);
		if (accumulator.MinZ.bValid)
			addUniqueSample(samples, accumulator.MinZ.Sample);
		if (accumulator.MaxZ.bValid)
			addUniqueSample(samples, accumulator.MaxZ.Sample);
	}
}

void ADungeonMainLevelScriptActor::BuildPartitionConnectedComponents()
{
	mPartitionConnectedComponents.Init(INDEX_NONE, DungeonPartitions.Num());
	int32 componentIndex = 0;
	TArray<int32> frontier;

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !IsValid(DungeonPartitions[partitionIndex]))
			continue;
		if (mPartitionConnectedComponents[partitionIndex] != INDEX_NONE)
			continue;

		frontier.Reset();
		frontier.Add(partitionIndex);
		mPartitionConnectedComponents[partitionIndex] = componentIndex;

		for (int32 readIndex = 0; readIndex < frontier.Num(); ++readIndex)
		{
			const int32 currentIndex = frontier[readIndex];
			if (!DungeonPartitions.IsValidIndex(currentIndex) || !IsValid(DungeonPartitions[currentIndex]))
				continue;

			for (const int32 neighborIndex : DungeonPartitions[currentIndex]->GetNeighborIndices())
			{
				if (!DungeonPartitions.IsValidIndex(neighborIndex) || !IsValid(DungeonPartitions[neighborIndex]))
					continue;
				if (mPartitionConnectedComponents[neighborIndex] != INDEX_NONE)
					continue;

				mPartitionConnectedComponents[neighborIndex] = componentIndex;
				frontier.Add(neighborIndex);
			}
		}

		++componentIndex;
	}
}

void ADungeonMainLevelScriptActor::BuildPrecomputedPartitionVisibility()
{
	mPartitionPotentialVisibilityMasks.Reset();
	if (DungeonPartitions.IsEmpty())
		return;

	const int32 partitionCount = DungeonPartitions.Num();
	const int32 wordCount = (partitionCount + 63) / 64;
	mPartitionPotentialVisibilityMasks.SetNum(partitionCount);
	for (TArray<uint64>& row : mPartitionPotentialVisibilityMasks)
	{
		row.Init(0ull, wordCount);
	}

	if (mPartitionVisibilitySamples.Num() != partitionCount || mPartitionConnectedComponents.Num() != partitionCount)
		return;

	const double maximumVisibilityDistanceSquared =
		mTheoreticalMaxVisibilityDistance > 0.f ?
		FMath::Square(static_cast<double>(mTheoreticalMaxVisibilityDistance)) :
		TNumericLimits<double>::Max();
	const int32 visibilityDilationHopCount = FMath::Max(0, PrecomputedVisibilityDilationHopCount);

	ParallelFor(partitionCount, [this, partitionCount, maximumVisibilityDistanceSquared, visibilityDilationHopCount](const int32 sourcePartitionIndex)
		{
			if (!DungeonPartitions.IsValidIndex(sourcePartitionIndex) || !IsValid(DungeonPartitions[sourcePartitionIndex]))
				return;

			TArray<uint64>& row = mPartitionPotentialVisibilityMasks[sourcePartitionIndex];
			const UDungeonPartition* sourcePartition = DungeonPartitions[sourcePartitionIndex];
			auto setVisible = [&row](const int32 partitionIndex)
			{
				row[partitionIndex / 64] |= (1ull << (partitionIndex % 64));
			};
			auto expandVisibleNeighbors = [this, sourcePartitionIndex, visibilityDilationHopCount, &setVisible](const int32 centerPartitionIndex)
			{
				if (visibilityDilationHopCount <= 0)
					return;

				TArray<int32, TInlineAllocator<16>> frontier;
				frontier.Add(centerPartitionIndex);
				TSet<int32> visited;
				visited.Reserve(16);
				visited.Add(centerPartitionIndex);

				for (int32 hop = 0; hop < visibilityDilationHopCount && frontier.IsEmpty() == false; ++hop)
				{
					TArray<int32, TInlineAllocator<16>> nextFrontier;
					for (const int32 currentPartitionIndex : frontier)
					{
						if (!DungeonPartitions.IsValidIndex(currentPartitionIndex) || !IsValid(DungeonPartitions[currentPartitionIndex]))
							continue;

						for (const int32 neighborIndex : DungeonPartitions[currentPartitionIndex]->GetNeighborIndices())
						{
							if (!DungeonPartitions.IsValidIndex(neighborIndex) || !IsValid(DungeonPartitions[neighborIndex]))
								continue;
							if (mPartitionConnectedComponents[sourcePartitionIndex] != mPartitionConnectedComponents[neighborIndex])
								continue;
							if (visited.Contains(neighborIndex))
								continue;

							visited.Add(neighborIndex);
							setVisible(neighborIndex);
							nextFrontier.Add(neighborIndex);
						}
					}

					frontier = MoveTemp(nextFrontier);
				}
			};
			auto setVisibleWithDilation = [&setVisible, &expandVisibleNeighbors](const int32 partitionIndex)
			{
				setVisible(partitionIndex);
				expandVisibleNeighbors(partitionIndex);
			};

			setVisibleWithDilation(sourcePartitionIndex);

			for (int32 targetPartitionIndex = 0; targetPartitionIndex < partitionCount; ++targetPartitionIndex)
			{
				if (targetPartitionIndex == sourcePartitionIndex)
					continue;
				if (!DungeonPartitions.IsValidIndex(targetPartitionIndex) || !IsValid(DungeonPartitions[targetPartitionIndex]))
					continue;
				if (mPartitionConnectedComponents[sourcePartitionIndex] != mPartitionConnectedComponents[targetPartitionIndex])
					continue;

				const UDungeonPartition* targetPartition = DungeonPartitions[targetPartitionIndex];
				if (ComputeSquaredDistanceBetweenBounds(sourcePartition->GetBounds(), targetPartition->GetBounds()) > maximumVisibilityDistanceSquared)
					continue;

				if (sourcePartition->GetNeighborIndices().Contains(targetPartitionIndex) || IsPartitionPairPotentiallyVisible(sourcePartitionIndex, targetPartitionIndex))
					setVisibleWithDilation(targetPartitionIndex);
			}
		},
		EParallelForFlags::None
	);
}

bool ADungeonMainLevelScriptActor::HasPrecomputedPartitionVisibility() const noexcept
{
	return
		mPartitionVisibilitySamples.Num() == DungeonPartitions.Num() &&
		mPartitionPotentialVisibilityMasks.Num() == DungeonPartitions.Num() &&
		mPartitionConnectedComponents.Num() == DungeonPartitions.Num() &&
		DungeonPartitions.IsEmpty() == false;
}

bool ADungeonMainLevelScriptActor::IsPartitionLoadControlAvailable() const noexcept
{
	return bEnableLoadControl && bUsePrecomputedPartitionVisibility && HasPrecomputedPartitionVisibility();
}

void ADungeonMainLevelScriptActor::MarkPrecomputedPartitionVisibility(const int32 sourcePartitionIndex) const
{
	if (!HasPrecomputedPartitionVisibility())
		return;
	if (!DungeonPartitions.IsValidIndex(sourcePartitionIndex))
		return;

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		if (!IsPrecomputedPartitionVisible(sourcePartitionIndex, partitionIndex))
			continue;
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !IsValid(DungeonPartitions[partitionIndex]))
			continue;

		DungeonPartitions[partitionIndex]->Mark();
	}
}

bool ADungeonMainLevelScriptActor::IsPrecomputedPartitionVisible(const int32 sourcePartitionIndex, const int32 targetPartitionIndex) const noexcept
{
	if (!mPartitionPotentialVisibilityMasks.IsValidIndex(sourcePartitionIndex))
		return false;
	if (!DungeonPartitions.IsValidIndex(targetPartitionIndex))
		return false;

	const TArray<uint64>& row = mPartitionPotentialVisibilityMasks[sourcePartitionIndex];
	const int32 wordIndex = targetPartitionIndex / 64;
	if (!row.IsValidIndex(wordIndex))
		return false;

	return (row[wordIndex] & (1ull << (targetPartitionIndex % 64))) != 0;
}

bool ADungeonMainLevelScriptActor::IsPartitionPairPotentiallyVisible(const int32 sourcePartitionIndex, const int32 targetPartitionIndex) const
{
	if (!mPartitionVisibilitySamples.IsValidIndex(sourcePartitionIndex) || !mPartitionVisibilitySamples.IsValidIndex(targetPartitionIndex))
		return false;

	const TArray<FPartitionVisibilitySample>& sourceSamples = mPartitionVisibilitySamples[sourcePartitionIndex];
	const TArray<FPartitionVisibilitySample>& targetSamples = mPartitionVisibilitySamples[targetPartitionIndex];
	if (sourceSamples.IsEmpty() || targetSamples.IsEmpty())
		return false;

	for (const FPartitionVisibilitySample& sourceSample : sourceSamples)
	{
		for (const FPartitionVisibilitySample& targetSample : targetSamples)
		{
			if (sourceSample.DungeonGenerateActor != targetSample.DungeonGenerateActor)
				continue;
			if (TracePartitionVisibility(sourceSample, targetSample))
				return true;
		}
	}

	return false;
}

bool ADungeonMainLevelScriptActor::TracePartitionVisibility(const FPartitionVisibilitySample& sourceSample, const FPartitionVisibilitySample& targetSample)
{
	const ADungeonGenerateActor* dungeonGenerateActor = sourceSample.DungeonGenerateActor;
	if (!IsValid(dungeonGenerateActor))
		return false;
	if (dungeonGenerateActor != targetSample.DungeonGenerateActor)
		return false;
	if (!IsValid(dungeonGenerateActor->mParameter))
		return false;

	const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
	if (generator == nullptr)
		return false;

	const auto& voxel = generator->GetVoxel();
	if (voxel == nullptr)
		return false;
	if (!voxel->Contain(sourceSample.GridLocation) || !voxel->Contain(targetSample.GridLocation))
		return false;

	auto isTransitionOpen = [voxel](const FIntVector& fromCell, const FIntVector& toCell)
	{
		if (!voxel->Contain(fromCell) || !voxel->Contain(toCell))
			return false;

		const dungeon::Grid& fromGrid = voxel->Get(fromCell);
		const dungeon::Grid& toGrid = voxel->Get(toCell);
		if (!IsTraversableGrid(fromGrid) || !IsTraversableGrid(toGrid))
			return false;

		const FIntVector delta = toCell - fromCell;
		if (delta == FIntVector(1, 0, 0))
			return !fromGrid.HasEastWall() && !toGrid.HasWestWall();
		if (delta == FIntVector(-1, 0, 0))
			return !fromGrid.HasWestWall() && !toGrid.HasEastWall();
		if (delta == FIntVector(0, 1, 0))
			return !fromGrid.HasSouthWall() && !toGrid.HasNorthWall();
		if (delta == FIntVector(0, -1, 0))
			return !fromGrid.HasNorthWall() && !toGrid.HasSouthWall();
		if (delta == FIntVector(0, 0, 1))
			return !fromGrid.HasCeiling() && !toGrid.HasFloor();
		if (delta == FIntVector(0, 0, -1))
			return !fromGrid.HasFloor() && !toGrid.HasCeiling();

		return false;
	};

	auto canTraverseDelta = [&isTransitionOpen](const FIntVector& startCell, const FIntVector& targetCell)
	{
		const FIntVector delta = targetCell - startCell;
		if (FMath::Abs(delta.X) > 1 || FMath::Abs(delta.Y) > 1 || FMath::Abs(delta.Z) > 1)
			return false;

		const int32 manhattanDistance = FMath::Abs(delta.X) + FMath::Abs(delta.Y) + FMath::Abs(delta.Z);
		if (manhattanDistance == 0)
			return true;
		if (manhattanDistance == 1)
			return isTransitionOpen(startCell, targetCell);

		std::array<FIntVector, 3> axisSteps = {
			FIntVector::ZeroValue,
			FIntVector::ZeroValue,
			FIntVector::ZeroValue
		};
		int32 axisCount = 0;
		if (delta.X != 0)
			axisSteps[axisCount++] = FIntVector(delta.X > 0 ? 1 : -1, 0, 0);
		if (delta.Y != 0)
			axisSteps[axisCount++] = FIntVector(0, delta.Y > 0 ? 1 : -1, 0);
		if (delta.Z != 0)
			axisSteps[axisCount++] = FIntVector(0, 0, delta.Z > 0 ? 1 : -1);

		const uint8 completeMask = static_cast<uint8>((1u << axisCount) - 1u);
		std::function<bool(const FIntVector&, uint8)> traverse = [&](const FIntVector& currentCell, const uint8 usedMask)
		{
			if (usedMask == completeMask)
				return true;

			for (int32 axisIndex = 0; axisIndex < axisCount; ++axisIndex)
			{
				const uint8 axisMask = static_cast<uint8>(1u << axisIndex);
				if ((usedMask & axisMask) != 0)
					continue;

				const FIntVector nextCell = currentCell + axisSteps[axisIndex];
				if (!isTransitionOpen(currentCell, nextCell))
					continue;
				if (traverse(nextCell, static_cast<uint8>(usedMask | axisMask)))
					return true;
			}

			return false;
		};

		return traverse(startCell, 0u);
	};

	const FVector gridSize = dungeonGenerateActor->mParameter->GetGridSize().To3D();
	const double minimumGridSize = FMath::Max(1.0, static_cast<double>(FMath::Min3(gridSize.X, gridSize.Y, gridSize.Z)));
	const FVector deltaWorld = targetSample.WorldLocation - sourceSample.WorldLocation;
	const double distance = deltaWorld.Size();
	if (distance <= KINDA_SMALL_NUMBER)
		return true;

	const double stepDistance = FMath::Max(minimumGridSize * 0.25, 1.0);
	const int32 stepCount = FMath::Max(1, FMath::CeilToInt(distance / stepDistance));
	const FVector actorLocation = dungeonGenerateActor->GetActorLocation();

	FIntVector currentCell = sourceSample.GridLocation;
	for (int32 stepIndex = 1; stepIndex <= stepCount; ++stepIndex)
	{
		const float alpha = static_cast<float>(stepIndex) / static_cast<float>(stepCount);
		const FVector worldLocation = FMath::Lerp(sourceSample.WorldLocation, targetSample.WorldLocation, alpha);
		const FIntVector nextCell = dungeonGenerateActor->mParameter->ToGrid(worldLocation - actorLocation);
		if (!voxel->Contain(nextCell))
			return false;
		if (!canTraverseDelta(currentCell, nextCell))
			return false;

		currentCell = nextCell;
	}

	return canTraverseDelta(currentCell, targetSample.GridLocation);
}

#if 1
FInt32Interval ADungeonMainLevelScriptActor::ComputeTerrainCullingDistanceRange() const noexcept
{
	const int32 theoreticalMaximumVisibilityDistance = FMath::Max(0, FMath::CeilToInt(mTheoreticalMaxVisibilityDistance));
	if (theoreticalMaximumVisibilityDistance <= 0)
		return FInt32Interval(0, 0);

	const int32 fadeBandDistance = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(theoreticalMaximumVisibilityDistance) * 0.08f),
		200,
		1200
	);
	int32 cullingStartDistance = FMath::Max(0, theoreticalMaximumVisibilityDistance - fadeBandDistance);
	if (cullingStartDistance >= theoreticalMaximumVisibilityDistance)
		cullingStartDistance = FMath::Max(0, theoreticalMaximumVisibilityDistance - 200);

	return FInt32Interval(cullingStartDistance, theoreticalMaximumVisibilityDistance);
}
#else
namespace
{
	double ComputeSquaredMaximumDistanceBetweenBounds(const FBox& left, const FBox& right) noexcept
	{
		const double dx = FMath::Max(
			FMath::Abs(left.Min.X - right.Max.X),
			FMath::Abs(left.Max.X - right.Min.X)
		);
		const double dy = FMath::Max(
			FMath::Abs(left.Min.Y - right.Max.Y),
			FMath::Abs(left.Max.Y - right.Min.Y)
		);
		const double dz = FMath::Max(
			FMath::Abs(left.Min.Z - right.Max.Z),
			FMath::Abs(left.Max.Z - right.Min.Z)
		);
		return dx * dx + dy * dy + dz * dz;
	}
}

FInt32Interval ADungeonMainLevelScriptActor::ComputeTerrainCullingDistanceRange() const noexcept
{
	const int32 theoreticalMax = FMath::Max(0, FMath::CeilToInt(mTheoreticalMaxVisibilityDistance));
	if (theoreticalMax <= 0)
		return FInt32Interval(0, 0);

	int32 maxVisibleDistance = theoreticalMax;

	if (HasPrecomputedPartitionVisibility())
	{
		double maxVisibleDistanceSquared = 0.0;

		for (int32 sourceIndex = 0; sourceIndex < DungeonPartitions.Num(); ++sourceIndex)
		{
			const UDungeonPartition* sourcePartition = DungeonPartitions[sourceIndex];
			if (!IsValid(sourcePartition))
				continue;

			for (int32 targetIndex = 0; targetIndex < DungeonPartitions.Num(); ++targetIndex)
			{
				if (!IsPrecomputedPartitionVisible(sourceIndex, targetIndex))
					continue;

				const UDungeonPartition* targetPartition = DungeonPartitions[targetIndex];
				if (!IsValid(targetPartition))
					continue;

				maxVisibleDistanceSquared = FMath::Max(
					maxVisibleDistanceSquared,
					ComputeSquaredMaximumDistanceBetweenBounds(sourcePartition->GetBounds(), targetPartition->GetBounds())
				);
			}
		}

		maxVisibleDistance = FMath::Min(
			theoreticalMax,
			FMath::CeilToInt(FMath::Sqrt(maxVisibleDistanceSquared))
		);
	}

	const int32 fadeBandDistance = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(maxVisibleDistance) * 0.08f),
		200,
		1200
	);

	const int32 cullingStartDistance = FMath::Max(0, maxVisibleDistance - fadeBandDistance);
	return FInt32Interval(cullingStartDistance, maxVisibleDistance);
}
#endif

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
	if (!IsPartitionLoadControlAvailable())
		return;

	// mBounding.Minを原点にした座標系に変換
	const int32 startPartitionIndex = FindPartitionIndex(playerLocation);
	if (!DungeonPartitions.IsValidIndex(startPartitionIndex))
		return;

	MarkPrecomputedPartitionVisibility(startPartitionIndex);
}

void ADungeonMainLevelScriptActor::End(const float deltaSeconds)
{
	if (mDesiredPartitionActivation.Num() != DungeonPartitions.Num() || mQueuedPartitionTransitions.Num() != DungeonPartitions.Num())
	{
		ResetPartitionTransitionQueue();
	}

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		UDungeonPartition* partition = DungeonPartitions[partitionIndex];
		check(IsValid(partition));

		partition->FlushRegisteredComponents();

		bool desiredActive;
		if (partition->IsMarked())
		{
			partition->ResetPartitionInactivateRemainTimer();
			desiredActive = true;
		}
		else
		{
			desiredActive = partition->UpdatePartitionInactivateRemainTimer(deltaSeconds);
		}

		mDesiredPartitionActivation[partitionIndex] = desiredActive ? 1 : 0;
		if (desiredActive != partition->IsPartitionActivate())
			EnqueuePartitionTransition(partitionIndex);
	}

	ProcessPartitionTransitionQueue();
}

void ADungeonMainLevelScriptActor::ForceActivate()
{
	ResetPartitionTransitionQueue();
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->CallPartitionActivate(true);
	}
	ResetPartitionTransitionQueue();
}

void ADungeonMainLevelScriptActor::ForceInactivate()
{
	ResetPartitionTransitionQueue();
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->CallPartitionInactivate();
	}
	ResetPartitionTransitionQueue();
}

bool ADungeonMainLevelScriptActor::IsEnableLoadControl() const noexcept
{
	return IsPartitionLoadControlAvailable();
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

void ADungeonMainLevelScriptActor::DrawDebugInformation() const
{
	// パーティエーションの状態を線で描画します
	constexpr double Margin = 10;
	for (const UDungeonPartition* partition : DungeonPartitions)
	{
		if (!IsValid(partition))
			continue;

		const FBox& bounds = partition->GetBounds();
		const FVector halfSize = bounds.GetExtent() - FVector(Margin);
		const auto color = partition->IsMarked() ? FColor::Red : FColor::Blue;
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounds.GetCenter(),
			halfSize,
			color,
			FRotator::ZeroRotator,
			0.f,
			10.f
		);
	}

	// 有効な範囲を黄色い線で描画します
}
#endif
