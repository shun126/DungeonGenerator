/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "MainLevel/DungeonLightStatePolicy.h"
#include "MainLevel/DungeonPartition.h"
#include "DungeonGenerateActor.h"
#include "Core/Generator.h"
#include "Core/Debug/Debug.h"
#include "Core/Debug/MeasureTime.h"
#include "Core/Voxelization/Grid.h"
#include "Core/Voxelization/Voxel.h"
#include "Parameter/DungeonGenerateParameter.h"
#include "SubActor/DungeonPointLightComponent.h"
#include "SubActor/DungeonSpotLightComponent.h"
#include "../Core/Debug/BuildInformation.h"
#include <DrawDebugHelpers.h>
#include <Async/ParallelFor.h>
#include <Components/PointLightComponent.h>
#include <Containers/Set.h>
#if WITH_EDITOR
#include <Debug/DebugDrawService.h>
#include <Engine/Canvas.h>
#include <Engine/Engine.h>
#include <SceneInterface.h>
#include <SceneView.h>
#endif
#include <Engine/Level.h>
#include <Engine/World.h>
#include <EngineUtils.h>
#include <GameFramework/Pawn.h>
#include <HAL/IConsoleManager.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/EngineVersionComparison.h>
#include <array>
#include <algorithm>
#include <functional>
#include <limits>

namespace
{
#if WITH_EDITOR
	TAutoConsoleVariable<int32> CVarValidatePVSOptimization(
		TEXT("DungeonGenerator.PVS.ValidateOptimization"),
		0,
		TEXT("Compares optimized partition visibility tests with the reference implementation when non-zero."),
		ECVF_Default
	);

	namespace DungeonDebugColors
	{
		const FColor RuntimeActive = FColor::Green;
		const FColor RuntimePendingInactivation = FColor::Yellow;
		const FColor RuntimePendingActivation = FColor::Orange;
		const FColor RuntimeInactive = FColor::Blue;
		const FColor PlayerPartition = FColor::White;
		const FColor PlayerInsideBoundsWithoutPartition = FColor::Magenta;
		const FColor PlayerOutsideBounds = FColor::Yellow;
		const FColor ConstructionBounds = FColor::Yellow;
		const FColor ConstructionCell = FColor(96, 96, 255);
		const FColor PVSVisiblePartition = FColor::Cyan;
		const FColor SampleCenter = FColor::White;
		const FColor SampleEdge = FColor::Cyan;
		const FColor SampleRoofVisible = FColor(180, 64, 255);
		const FColor SampleRoofCandidate = FColor(72, 24, 96);
		const FColor SampleSlopeLower = FColor::Green;
		const FColor SampleSlopeUpper = FColor::Orange;
	}
#endif

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

	/**
	 * Visits only the grid-cell transitions crossed by a line segment using three-dimensional DDA.
	 * 3D DDAを使用し、線分が横切るグリッドセル遷移だけを訪問します。
	 */
	template <typename VisitorType>
	bool TraversePVSDDACells(
		const FVector& sourceLocalLocation,
		const FVector& targetLocalLocation,
		const FVector& gridSize,
		const FIntVector& sourceCell,
		const FIntVector& targetCell,
		VisitorType&& visitor)
	{
		if (gridSize.X <= 0.0 || gridSize.Y <= 0.0 || gridSize.Z <= 0.0)
			return false;
		if (sourceCell == targetCell)
			return true;

		const FVector inverseGridSize(1.0 / gridSize.X, 1.0 / gridSize.Y, 1.0 / gridSize.Z);
		const FVector sourceGridLocation = sourceLocalLocation * inverseGridSize;
		const FVector targetGridLocation = targetLocalLocation * inverseGridSize;
		const FVector direction = targetGridLocation - sourceGridLocation;
		const double infinity = TNumericLimits<double>::Max();

		FIntVector axisStep = FIntVector::ZeroValue;
		FVector tMaximum(infinity, infinity, infinity);
		FVector tDelta(infinity, infinity, infinity);
		auto initializeAxis = [](const double source, const double delta, const int32 currentCell, int32& step, double& maximum, double& interval)
		{
			if (delta > 0.0)
			{
				step = 1;
				maximum = (static_cast<double>(currentCell + 1) - source) / delta;
				interval = 1.0 / delta;
			}
			else if (delta < 0.0)
			{
				step = -1;
				maximum = (static_cast<double>(currentCell) - source) / delta;
				interval = -1.0 / delta;
			}
		};

		initializeAxis(sourceGridLocation.X, direction.X, sourceCell.X, axisStep.X, tMaximum.X, tDelta.X);
		initializeAxis(sourceGridLocation.Y, direction.Y, sourceCell.Y, axisStep.Y, tMaximum.Y, tDelta.Y);
		initializeAxis(sourceGridLocation.Z, direction.Z, sourceCell.Z, axisStep.Z, tMaximum.Z, tDelta.Z);

		FIntVector currentCell = sourceCell;
		const int32 maximumTransitionCount =
			FMath::Abs(targetCell.X - sourceCell.X) +
			FMath::Abs(targetCell.Y - sourceCell.Y) +
			FMath::Abs(targetCell.Z - sourceCell.Z);
		for (int32 transitionIndex = 0; transitionIndex < maximumTransitionCount && currentCell != targetCell; ++transitionIndex)
		{
			const double nextT = FMath::Min3(tMaximum.X, tMaximum.Y, tMaximum.Z);
			if (!FMath::IsFinite(nextT))
				return false;
			const double boundaryTolerance = 1.e-12 * FMath::Max(1.0, FMath::Abs(nextT));

			FIntVector nextCell = currentCell;
			if (FMath::Abs(tMaximum.X - nextT) <= boundaryTolerance)
			{
				nextCell.X += axisStep.X;
				tMaximum.X += tDelta.X;
			}
			if (FMath::Abs(tMaximum.Y - nextT) <= boundaryTolerance)
			{
				nextCell.Y += axisStep.Y;
				tMaximum.Y += tDelta.Y;
			}
			if (FMath::Abs(tMaximum.Z - nextT) <= boundaryTolerance)
			{
				nextCell.Z += axisStep.Z;
				tMaximum.Z += tDelta.Z;
			}

			if (nextCell == currentCell || !visitor(currentCell, nextCell))
				return false;
			currentCell = nextCell;
		}

		return currentCell == targetCell;
	}

	void SetManagedPointOrSpotLightVisibility(UPointLightComponent* lightComponent, const bool visible, const float fadeInTime, const float fadeOutTime)
	{
		if (!IsValid(lightComponent))
			return;

		if (UDungeonSpotLightComponent* dungeonSpotLightComponent = Cast<UDungeonSpotLightComponent>(lightComponent))
		{
			visible ? dungeonSpotLightComponent->TurnOn(fadeInTime) : dungeonSpotLightComponent->TurnOff(fadeOutTime);
			return;
		}

		if (UDungeonPointLightComponent* dungeonPointLightComponent = Cast<UDungeonPointLightComponent>(lightComponent))
		{
			visible ? dungeonPointLightComponent->TurnOn(fadeInTime) : dungeonPointLightComponent->TurnOff(fadeOutTime);
			return;
		}

		lightComponent->SetVisibility(visible);
	}
}

struct ADungeonMainLevelScriptActor::FPVSBuildStatistics
{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	uint64 DistancePassedPartitionPairCount = 0;
	uint64 PartitionPairVisibilityCallCount = 0;
	uint64 SamplePairAttemptCount = 0;
	uint64 TraceCallCount = 0;
	uint64 TraceStepCount = 0;
	uint64 DilationCallCount = 0;
	uint64 DilationVisitedPartitionCount = 0;
#endif
};

struct ADungeonMainLevelScriptActor::FPVSPartitionSamples
{
	TArray<int32> SourceSampleIndices;
	TArray<int32> TargetSampleIndices;
};

struct ADungeonMainLevelScriptActor::FPVSTraceContext
{
	const ADungeonGenerateActor* DungeonGenerateActor = nullptr;
	const UDungeonGenerateParameter* Parameter = nullptr;
	std::shared_ptr<const dungeon::Generator> Generator;
	std::shared_ptr<dungeon::Voxel> Voxel;
	FVector GridSize = FVector::OneVector;
	FVector ActorLocation = FVector::ZeroVector;

	/**
	 * Caches immutable data required by repeated PVS traces.
	 * PVSの反復Traceに必要な不変データをキャッシュします。
	 */
	bool Initialize(const ADungeonGenerateActor* dungeonGenerateActor)
	{
		if (!IsValid(dungeonGenerateActor) || !IsValid(dungeonGenerateActor->mParameter))
			return false;

		const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
		if (generator == nullptr || generator->GetVoxel() == nullptr)
			return false;

		DungeonGenerateActor = dungeonGenerateActor;
		Parameter = dungeonGenerateActor->mParameter;
		Generator = generator;
		Voxel = generator->GetVoxel();
		GridSize = Parameter->GetGridSize().To3D();
		ActorLocation = dungeonGenerateActor->GetActorLocation();
		return true;
	}

	/**
	 * Returns whether this context can trace the specified sample.
	 * このコンテキストで指定サンプルをTraceできるか返します。
	 */
	bool IsReadyFor(const FPartitionVisibilitySample& sample) const noexcept
	{
		return DungeonGenerateActor != nullptr && Parameter != nullptr && Voxel != nullptr && DungeonGenerateActor == sample.DungeonGenerateActor;
	}

	/**
	 * Tests whether a horizontal transition is open in both cells.
	 * 水平遷移が両方のセルで開いているか判定します。
	 */
	bool IsHorizontalTransitionOpen(const FIntVector& fromCell, const FIntVector& toCell) const
	{
		if (!Voxel->Contain(fromCell) || !Voxel->Contain(toCell))
			return false;

		const dungeon::Grid& fromGrid = Voxel->Get(fromCell);
		const dungeon::Grid& toGrid = Voxel->Get(toCell);
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
		return false;
	}

	/**
	 * Tests whether a single-axis transition is open.
	 * 単一軸の遷移が開いているか判定します。
	 */
	bool IsTransitionOpen(const FIntVector& fromCell, const FIntVector& toCell) const
	{
		if (!Voxel->Contain(fromCell) || !Voxel->Contain(toCell))
			return false;

		const dungeon::Grid& fromGrid = Voxel->Get(fromCell);
		const dungeon::Grid& toGrid = Voxel->Get(toCell);
		if (!IsTraversableGrid(fromGrid) || !IsTraversableGrid(toGrid))
			return false;

		const FIntVector delta = toCell - fromCell;
		if (FMath::Abs(delta.X) + FMath::Abs(delta.Y) == 1 && delta.Z == 0)
			return IsHorizontalTransitionOpen(fromCell, toCell);
		if (delta == FIntVector(0, 0, 1))
			return !fromGrid.HasCeiling() && !toGrid.HasFloor();
		if (delta == FIntVector(0, 0, -1))
			return !fromGrid.HasFloor() && !toGrid.HasCeiling();
		return false;
	}

	/**
	 * Preserves the existing slope exception for diagonal transitions.
	 * 斜め遷移に対する既存のスロープ例外を維持して判定します。
	 */
	bool IsSlopeDiagonalTransitionOpen(const FIntVector& startCell, const FIntVector& targetCell) const
	{
		if (!Voxel->Contain(startCell) || !Voxel->Contain(targetCell))
			return false;

		const dungeon::Grid& startGrid = Voxel->Get(startCell);
		const dungeon::Grid& targetGrid = Voxel->Get(targetCell);
		if (!IsTraversableGrid(startGrid) || !IsTraversableGrid(targetGrid))
			return false;

		const FIntVector delta = targetCell - startCell;
		if (delta.Z == 0 || FMath::Abs(delta.X) > 1 || FMath::Abs(delta.Y) > 1 || FMath::Abs(delta.Z) > 1)
			return false;
		if (FMath::Abs(delta.X) + FMath::Abs(delta.Y) <= 0)
			return false;

		std::array<FIntVector, 6> cellsToCheck;
		int32 cellCount = 0;
		cellsToCheck[cellCount++] = startCell;
		cellsToCheck[cellCount++] = targetCell;

		FIntVector currentHorizontalCell = startCell;
		if (delta.X != 0)
		{
			const FIntVector nextHorizontalCell = currentHorizontalCell + FIntVector(delta.X > 0 ? 1 : -1, 0, 0);
			if (!IsHorizontalTransitionOpen(currentHorizontalCell, nextHorizontalCell))
				return false;
			cellsToCheck[cellCount++] = nextHorizontalCell;
			cellsToCheck[cellCount++] = nextHorizontalCell + FIntVector(0, 0, delta.Z);
			currentHorizontalCell = nextHorizontalCell;
		}
		if (delta.Y != 0)
		{
			const FIntVector nextHorizontalCell = currentHorizontalCell + FIntVector(0, delta.Y > 0 ? 1 : -1, 0);
			if (!IsHorizontalTransitionOpen(currentHorizontalCell, nextHorizontalCell))
				return false;
			cellsToCheck[cellCount++] = nextHorizontalCell;
			cellsToCheck[cellCount++] = nextHorizontalCell + FIntVector(0, 0, delta.Z);
		}

		for (int32 cellIndex = 0; cellIndex < cellCount; ++cellIndex)
		{
			const FIntVector& cell = cellsToCheck[cellIndex];
			if (!Voxel->Contain(cell))
				continue;
			const dungeon::Grid& grid = Voxel->Get(cell);
			if (IsTraversableGrid(grid) && grid.IsKindOfSlopeType())
				return true;
		}
		return false;
	}

	/**
	 * Tests one fixed axis traversal order.
	 * 固定された1通りの軸移動順序を判定します。
	 */
	bool IsAxisOrderOpen(const FIntVector& startCell, const std::array<FIntVector, 3>& axisSteps, const int32* axisOrder, const int32 axisCount) const
	{
		FIntVector currentCell = startCell;
		for (int32 orderIndex = 0; orderIndex < axisCount; ++orderIndex)
		{
			const FIntVector nextCell = currentCell + axisSteps[axisOrder[orderIndex]];
			if (!IsTransitionOpen(currentCell, nextCell))
				return false;
			currentCell = nextCell;
		}
		return true;
	}

	/**
	 * Tests all valid axis orders in a fixed sequence.
	 * 有効な全軸順序を固定順で判定します。
	 */
	bool CanTraverseDelta(const FIntVector& startCell, const FIntVector& targetCell) const
	{
		const FIntVector delta = targetCell - startCell;
		if (FMath::Abs(delta.X) > 1 || FMath::Abs(delta.Y) > 1 || FMath::Abs(delta.Z) > 1)
			return false;

		const int32 manhattanDistance = FMath::Abs(delta.X) + FMath::Abs(delta.Y) + FMath::Abs(delta.Z);
		if (manhattanDistance == 0)
			return true;
		if (manhattanDistance == 1)
			return IsTransitionOpen(startCell, targetCell);
		if (IsSlopeDiagonalTransitionOpen(startCell, targetCell))
			return true;

		std::array<FIntVector, 3> axisSteps = { FIntVector::ZeroValue, FIntVector::ZeroValue, FIntVector::ZeroValue };
		int32 axisCount = 0;
		if (delta.X != 0)
			axisSteps[axisCount++] = FIntVector(delta.X > 0 ? 1 : -1, 0, 0);
		if (delta.Y != 0)
			axisSteps[axisCount++] = FIntVector(0, delta.Y > 0 ? 1 : -1, 0);
		if (delta.Z != 0)
			axisSteps[axisCount++] = FIntVector(0, 0, delta.Z > 0 ? 1 : -1);

		static constexpr int32 AxisOrders[6][3] = {
			{ 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 },
			{ 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 }
		};
		const int32 orderCount = axisCount == 2 ? 2 : 6;
		if (axisCount == 2)
		{
			static constexpr int32 TwoAxisOrders[2][2] = { { 0, 1 }, { 1, 0 } };
			for (int32 orderIndex = 0; orderIndex < orderCount; ++orderIndex)
			{
				if (IsAxisOrderOpen(startCell, axisSteps, TwoAxisOrders[orderIndex], axisCount))
					return true;
			}
			return false;
		}
		for (int32 orderIndex = 0; orderIndex < orderCount; ++orderIndex)
		{
			if (IsAxisOrderOpen(startCell, axisSteps, AxisOrders[orderIndex], axisCount))
				return true;
		}
		return false;
	}
};

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

void ADungeonMainLevelScriptActor::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITOR
	RegisterDebugLegend();
#endif
}

void ADungeonMainLevelScriptActor::RebuildSparsePartitionGraphAndRefresh()
{
	ExecutePartitionBuild(FPartitionBuildOptions{ EPartitionBuildReason::RuntimeRebuild });
}

void ADungeonMainLevelScriptActor::RequestSpatialMeshGroupPartitionLinkRefresh()
{
	mSpatialMeshGroupPartitionLinksDirty = true;
	SetActorTickEnabled(true);
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
	BuildInstancedMeshGroupPartitionLinks(context);
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
 * 1 つのダンジョン actor が持つ集計値を共通コンテキストへ加算します。
 */
void ADungeonMainLevelScriptActor::AccumulatePartitionBuildActor(const ADungeonGenerateActor* dungeonGenerateActor, FPartitionBuildContext& context)
{
	FBox traversableGridBounds(EForceInit::ForceInit);
	if (IsValid(dungeonGenerateActor->mParameter))
	{
		if (const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator())
		{
			if (const auto& voxel = generator->GetVoxel())
			{
				const FVector actorLocation = dungeonGenerateActor->GetActorLocation();
				const FVector gridSize3D = dungeonGenerateActor->mParameter->GetGridSize().To3D();
				voxel->Each([dungeonGenerateActor, actorLocation, gridSize3D, &traversableGridBounds](const FIntVector& location, const dungeon::Grid& grid)
					{
						if (!IsTraversableGrid(grid))
							return true;

						const FVector cellMin = dungeonGenerateActor->mParameter->ToWorld(location) + actorLocation;
						traversableGridBounds += FBox(cellMin, cellMin + gridSize3D);
						return true;
					}
				);
			}
		}
	}

	context.Bounding += traversableGridBounds.IsValid ? traversableGridBounds : dungeonGenerateActor->CalculateBoundingBox();

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

	DUNGEON_GENERATOR_VERBOSE(TEXT("LongestStraightPath: X=%f, Y=%f (%s)"), dungeonLongestStraightPath.X, dungeonLongestStraightPath.Y, *dungeonGenerateActor->GetName());
	DUNGEON_GENERATOR_VERBOSE(TEXT("RoomMaxSize: X=%f, Y=%f (%s)"), dungeonRoomSize.X, dungeonRoomSize.Y, *dungeonGenerateActor->GetName());
	DUNGEON_GENERATOR_VERBOSE(TEXT("GridSize=%f (%s)"), gridSize, *dungeonGenerateActor->GetName());
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
	return context.Bounding.IsValid != 0;
}

/*
 * Clears the current partition-related runtime state before a rebuild starts.
 * 再構築開始前に現在のパーティション関連 runtime 状態を消去します。
 */
void ADungeonMainLevelScriptActor::ResetPartitionBuildState()
{
	ResetInstancedMeshGroupPartitionLinks();
	mBounding.Init();
	mTheoreticalMaxVisibilityDistance = 0.f;
	mPartitionWorldSize = FVector::OneVector;
	mPartitionGridCount = FIntVector(AutoPartitionHorizontalMinGridCount, AutoPartitionHorizontalMinGridCount, AutoPartitionVerticalMinGridCount);
	DungeonPartitions.Reset();
	mPartitionIndexByCell.Reset();
	mDungeonGenerateActors.Reset();
	ResetPrecomputedPartitionVisibility();
	ResetPartitionTransitionQueue();
}

/*
 * Restores fail-open visibility and clears every transient partition-to-group association.
 * 安全側の表示状態へ復元し、一時的なPartitionとGroupの関連をすべて消去します。
 */
void ADungeonMainLevelScriptActor::ResetInstancedMeshGroupPartitionLinks()
{
	for (const auto& pair : mActivePartitionCountByInstancedMeshGroup)
		SetInstancedMeshGroupVisible(pair.Key, true);

	mInstancedMeshGroupsByPartition.Reset();
	mActivePartitionCountByInstancedMeshGroup.Reset();
	mSpatialMeshGroupPartitionLinksDirty = false;
}

/*
 * Applies partition-controlled visibility to one actor-owned instanced mesh group.
 * 1つのActor所有Instanced Mesh GroupへPartition制御の表示状態を適用します。
 */
void ADungeonMainLevelScriptActor::SetInstancedMeshGroupVisible(const FInstancedMeshGroupHandle& handle, const bool visible) const
{
	ADungeonGenerateActor* dungeonGenerateActor = handle.DungeonGenerateActor.Get();
	if (!IsValid(dungeonGenerateActor))
		return;

	if (FDungeonInstancedMeshCluster* cluster = dungeonGenerateActor->mInstancedMeshClusters.Find(handle.GroupCoordinate))
		cluster->SetPartitionVisible(visible);
}

/*
 * Builds sparse associations between occupied instanced mesh group bounds and partition cells.
 * Instanced Mesh Groupの占有境界とPartition Cell間の疎な関連を構築します。
 */
void ADungeonMainLevelScriptActor::BuildInstancedMeshGroupPartitionLinks(const FPartitionBuildContext& context)
{
	ResetInstancedMeshGroupPartitionLinks();
	mInstancedMeshGroupsByPartition.SetNum(DungeonPartitions.Num());
	if (DungeonPartitions.IsEmpty() || !mBounding.IsValid)
		return;

	const FVector clampedBoundsMaximum = mBounding.Max - FVector(UE_KINDA_SMALL_NUMBER);

	auto linkGroup = [this, &clampedBoundsMaximum](const FInstancedMeshGroupHandle& handle, const FBox& groupBounds)
	{
		if (!groupBounds.IsValid)
		{
			SetInstancedMeshGroupVisible(handle, true);
			return;
		}

		constexpr double BoundsExpansion = 1.0;
		const FBox expandedGroupBounds = groupBounds.ExpandBy(BoundsExpansion);
		if (!expandedGroupBounds.Intersect(mBounding))
		{
			SetInstancedMeshGroupVisible(handle, true);
			return;
		}

		const FBox clippedGroupBounds(
			expandedGroupBounds.Min.ComponentMax(mBounding.Min),
			expandedGroupBounds.Max.ComponentMin(clampedBoundsMaximum)
		);
		if (!clippedGroupBounds.IsValid)
		{
			SetInstancedMeshGroupVisible(handle, true);
			return;
		}

		const FIntVector minimumCell = ToPartitionCell(clippedGroupBounds.Min);
		const FIntVector maximumCell = ToPartitionCell(clippedGroupBounds.Max);
		int32 linkedPartitionCount = 0;
		int32 activePartitionCount = 0;
		for (int32 z = minimumCell.Z; z <= maximumCell.Z; ++z)
		{
			for (int32 y = minimumCell.Y; y <= maximumCell.Y; ++y)
			{
				for (int32 x = minimumCell.X; x <= maximumCell.X; ++x)
				{
					const int32* partitionIndex = mPartitionIndexByCell.Find(FIntVector(x, y, z));
					if (partitionIndex == nullptr || !DungeonPartitions.IsValidIndex(*partitionIndex))
						continue;

					const UDungeonPartition* partition = DungeonPartitions[*partitionIndex];
					if (!IsValid(partition) || !partition->GetBounds().Intersect(expandedGroupBounds))
						continue;

					mInstancedMeshGroupsByPartition[*partitionIndex].Add(handle);
					++linkedPartitionCount;
					if (partition->IsPartitionActivate())
						++activePartitionCount;
				}
			}
		}

		if (linkedPartitionCount > 0)
		{
			mActivePartitionCountByInstancedMeshGroup.Add(handle, activePartitionCount);
			SetInstancedMeshGroupVisible(handle, activePartitionCount > 0);
		}
		else
		{
			SetInstancedMeshGroupVisible(handle, true);
		}
	};

	for (ADungeonGenerateActor* dungeonGenerateActor : context.DungeonGenerateActors)
	{
		if (!IsValid(dungeonGenerateActor))
			continue;

		for (auto& clusterPair : dungeonGenerateActor->mInstancedMeshClusters)
		{
			FInstancedMeshGroupHandle handle;
			handle.DungeonGenerateActor = dungeonGenerateActor;
			handle.GroupCoordinate = clusterPair.Key;
			linkGroup(handle, clusterPair.Value.GetPartitionBounds());
		}
	}
}

/*
 * Updates the active-partition reference count for every group linked to one partition.
 * 1つのPartitionに関連する全GroupのActive Partition参照数を更新します。
 */
void ADungeonMainLevelScriptActor::UpdateInstancedMeshGroupPartitionActivation(const int32 partitionIndex, const bool active)
{
	if (!mInstancedMeshGroupsByPartition.IsValidIndex(partitionIndex))
		return;

	for (const FInstancedMeshGroupHandle& handle : mInstancedMeshGroupsByPartition[partitionIndex])
	{
		int32* activePartitionCount = mActivePartitionCountByInstancedMeshGroup.Find(handle);
		if (activePartitionCount == nullptr)
			continue;

		const int32 previousCount = *activePartitionCount;
		*activePartitionCount = active ? previousCount + 1 : FMath::Max(0, previousCount - 1);
		if (previousCount == 0 && *activePartitionCount > 0)
			SetInstancedMeshGroupVisible(handle, true);
		else if (previousCount > 0 && *activePartitionCount == 0)
			SetInstancedMeshGroupVisible(handle, false);
	}
}

/*
 * Applies one partition state transition and synchronizes linked instanced mesh groups.
 * 1つのPartition状態遷移を適用し、関連するInstanced Mesh Groupを同期します。
 */
void ADungeonMainLevelScriptActor::SetPartitionActivationState(const int32 partitionIndex, const bool active, const bool resetPartitionInactivateRemainTimer)
{
	if (!DungeonPartitions.IsValidIndex(partitionIndex))
		return;

	UDungeonPartition* partition = DungeonPartitions[partitionIndex];
	if (!IsValid(partition))
		return;

	const bool wasActive = partition->IsPartitionActivate();
	if (active)
		partition->CallPartitionActivate(resetPartitionInactivateRemainTimer);
	else
		partition->CallPartitionInactivate();

	if (wasActive != partition->IsPartitionActivate())
		UpdateInstancedMeshGroupPartitionActivation(partitionIndex, partition->IsPartitionActivate());
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
	mDungeonGenerateActors.Reset(context.DungeonGenerateActors.Num());
	for (ADungeonGenerateActor* dungeonGenerateActor : context.DungeonGenerateActors)
	{
		if (IsValid(dungeonGenerateActor))
			mDungeonGenerateActors.Add(dungeonGenerateActor);
	}
}

/*
 * Rebuilds graph, visibility, and transition data using the committed runtime members.
 * 確定反映済み runtime member を使って graph、visibility、遷移情報を再構築します。
 */
void ADungeonMainLevelScriptActor::BuildPartitionRuntimeData(const FPartitionBuildContext& context)
{
	MEASURE_TIME_START(totalStopwatch);
	MEASURE_TIME_START(stepStopwatch);

	BuildSparsePartitionGraph(context.DungeonGenerateActors);
	MEASURE_TIME_LAP(stepStopwatch, TEXT("Sparse partition graph"));

	BuildPartitionVisibilitySamples(context.DungeonGenerateActors);
	MEASURE_TIME_LAP(stepStopwatch, TEXT("Partition visibility samples"));

	BuildPartitionConnectedComponents();
	MEASURE_TIME_LAP(stepStopwatch, TEXT("Partition connected components"));

	BuildPrecomputedPartitionVisibility(context.DungeonGenerateActors);
	MEASURE_TIME_LAP(stepStopwatch, TEXT("Precomputed partition visibility (PVS)"));

	ResetPartitionTransitionQueue();
	MEASURE_TIME_LAP(totalStopwatch, TEXT("Partition/PVS build total"));
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
		RestorePointAndSpotLightStates();
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

		// ポイントライトおよびスポットライトの表示と影を制御
		UpdatePointAndSpotLightStates();
	}
	else
	{
		ForceActivate();
		RestorePointAndSpotLightStates();
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

			SetPartitionActivationState(partitionIndex, true, false);
			--remainingActivations;
		}
		else
		{
			if (remainingInactivations <= 0)
			{
				mPendingPartitionTransitions.Add(partitionIndex);
				continue;
			}

			SetPartitionActivationState(partitionIndex, false, false);
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
#if WITH_EDITOR
	UnregisterDebugLegend();
#endif

	Super::EndPlay(endPlayReason);

	ResetInstancedMeshGroupPartitionLinks();
	DungeonPartitions.Reset();
	mPartitionIndexByCell.Reset();
	mDungeonGenerateActors.Reset();
	ResetPrecomputedPartitionVisibility();
	mBounding.Init();
	ResetPartitionTransitionQueue();
}

void ADungeonMainLevelScriptActor::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (mSpatialMeshGroupPartitionLinksDirty && mBounding.IsValid)
	{
		FPartitionBuildContext context;
		for (const TWeakObjectPtr<ADungeonGenerateActor>& weakDungeonGenerateActor : mDungeonGenerateActors)
		{
			if (ADungeonGenerateActor* dungeonGenerateActor = weakDungeonGenerateActor.Get())
				context.DungeonGenerateActors.Add(dungeonGenerateActor);
		}
		BuildInstancedMeshGroupPartitionLinks(context);
	}

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

			// ポイントライトおよびスポットライトの表示と影を制御
			UpdatePointAndSpotLightStates();
		}
		else
		{
			ForceActivate();
			RestorePointAndSpotLightStates();
		}

		mLastEnableLoadControl = true;
	}
	else
	{
		if (mLastEnableLoadControl)
		{
			mLastEnableLoadControl = false;
			ForceActivate();
			RestorePointAndSpotLightStates();
		}
	}

#if WITH_EDITOR
	if (ShowRuntimePartitionState || ShowPartitionConstruction || ShowPVSResult || ShowPVSBuildSamples)
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
 * キャッシュ済みダンジョンActorを使ってワールド座標にある生成グリッドのIdentifierを検索します。
 */
bool ADungeonMainLevelScriptActor::FindGridIdentifier(const FVector& worldLocation, uint16& identifier) const noexcept
{
	for (const TWeakObjectPtr<ADungeonGenerateActor>& weakDungeonGenerateActor : mDungeonGenerateActors)
	{
		const ADungeonGenerateActor* dungeonGenerateActor = weakDungeonGenerateActor.Get();
		if (!IsValid(dungeonGenerateActor) || !IsValid(dungeonGenerateActor->mParameter))
			continue;

		const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
		if (generator == nullptr)
			continue;
		const std::shared_ptr<dungeon::Voxel> voxel = generator->GetVoxel();
		if (voxel == nullptr)
			continue;

		const FIntVector gridLocation = dungeonGenerateActor->mParameter->ToGrid(worldLocation - dungeonGenerateActor->GetActorLocation());
		if (!voxel->Contain(gridLocation))
			continue;

		const dungeon::Grid& grid = voxel->Get(gridLocation);
		if (grid.IsInvalidIdentifier())
			continue;

		identifier = static_cast<uint16>(grid.GetIdentifier());
		return true;
	}

	return false;
}

bool ADungeonMainLevelScriptActor::IsPartitionActive(const int32 partitionIndex) const noexcept
{
	if (!DungeonPartitions.IsValidIndex(partitionIndex))
	{
		return false;
	}

	const UDungeonPartition* partition = DungeonPartitions[partitionIndex];
	return IsValid(partition) && partition->IsPartitionActivate();
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

/**
 * Builds the center and eight-corner anchors used to select spatial visibility samples.
 * 空間可視性サンプルの選択に使う中央と8隅のアンカーを構築します。
 */
TArray<FVector> ADungeonMainLevelScriptActor::MakeSpatialVisibilitySampleAnchors(const FBox& bounds)
{
	TArray<FVector> anchors;
	anchors.Reserve(9);
	anchors.Add(bounds.GetCenter());
	for (int32 z = 0; z < 2; ++z)
	{
		for (int32 y = 0; y < 2; ++y)
		{
			for (int32 x = 0; x < 2; ++x)
			{
				anchors.Add(FVector(
					x == 0 ? bounds.Min.X : bounds.Max.X,
					y == 0 ? bounds.Min.Y : bounds.Max.Y,
					z == 0 ? bounds.Min.Z : bounds.Max.Z
				));
			}
		}
	}
	return anchors;
}

/**
 * Builds the center, corners, and edge-midpoint anchors used to select roof samples.
 * 屋根サンプルの選択に使う中央、四隅、四辺中央のアンカーを構築します。
 */
TArray<FVector> ADungeonMainLevelScriptActor::MakeRoofVisibilitySampleAnchors(const FBox& bounds)
{
	const FVector center = bounds.GetCenter();
	const float z = bounds.Max.Z;
	return {
		FVector(center.X, center.Y, z),
		FVector(bounds.Min.X, bounds.Min.Y, z),
		FVector(center.X, bounds.Min.Y, z),
		FVector(bounds.Max.X, bounds.Min.Y, z),
		FVector(bounds.Min.X, center.Y, z),
		FVector(bounds.Max.X, center.Y, z),
		FVector(bounds.Min.X, bounds.Max.Y, z),
		FVector(center.X, bounds.Max.Y, z),
		FVector(bounds.Max.X, bounds.Max.Y, z)
	};
}

/**
 * Appends the closest candidate for each anchor while collapsing identical actor, grid, and type entries.
 * 各アンカーに最も近い候補を追加し、同じActor、グリッド、種別の項目を集約します。
 */
void ADungeonMainLevelScriptActor::AppendClosestUniqueVisibilitySamples(
	TArray<FPartitionVisibilitySample>& destination,
	const TArray<FPartitionVisibilitySample>& candidates,
	const TArray<FVector>& anchors,
	const EPartitionVisibilitySampleType sampleType)
{
	auto containsSample = [&destination, sampleType](const FPartitionVisibilitySample& candidate)
	{
		return destination.ContainsByPredicate([&candidate, sampleType](const FPartitionVisibilitySample& existing)
			{
				return
					existing.DungeonGenerateActor == candidate.DungeonGenerateActor &&
					existing.GridLocation == candidate.GridLocation &&
					existing.SampleType == sampleType;
			}
		);
	};

	for (const FVector& anchor : anchors)
	{
		const FPartitionVisibilitySample* closestSample = nullptr;
		double closestDistanceSquared = TNumericLimits<double>::Max();
		for (const FPartitionVisibilitySample& candidate : candidates)
		{
			if (!IsValid(candidate.DungeonGenerateActor) || containsSample(candidate))
				continue;

			const double distanceSquared = FVector::DistSquared(anchor, candidate.WorldLocation);
			if (distanceSquared < closestDistanceSquared)
			{
				closestSample = &candidate;
				closestDistanceSquared = distanceSquared;
			}
		}

		if (closestSample != nullptr)
		{
			FPartitionVisibilitySample selectedSample = *closestSample;
			selectedSample.SampleType = sampleType;
			destination.Add(MoveTemp(selectedSample));
		}
	}
}

/**
 * Appends every candidate not already represented by the same actor, grid, and sample type.
 * 同じActor、グリッド、サンプル種別で未登録の全候補を順序を維持して追加します。
 */
void ADungeonMainLevelScriptActor::AppendRemainingUniqueVisibilitySamples(
	TArray<FPartitionVisibilitySample>& destination,
	const TArray<FPartitionVisibilitySample>& candidates,
	const EPartitionVisibilitySampleType sampleType)
{
	for (const FPartitionVisibilitySample& candidate : candidates)
	{
		if (!IsValid(candidate.DungeonGenerateActor))
			continue;

		const bool bAlreadySelected = destination.ContainsByPredicate([&candidate, sampleType](const FPartitionVisibilitySample& existing)
			{
				return
					existing.DungeonGenerateActor == candidate.DungeonGenerateActor &&
					existing.GridLocation == candidate.GridLocation &&
					existing.SampleType == sampleType;
			}
		);
		if (bAlreadySelected)
			continue;

		FPartitionVisibilitySample selectedSample = candidate;
		selectedSample.SampleType = sampleType;
		destination.Add(MoveTemp(selectedSample));
	}
}

/**
 * Returns whether a sample represents a possible player-side visibility origin.
 * サンプルがプレイヤー側の可視性起点として使用可能か返します。
 */
bool ADungeonMainLevelScriptActor::IsVisibilitySampleUsableAsSource(const EPartitionVisibilitySampleType sampleType) noexcept
{
	return sampleType != EPartitionVisibilitySampleType::Roof;
}

void ADungeonMainLevelScriptActor::BuildPartitionVisibilitySamples(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors)
{
	mPartitionVisibilitySamples.Reset();
	mPartitionVisibilitySamples.SetNum(DungeonPartitions.Num());
	if (DungeonPartitions.IsEmpty())
		return;

	TArray<TArray<FPartitionVisibilitySample>> traversableCandidates;
	traversableCandidates.SetNum(DungeonPartitions.Num());
	TArray<TArray<FPartitionVisibilitySample>> roofCandidates;
	roofCandidates.SetNum(DungeonPartitions.Num());

	for (int32 dungeonGenerateActorIndex = 0; dungeonGenerateActorIndex < dungeonGenerateActors.Num(); ++dungeonGenerateActorIndex)
	{
		ADungeonGenerateActor* dungeonGenerateActor = dungeonGenerateActors[dungeonGenerateActorIndex];
		if (!IsValid(dungeonGenerateActor) || !IsValid(dungeonGenerateActor->mParameter))
			continue;

		const std::shared_ptr<const dungeon::Generator> generator = dungeonGenerateActor->GetGenerator();
		if (generator == nullptr)
			continue;

		const auto& voxel = generator->GetVoxel();
		if (voxel == nullptr)
			continue;

		const FVector gridSize = dungeonGenerateActor->mParameter->GetGridSize().To3D();
		const FVector gridHalfSize = gridSize * 0.5f;
		voxel->Each([this, &traversableCandidates, &roofCandidates, dungeonGenerateActor, dungeonGenerateActorIndex, generator, gridSize, gridHalfSize](const FIntVector& location, const dungeon::Grid& grid)
			{
				if (!IsTraversableGrid(grid))
					return true;

				const FVector worldLocation = dungeonGenerateActor->mParameter->ToWorld(location) + dungeonGenerateActor->GetActorLocation() + gridHalfSize;
				const int32 partitionIndex = FindPartitionIndex(worldLocation);
				if (!DungeonPartitions.IsValidIndex(partitionIndex))
					return true;

				const auto makeSample = [dungeonGenerateActor, dungeonGenerateActorIndex, location](const FVector& sampleWorldLocation, const EPartitionVisibilitySampleType sampleType)
				{
					FPartitionVisibilitySample sample;
					sample.DungeonGenerateActor = dungeonGenerateActor;
					sample.GridLocation = location;
					sample.WorldLocation = sampleWorldLocation;
					sample.SampleType = sampleType;
					sample.TraceContextIndex = dungeonGenerateActorIndex;
					return sample;
				};

				traversableCandidates[partitionIndex].Add(makeSample(worldLocation, EPartitionVisibilitySampleType::Center));

				const FIntVector upperLocation = location + FIntVector(0, 0, 1);
				if (grid.CanBuildRoof(generator->GetGrid(upperLocation), true))
				{
					const FVector roofWorldLocation = ADungeonGenerateBase::CalculateRoofPartitionRegistrationWorldLocation(worldLocation, gridSize);
					roofCandidates[partitionIndex].Add(makeSample(roofWorldLocation, EPartitionVisibilitySampleType::Roof));
				}

				if (grid.IsKindOfSlopeType())
				{
					const FVector slopeDirection = FVector(
						static_cast<float>(grid.GetDirection().GetVector().X),
						static_cast<float>(grid.GetDirection().GetVector().Y),
						0.f
					);
					const FVector catwalkDirection = FVector(
						static_cast<float>(grid.GetCatwalkDirection().GetVector().X),
						static_cast<float>(grid.GetCatwalkDirection().GetVector().Y),
						0.f
					);
					const FVector slopeOffset(
						slopeDirection.X * gridSize.X * 0.35f,
						slopeDirection.Y * gridSize.Y * 0.35f,
						gridSize.Z * 0.25f
					);
					const FVector catwalkOffset(
						catwalkDirection.X * gridSize.X * 0.2f,
						catwalkDirection.Y * gridSize.Y * 0.2f,
						0.f
					);

					TArray<FPartitionVisibilitySample>& samples = mPartitionVisibilitySamples[partitionIndex];
					samples.Add(makeSample(worldLocation - slopeOffset + catwalkOffset, EPartitionVisibilitySampleType::SlopeLower));
					samples.Add(makeSample(worldLocation + slopeOffset + catwalkOffset, EPartitionVisibilitySampleType::SlopeUpper));
				}
				return true;
			}
		);
	}

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !IsValid(DungeonPartitions[partitionIndex]))
			continue;

		TArray<FPartitionVisibilitySample>& samples = mPartitionVisibilitySamples[partitionIndex];
		const FBox& bounds = DungeonPartitions[partitionIndex]->GetBounds();
		const TArray<FVector> spatialAnchors = MakeSpatialVisibilitySampleAnchors(bounds);
		const TArray<FVector> centerAnchor = { spatialAnchors[0] };
		TArray<FVector> cornerAnchors;
		cornerAnchors.Append(spatialAnchors.GetData() + 1, spatialAnchors.Num() - 1);
		AppendClosestUniqueVisibilitySamples(samples, traversableCandidates[partitionIndex], centerAnchor, EPartitionVisibilitySampleType::Center);
		AppendClosestUniqueVisibilitySamples(samples, traversableCandidates[partitionIndex], cornerAnchors, EPartitionVisibilitySampleType::Edge);
		AppendClosestUniqueVisibilitySamples(samples, roofCandidates[partitionIndex], MakeRoofVisibilitySampleAnchors(bounds), EPartitionVisibilitySampleType::Roof);
		AppendRemainingUniqueVisibilitySamples(samples, roofCandidates[partitionIndex], EPartitionVisibilitySampleType::Roof);
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

void ADungeonMainLevelScriptActor::BuildPrecomputedPartitionVisibility(const TArray<ADungeonGenerateActor*>& dungeonGenerateActors)
{
	MEASURE_TIME_START(stopwatch);

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
	MEASURE_TIME_LAP(stopwatch, TEXT("PVS visibility mask initialization"));

	if (mPartitionVisibilitySamples.Num() != partitionCount || mPartitionConnectedComponents.Num() != partitionCount)
		return;

	const double maximumVisibilityDistanceSquared =
		mTheoreticalMaxVisibilityDistance > 0.f ?
		FMath::Square(static_cast<double>(mTheoreticalMaxVisibilityDistance)) :
		TNumericLimits<double>::Max();
	const int32 visibilityDilationHopCount = FMath::Max(0, PrecomputedVisibilityDilationHopCount);

	TArray<FPVSTraceContext> traceContexts;
	traceContexts.SetNum(dungeonGenerateActors.Num());
	for (int32 dungeonGenerateActorIndex = 0; dungeonGenerateActorIndex < dungeonGenerateActors.Num(); ++dungeonGenerateActorIndex)
	{
		traceContexts[dungeonGenerateActorIndex].Initialize(dungeonGenerateActors[dungeonGenerateActorIndex]);
	}

	TArray<FPVSPartitionSamples> partitionSamples;
	partitionSamples.SetNum(partitionCount);
	for (int32 partitionIndex = 0; partitionIndex < partitionCount; ++partitionIndex)
	{
		BuildUniqueVisibilitySampleIndices(mPartitionVisibilitySamples[partitionIndex], true, partitionSamples[partitionIndex].SourceSampleIndices);
		BuildUniqueVisibilitySampleIndices(mPartitionVisibilitySamples[partitionIndex], false, partitionSamples[partitionIndex].TargetSampleIndices);
	}

#if WITH_EDITOR
	const bool validateOptimization = CVarValidatePVSOptimization.GetValueOnGameThread() != 0;
#else
	constexpr bool validateOptimization = false;
#endif
	MEASURE_TIME_LAP(stopwatch, TEXT("PVS parallel evaluation setup"));

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	TArray<FPVSBuildStatistics> sourceStatistics;
	sourceStatistics.SetNum(partitionCount);
#endif

	ParallelForTemplate(partitionCount, [this, partitionCount, maximumVisibilityDistanceSquared, visibilityDilationHopCount, &traceContexts, &partitionSamples, validateOptimization
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		, &sourceStatistics
#endif
	](const int32 sourcePartitionIndex)
		{
			if (!DungeonPartitions.IsValidIndex(sourcePartitionIndex) || !IsValid(DungeonPartitions[sourcePartitionIndex]))
				return;

			FPVSBuildStatistics statistics;
			TArray<uint64>& row = mPartitionPotentialVisibilityMasks[sourcePartitionIndex];
			const UDungeonPartition* sourcePartition = DungeonPartitions[sourcePartitionIndex];
			auto setVisible = [&row](const int32 partitionIndex)
			{
				row[partitionIndex / 64] |= (1ull << (partitionIndex % 64));
			};
			auto expandVisibleNeighbors = [this, sourcePartitionIndex, visibilityDilationHopCount, &setVisible, &statistics](const int32 centerPartitionIndex)
			{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
				++statistics.DilationCallCount;
#endif
				if (visibilityDilationHopCount <= 0)
					return;

				TArray<int32, TInlineAllocator<16>> frontier;
				frontier.Add(centerPartitionIndex);
				TSet<int32> visited;
				visited.Reserve(16);
				visited.Add(centerPartitionIndex);
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
				++statistics.DilationVisitedPartitionCount;
#endif

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
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
							++statistics.DilationVisitedPartitionCount;
#endif
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
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
				++statistics.DistancePassedPartitionPairCount;
#endif

				bool potentiallyVisible = sourcePartition->GetNeighborIndices().Contains(targetPartitionIndex);
				if (!potentiallyVisible)
				{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
					++statistics.PartitionPairVisibilityCallCount;
#endif
					potentiallyVisible = IsPartitionPairPotentiallyVisible(sourcePartitionIndex, targetPartitionIndex, traceContexts, partitionSamples, validateOptimization, statistics);
				}

				if (potentiallyVisible)
					setVisibleWithDilation(targetPartitionIndex);
			}

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			sourceStatistics[sourcePartitionIndex] = statistics;
#endif
		},
		EParallelForFlags::Unbalanced
	);
	MEASURE_TIME_LAP(stopwatch, TEXT("PVS ParallelFor"));

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
	FPVSBuildStatistics totalStatistics;
	uint64 maximumSamplePairAttemptCount = 0;
	uint64 maximumTraceStepCount = 0;
	int32 maximumSamplePairSourcePartitionIndex = INDEX_NONE;
	int32 maximumTraceStepSourcePartitionIndex = INDEX_NONE;
	for (int32 sourcePartitionIndex = 0; sourcePartitionIndex < sourceStatistics.Num(); ++sourcePartitionIndex)
	{
		const FPVSBuildStatistics& statistics = sourceStatistics[sourcePartitionIndex];
		totalStatistics.DistancePassedPartitionPairCount += statistics.DistancePassedPartitionPairCount;
		totalStatistics.PartitionPairVisibilityCallCount += statistics.PartitionPairVisibilityCallCount;
		totalStatistics.SamplePairAttemptCount += statistics.SamplePairAttemptCount;
		totalStatistics.TraceCallCount += statistics.TraceCallCount;
		totalStatistics.TraceStepCount += statistics.TraceStepCount;
		totalStatistics.DilationCallCount += statistics.DilationCallCount;
		totalStatistics.DilationVisitedPartitionCount += statistics.DilationVisitedPartitionCount;

		if (maximumSamplePairSourcePartitionIndex == INDEX_NONE || statistics.SamplePairAttemptCount > maximumSamplePairAttemptCount)
		{
			maximumSamplePairAttemptCount = statistics.SamplePairAttemptCount;
			maximumSamplePairSourcePartitionIndex = sourcePartitionIndex;
		}
		if (maximumTraceStepSourcePartitionIndex == INDEX_NONE || statistics.TraceStepCount > maximumTraceStepCount)
		{
			maximumTraceStepCount = statistics.TraceStepCount;
			maximumTraceStepSourcePartitionIndex = sourcePartitionIndex;
		}
	}

	DUNGEON_GENERATOR_MEASURE_SCOPE();
	DUNGEON_GENERATOR_MEASURE(TEXT("PVS statistics: partitions=%d, distance-passed pairs=%llu, pair visibility calls=%llu"), partitionCount, totalStatistics.DistancePassedPartitionPairCount, totalStatistics.PartitionPairVisibilityCallCount);
	DUNGEON_GENERATOR_MEASURE(TEXT("PVS trace statistics: sample-pair attempts=%llu, trace calls=%llu, cell transition checks=%llu"), totalStatistics.SamplePairAttemptCount, totalStatistics.TraceCallCount, totalStatistics.TraceStepCount);
	DUNGEON_GENERATOR_MEASURE(TEXT("PVS dilation statistics: calls=%llu, visited partitions=%llu"), totalStatistics.DilationCallCount, totalStatistics.DilationVisitedPartitionCount);
	DUNGEON_GENERATOR_MEASURE(TEXT("PVS maximum source load: sample-pair attempts=%llu (source=%d), cell transition checks=%llu (source=%d)"), maximumSamplePairAttemptCount, maximumSamplePairSourcePartitionIndex, maximumTraceStepCount, maximumTraceStepSourcePartitionIndex);

	constexpr uint64 FNVOffsetBasis = 14695981039346656037ull;
	constexpr uint64 FNVPrime = 1099511628211ull;
	uint64 visibilityFingerprint = FNVOffsetBasis;
	uint64 visiblePartitionCount = 0;
	for (const TArray<uint64>& row : mPartitionPotentialVisibilityMasks)
	{
		visibilityFingerprint = (visibilityFingerprint ^ static_cast<uint64>(row.Num())) * FNVPrime;
		for (const uint64 word : row)
		{
			visibilityFingerprint = (visibilityFingerprint ^ word) * FNVPrime;
			uint64 remainingBits = word;
			while (remainingBits != 0)
			{
				remainingBits &= remainingBits - 1;
				++visiblePartitionCount;
			}
		}
	}
	DUNGEON_GENERATOR_MEASURE(TEXT("PVS mask: fingerprint=%016llx, visible bits=%llu"), visibilityFingerprint, visiblePartitionCount);
#endif
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
	return bEnableLoadControl && HasPrecomputedPartitionVisibility();
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

/**
 * Builds a first-occurrence list of exact PVS trace samples.
 * 完全一致するPVS Traceサンプルを先着順で重複排除します。
 */
void ADungeonMainLevelScriptActor::BuildUniqueVisibilitySampleIndices(const TArray<FPartitionVisibilitySample>& samples, const bool sourceSamplesOnly, TArray<int32>& outputIndices)
{
	outputIndices.Reset(samples.Num());
	for (int32 sampleIndex = 0; sampleIndex < samples.Num(); ++sampleIndex)
	{
		const FPartitionVisibilitySample& sample = samples[sampleIndex];
		if (sourceSamplesOnly && !IsVisibilitySampleUsableAsSource(sample.SampleType))
			continue;

		const bool alreadyAdded = outputIndices.ContainsByPredicate([&samples, &sample](const int32 existingIndex)
			{
				const FPartitionVisibilitySample& existing = samples[existingIndex];
				return
					existing.DungeonGenerateActor == sample.DungeonGenerateActor &&
					existing.GridLocation == sample.GridLocation &&
					existing.WorldLocation == sample.WorldLocation;
			}
		);
		if (!alreadyAdded)
			outputIndices.Add(sampleIndex);
	}
}

bool ADungeonMainLevelScriptActor::IsPartitionPairPotentiallyVisible(
	const int32 sourcePartitionIndex,
	const int32 targetPartitionIndex,
	const TArray<FPVSTraceContext>& traceContexts,
	const TArray<FPVSPartitionSamples>& partitionSamples,
	const bool validateOptimization,
	FPVSBuildStatistics& statistics) const
{
	if (!mPartitionVisibilitySamples.IsValidIndex(sourcePartitionIndex) || !mPartitionVisibilitySamples.IsValidIndex(targetPartitionIndex))
		return false;
	if (!partitionSamples.IsValidIndex(sourcePartitionIndex) || !partitionSamples.IsValidIndex(targetPartitionIndex))
		return false;

	const TArray<FPartitionVisibilitySample>& sourceSamples = mPartitionVisibilitySamples[sourcePartitionIndex];
	const TArray<FPartitionVisibilitySample>& targetSamples = mPartitionVisibilitySamples[targetPartitionIndex];
	if (sourceSamples.IsEmpty() || targetSamples.IsEmpty())
		return false;

	bool potentiallyVisible = false;
	for (const int32 sourceSampleIndex : partitionSamples[sourcePartitionIndex].SourceSampleIndices)
	{
		if (!sourceSamples.IsValidIndex(sourceSampleIndex))
			continue;
		const FPartitionVisibilitySample& sourceSample = sourceSamples[sourceSampleIndex];
		if (!traceContexts.IsValidIndex(sourceSample.TraceContextIndex))
			continue;
		const FPVSTraceContext& traceContext = traceContexts[sourceSample.TraceContextIndex];

		for (const int32 targetSampleIndex : partitionSamples[targetPartitionIndex].TargetSampleIndices)
		{
			if (!targetSamples.IsValidIndex(targetSampleIndex))
				continue;
			const FPartitionVisibilitySample& targetSample = targetSamples[targetSampleIndex];
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			++statistics.SamplePairAttemptCount;
#endif
			if (sourceSample.DungeonGenerateActor != targetSample.DungeonGenerateActor || sourceSample.TraceContextIndex != targetSample.TraceContextIndex)
				continue;
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			++statistics.TraceCallCount;
#endif
			const bool traceVisible = TracePartitionVisibility(sourceSample, targetSample, traceContext, &statistics);
#if WITH_EDITOR
			if (validateOptimization)
			{
				FPVSBuildStatistics referenceTraceStatistics;
				const bool referenceTraceVisible = TracePartitionVisibilityReference(sourceSample, targetSample, referenceTraceStatistics);
				ensureAlwaysMsgf(
					traceVisible == referenceTraceVisible,
					TEXT("PVS trace optimization mismatch for source partition %d, target partition %d, source sample %d, and target sample %d"),
					sourcePartitionIndex,
					targetPartitionIndex,
					sourceSampleIndex,
					targetSampleIndex
				);
			}
#endif
			if (traceVisible)
			{
				potentiallyVisible = true;
				break;
			}
		}
		if (potentiallyVisible)
			break;
	}

#if WITH_EDITOR
	if (validateOptimization)
	{
		bool referencePotentiallyVisible = false;
		FPVSBuildStatistics referenceStatistics;
		for (const FPartitionVisibilitySample& sourceSample : sourceSamples)
		{
			if (!IsVisibilitySampleUsableAsSource(sourceSample.SampleType))
				continue;
			for (const FPartitionVisibilitySample& targetSample : targetSamples)
			{
				if (sourceSample.DungeonGenerateActor != targetSample.DungeonGenerateActor)
					continue;
				if (TracePartitionVisibilityReference(sourceSample, targetSample, referenceStatistics))
				{
					referencePotentiallyVisible = true;
					break;
				}
			}
			if (referencePotentiallyVisible)
				break;
		}
		ensureAlwaysMsgf(
			potentiallyVisible == referencePotentiallyVisible,
			TEXT("PVS optimization mismatch for source partition %d and target partition %d"),
			sourcePartitionIndex,
			targetPartitionIndex
		);
	}
#else
	(void)validateOptimization;
#endif

	return potentiallyVisible;
}

bool ADungeonMainLevelScriptActor::TracePartitionVisibility(
	const FPartitionVisibilitySample& sourceSample,
	const FPartitionVisibilitySample& targetSample,
	const FPVSTraceContext& traceContext,
	FPVSBuildStatistics* statistics)
{
	if (!traceContext.IsReadyFor(sourceSample) || !traceContext.IsReadyFor(targetSample))
		return false;
	if (!traceContext.Voxel->Contain(sourceSample.GridLocation) || !traceContext.Voxel->Contain(targetSample.GridLocation))
		return false;

	const FVector sourceLocalLocation = sourceSample.WorldLocation - traceContext.ActorLocation;
	const FVector targetLocalLocation = targetSample.WorldLocation - traceContext.ActorLocation;
	return TraversePVSDDACells(
		sourceLocalLocation,
		targetLocalLocation,
		traceContext.GridSize,
		sourceSample.GridLocation,
		targetSample.GridLocation,
		[&traceContext, statistics](const FIntVector& currentCell, const FIntVector& nextCell)
		{
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
			if (statistics != nullptr)
				++statistics->TraceStepCount;
#endif
			return traceContext.Voxel->Contain(nextCell) && traceContext.CanTraverseDelta(currentCell, nextCell);
		}
	);
}


#if WITH_EDITOR
bool ADungeonMainLevelScriptActor::TracePartitionVisibilityReference(const FPartitionVisibilitySample& sourceSample, const FPartitionVisibilitySample& targetSample, FPVSBuildStatistics& statistics)
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

	auto isHorizontalTransitionOpen = [voxel](const FIntVector& fromCell, const FIntVector& toCell)
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
		return false;
	};

	auto isTransitionOpen = [voxel, &isHorizontalTransitionOpen](const FIntVector& fromCell, const FIntVector& toCell)
	{
		if (!voxel->Contain(fromCell) || !voxel->Contain(toCell))
			return false;

		const dungeon::Grid& fromGrid = voxel->Get(fromCell);
		const dungeon::Grid& toGrid = voxel->Get(toCell);
		if (!IsTraversableGrid(fromGrid) || !IsTraversableGrid(toGrid))
			return false;

		const FIntVector delta = toCell - fromCell;
		if (FMath::Abs(delta.X) + FMath::Abs(delta.Y) == 1 && delta.Z == 0)
			return isHorizontalTransitionOpen(fromCell, toCell);
		if (delta == FIntVector(0, 0, 1))
			return !fromGrid.HasCeiling() && !toGrid.HasFloor();
		if (delta == FIntVector(0, 0, -1))
			return !fromGrid.HasFloor() && !toGrid.HasCeiling();

		return false;
	};

	auto isSlopeDiagonalTransitionOpen = [voxel, &isHorizontalTransitionOpen](const FIntVector& startCell, const FIntVector& targetCell)
	{
		if (!voxel->Contain(startCell) || !voxel->Contain(targetCell))
			return false;

		const dungeon::Grid& startGrid = voxel->Get(startCell);
		const dungeon::Grid& targetGrid = voxel->Get(targetCell);
		if (!IsTraversableGrid(startGrid) || !IsTraversableGrid(targetGrid))
			return false;

		const FIntVector delta = targetCell - startCell;
		if (delta.Z == 0)
			return false;
		if (FMath::Abs(delta.X) > 1 || FMath::Abs(delta.Y) > 1 || FMath::Abs(delta.Z) > 1)
			return false;
		if (FMath::Abs(delta.X) + FMath::Abs(delta.Y) <= 0)
			return false;

		TArray<FIntVector, TInlineAllocator<6>> cellsToCheck;
		cellsToCheck.Add(startCell);
		cellsToCheck.Add(targetCell);

		FIntVector currentHorizontalCell = startCell;
		if (delta.X != 0)
		{
			const FIntVector nextHorizontalCell = currentHorizontalCell + FIntVector(delta.X > 0 ? 1 : -1, 0, 0);
			if (!isHorizontalTransitionOpen(currentHorizontalCell, nextHorizontalCell))
				return false;
			cellsToCheck.Add(nextHorizontalCell);
			cellsToCheck.Add(nextHorizontalCell + FIntVector(0, 0, delta.Z));
			currentHorizontalCell = nextHorizontalCell;
		}
		if (delta.Y != 0)
		{
			const FIntVector nextHorizontalCell = currentHorizontalCell + FIntVector(0, delta.Y > 0 ? 1 : -1, 0);
			if (!isHorizontalTransitionOpen(currentHorizontalCell, nextHorizontalCell))
				return false;
			cellsToCheck.Add(nextHorizontalCell);
			cellsToCheck.Add(nextHorizontalCell + FIntVector(0, 0, delta.Z));
		}

		for (const FIntVector& cell : cellsToCheck)
		{
			if (!voxel->Contain(cell))
				continue;

			const dungeon::Grid& grid = voxel->Get(cell);
			if (IsTraversableGrid(grid) && grid.IsKindOfSlopeType())
				return true;
		}

		return false;
	};

	auto canTraverseDelta = [&isTransitionOpen, &isSlopeDiagonalTransitionOpen](const FIntVector& startCell, const FIntVector& targetCell)
	{
		const FIntVector delta = targetCell - startCell;
		if (FMath::Abs(delta.X) > 1 || FMath::Abs(delta.Y) > 1 || FMath::Abs(delta.Z) > 1)
			return false;

		const int32 manhattanDistance = FMath::Abs(delta.X) + FMath::Abs(delta.Y) + FMath::Abs(delta.Z);
		if (manhattanDistance == 0)
			return true;
		if (manhattanDistance == 1)
			return isTransitionOpen(startCell, targetCell);
		if (isSlopeDiagonalTransitionOpen(startCell, targetCell))
			return true;

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
#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
		++statistics.TraceStepCount;
#endif
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
#endif

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
	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		check(IsValid(DungeonPartitions[partitionIndex]));
		SetPartitionActivationState(partitionIndex, true, true);
	}
	ResetPartitionTransitionQueue();
}

void ADungeonMainLevelScriptActor::ForceInactivate()
{
	ResetPartitionTransitionQueue();
	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		check(IsValid(DungeonPartitions[partitionIndex]));
		SetPartitionActivationState(partitionIndex, false, false);
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

/*
 * 管理対象のポイントライトとスポットライトの表示状態および影生成状態を更新します。
 */
void ADungeonMainLevelScriptActor::UpdatePointAndSpotLightStates()
{
	/*
	 * Uses the first local player's view as the reference for light selection.
	 * ライト選択の基準として、最初のローカルプレイヤーの視点を使用します。
	 */
	const APlayerController* playerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!IsValid(playerController))
		return;

	/*
	 * Captures the camera position and forward direction once for all light evaluations in this frame.
	 * このフレーム内の全ライト判定で共有するカメラ位置と正面方向を一度だけ取得します。
	 */
	FVector cameraLocation;
	FRotator cameraRotation;
	playerController->GetPlayerViewPoint(cameraLocation, cameraRotation);
	const FVector cameraDirection = cameraRotation.Vector();

	/*
	 * Resolves the grid identifier from the pawn position, not the camera position.
	 * カメラ位置ではなくPawn位置から、プレイヤーが所属するグリッドIdentifierを取得します。
	 */
	uint16 playerGridIdentifier = 0;
	bool hasPlayerGridIdentifier = false;
	if (const APawn* playerPawn = playerController->GetPawn())
		hasPlayerGridIdentifier = FindGridIdentifier(playerPawn->GetActorLocation(), playerGridIdentifier);

	/*
	 * Collects only visible light candidates; shadow casting is assigned after global priority sorting.
	 * 表示可能なライト候補だけを収集し、影生成は全候補の優先順位を確定した後に割り当てます。
	 */
	struct FLightCandidate final
	{
		FDungeonControlledPointAndSpotLight* ControlledLight = nullptr;
		float Score = 0.f;
		bool IdentifierMatches = false;
	};
	std::vector<FLightCandidate> lightCandidates;

	/*
	 * Evaluates every managed light grouped by its owning runtime partition and activator.
	 * 所属する実行時パーティションとActivator単位で、全ての管理対象ライトを判定します。
	 */
	for (const UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		const bool partitionActive = partition->IsPartitionActivate();
		partition->EachDungeonComponentActivatorComponent(
			[this, partitionActive, cameraLocation, cameraDirection, hasPlayerGridIdentifier, playerGridIdentifier, &lightCandidates](UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
			{
				/*
				 * Other activation reasons may keep the actor hidden even when its partition is active.
				 * パーティションがアクティブでも、他の無効化理由が残っていればActorの非表示状態を維持します。
				 */
				const bool activatorVisibilityEnabled = dungeonComponentActivatorComponent->mComponentVisibility.all();

				/*
				 * A matching identifier is the safety rule that bypasses facing-angle culling.
				 * Identifier一致は、正面角度カリングを回避するための安全判定として使用します。
				 */
				const bool identifierMatches =
					hasPlayerGridIdentifier &&
					dungeonComponentActivatorComponent->HasGridIdentifier() &&
					dungeonComponentActivatorComponent->GetGridIdentifier() == playerGridIdentifier;

				dungeonComponentActivatorComponent->EachControlledPointAndSpotLight(
					[this, dungeonComponentActivatorComponent, partitionActive, activatorVisibilityEnabled, identifierMatches, cameraLocation, cameraDirection, &lightCandidates](FDungeonControlledPointAndSpotLight& controlledLight)
					{
						UPointLightComponent* pointLightComponent = controlledLight.Component.Get();
						if (!IsValid(pointLightComponent))
							return;

						/*
						 * Fades out lights that cannot be shown while retaining shadows, before performing angular or score calculations.
						 * 表示できないライトは、角度やスコアを計算する前に影を維持したままフェードアウトします。
						 */
						if (!partitionActive || !controlledLight.InitialVisibility || !activatorVisibilityEnabled)
						{
							pointLightComponent->SetCastShadows(true);
							SetManagedPointOrSpotLightVisibility(pointLightComponent, false, PointAndSpotLightFadeInTime, PointAndSpotLightFadeOutTime);
							return;
						}

						/*
						 * The facing dot uses the activator's configured owner-local axis, independently of the light component's relative rotation.
						 * Light Componentの相対回転とは独立して、Activatorに設定された所有Actorのローカル軸を正面判定に使用します。
						 * The dot is 1 in front of the actor and -1 directly behind it; the previous state is kept between the on/off angles.
						 * 内積はActor正面で1、真後ろで-1になり、ON/OFF角度間では直前状態を維持します。
						 */
						const FVector fromLightToCamera = cameraLocation - pointLightComponent->GetComponentLocation();
						float facingDot = 1.f;
						if (const AActor* ownerActor = pointLightComponent->GetOwner(); IsValid(ownerActor))
						{
							facingDot = dungeon::LightStatePolicy::ComputeActorFacingDot(
								dungeonComponentActivatorComponent->GetManagedLightFacingDirection(ownerActor),
								fromLightToCamera
							);
						}
						controlledLight.EnabledByManager = dungeon::LightStatePolicy::ResolveManagerEnabled(
							identifierMatches,
							facingDot,
							controlledLight.EnabledByManager,
							PointAndSpotLightTurnOnAngle,
							PointAndSpotLightTurnOffAngle
						);

						/*
						 * Applies all visibility gates after updating the identifier and facing-angle state.
						 * Identifierと正面角度の状態更新後に、全ての表示条件をまとめて適用します。
						 */
						if (!dungeon::LightStatePolicy::ShouldShowLight(
							partitionActive,
							controlledLight.InitialVisibility,
							activatorVisibilityEnabled,
							controlledLight.EnabledByManager))
						{
							pointLightComponent->SetCastShadows(true);
							SetManagedPointOrSpotLightVisibility(pointLightComponent, false, PointAndSpotLightFadeInTime, PointAndSpotLightFadeOutTime);
							return;
						}

						/*
						 * Scores candidates with a linear facing factor so lights nearer the camera's forward direction receive stronger priority.
						 * カメラ正面に近いライトを強く優先しつつ、距離も考慮する線形スコアを計算します。
						 */
						const FVector toLight = pointLightComponent->GetComponentLocation() - cameraLocation;
						const float score = dungeon::LightStatePolicy::ComputeCameraPriorityScore(cameraDirection, toLight);
						lightCandidates.emplace_back(FLightCandidate{ &controlledLight, score, identifierMatches });
					}
				);
			}
		);
	}

	/*
	 * Sorts by camera-based score first, then favors identifier matches without disturbing complete ties.
	 * カメラ基準スコアを最優先にし、同スコアではIdentifier一致を優先する安定ソートを行います。
	 */
	std::stable_sort(lightCandidates.begin(), lightCandidates.end(), [](const FLightCandidate& left, const FLightCandidate& right)
		{
			return dungeon::LightStatePolicy::HasHigherPriority(
				left.IdentifierMatches,
				left.Score,
				right.IdentifierMatches,
				right.Score
			);
		}
	);

	/*
	 * Shows only the budgeted prefix with shadows; overflow candidates fade out while retaining shadows to prevent light leaks.
	 * 上限内の先頭候補だけ影付きで表示し、上限を超えた候補は光漏れを防ぐため影を維持したままフェードアウトします。
	 */
	const size_t shadowCastingCount = dungeon::LightStatePolicy::ResolveShadowCastingCount(
		MaxShadowCastingPointAndSpotLights,
		lightCandidates.size()
	);
	for (size_t index = 0; index < lightCandidates.size(); ++index)
	{
		if (UPointLightComponent* pointLightComponent = lightCandidates[index].ControlledLight->Component.Get())
		{
			const dungeon::LightStatePolicy::FShadowBudgetState state = dungeon::LightStatePolicy::ResolveShadowBudgetState(index, shadowCastingCount);
			pointLightComponent->SetCastShadows(state.CastShadows);
			SetManagedPointOrSpotLightVisibility(pointLightComponent, state.Visible, PointAndSpotLightFadeInTime, PointAndSpotLightFadeOutTime);
		}
	}
}

/*
 * Restores managed point and spot lights to their states recorded at BeginPlay.
 * 管理対象のポイントライトとスポットライトをBeginPlay時に記録した状態へ復元します。
 */
void ADungeonMainLevelScriptActor::RestorePointAndSpotLightStates()
{
	for (UDungeonPartition* partition : DungeonPartitions)
	{
		check(IsValid(partition));
		partition->EachDungeonComponentActivatorComponent([](UDungeonComponentActivatorComponent* dungeonComponentActivatorComponent)
			{
				dungeonComponentActivatorComponent->RestoreControlledPointAndSpotLightStates();
			}
		);
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

/**
 * Dispatches only the enabled debug visualization groups.
 * 有効なデバッグ可視化グループだけを描画します。
 */
void ADungeonMainLevelScriptActor::DrawDebugInformation() const
{
	if (ShowRuntimePartitionState)
		DrawDebugRuntimePartitionState();
	if (ShowPartitionConstruction)
		DrawDebugPartitionConstruction();
	if (ShowPVSResult)
		DrawDebugPVSResult();
	if (ShowPVSBuildSamples)
		DrawDebugPVSBuildSamples();
}

/**
 * Registers the editor-only canvas delegate used by the automatic debug legend.
 * 自動デバッグ凡例に使用するエディタ専用キャンバスデリゲートを登録します。
 */
void ADungeonMainLevelScriptActor::RegisterDebugLegend()
{
	if (mDebugLegendDelegateHandle.IsValid())
		return;

	mDebugLegendDelegateHandle = UDebugDrawService::Register(
		TEXT("OnScreenDebug"),
		FDebugDrawDelegate::CreateUObject(this, &ADungeonMainLevelScriptActor::DrawDebugLegend)
	);
	ensureMsgf(mDebugLegendDelegateHandle.IsValid(), TEXT("Failed to register the Dungeon Generator debug legend"));
}

/**
 * Unregisters the debug legend delegate before this actor leaves play.
 * このActorがPlayを終了する前にデバッグ凡例デリゲートを解除します。
 */
void ADungeonMainLevelScriptActor::UnregisterDebugLegend()
{
	if (!mDebugLegendDelegateHandle.IsValid())
		return;

	UDebugDrawService::Unregister(mDebugLegendDelegateHandle);
	mDebugLegendDelegateHandle = FDelegateHandle();
}

/**
 * Draws a fixed screen-space legend containing only the enabled debug groups.
 * 有効なデバッググループだけを含む固定スクリーン凡例を描画します。
 */
void ADungeonMainLevelScriptActor::DrawDebugLegend(UCanvas* canvas, APlayerController* playerController) const
{
	if (!IsValid(canvas))
		return;

	const UWorld* drawingWorld = IsValid(playerController) ? playerController->GetWorld() : nullptr;
	if (!IsValid(drawingWorld) &&
		canvas->SceneView != nullptr &&
		canvas->SceneView->Family != nullptr &&
		canvas->SceneView->Family->Scene != nullptr)
	{
		drawingWorld = canvas->SceneView->Family->Scene->GetWorld();
	}
	if (drawingWorld != GetWorld())
		return;
	if (!ShowRuntimePartitionState && !ShowPartitionConstruction && !ShowPVSResult && !ShowPVSBuildSamples)
		return;

	struct FLegendRow
	{
		FString Label;
		FColor Color = FColor::Transparent;
		bool IsSection = false;
	};

	TArray<FLegendRow, TInlineAllocator<32>> rows;
	auto addSection = [&rows](const TCHAR* label)
	{
		rows.Add({ label, FColor::Transparent, true });
	};
	auto addItem = [&rows](const TCHAR* label, const FColor& color)
	{
		rows.Add({ label, color, false });
	};

	if (ShowRuntimePartitionState)
	{
		addSection(TEXT("Runtime Partition State"));
		addItem(TEXT("Active / requested active"), DungeonDebugColors::RuntimeActive);
		addItem(TEXT("Active / pending deactivation"), DungeonDebugColors::RuntimePendingInactivation);
		addItem(TEXT("Inactive / pending activation"), DungeonDebugColors::RuntimePendingActivation);
		addItem(TEXT("Inactive"), DungeonDebugColors::RuntimeInactive);
		addItem(TEXT("Player partition"), DungeonDebugColors::PlayerPartition);
		addItem(TEXT("Player inside bounds without partition"), DungeonDebugColors::PlayerInsideBoundsWithoutPartition);
		addItem(TEXT("Player outside bounds"), DungeonDebugColors::PlayerOutsideBounds);
	}
	if (ShowPartitionConstruction)
	{
		addSection(TEXT("Partition Construction"));
		addItem(TEXT("Dungeon bounds"), DungeonDebugColors::ConstructionBounds);
		if (!ShowRuntimePartitionState)
			addItem(TEXT("Partition cell"), DungeonDebugColors::ConstructionCell);
	}
	if (ShowPVSResult)
	{
		addSection(TEXT("PVS Result"));
		addItem(TEXT("PVS-visible partition"), DungeonDebugColors::PVSVisiblePartition);
		if (!ShowRuntimePartitionState)
			addItem(TEXT("PVS source partition"), DungeonDebugColors::PlayerPartition);
	}
	if (ShowPVSBuildSamples)
	{
		addSection(TEXT("PVS Build Samples"));
		addItem(TEXT("Center sample"), DungeonDebugColors::SampleCenter);
		addItem(TEXT("Edge sample"), DungeonDebugColors::SampleEdge);
		addItem(TEXT("Visible roof sample"), DungeonDebugColors::SampleRoofVisible);
		addItem(TEXT("Non-visible roof candidate"), DungeonDebugColors::SampleRoofCandidate);
		addItem(TEXT("Slope lower sample"), DungeonDebugColors::SampleSlopeLower);
		addItem(TEXT("Slope upper sample"), DungeonDebugColors::SampleSlopeUpper);
	}

	constexpr float OriginX = 20.f;
	constexpr float OriginY = 70.f;
	constexpr float MaximumPanelWidth = 500.f;
	constexpr float MinimumPanelWidth = 260.f;
	constexpr float Padding = 10.f;
	constexpr float TitleHeight = 24.f;
	constexpr float RowHeight = 18.f;
	constexpr float SwatchSize = 11.f;
	const float panelWidth = FMath::Min(MaximumPanelWidth, FMath::Max(MinimumPanelWidth, canvas->ClipX - OriginX * 2.f));
	const float panelHeight = Padding * 2.f + TitleHeight + RowHeight * static_cast<float>(rows.Num());
	const FVector2D panelPosition(OriginX, OriginY);
	const FVector2D panelSize(panelWidth, panelHeight);

	canvas->K2_DrawTexture(
		nullptr,
		panelPosition,
		panelSize,
		FVector2D::ZeroVector,
		FVector2D::UnitVector,
		FLinearColor(0.f, 0.f, 0.f, 0.78f)
	);
	canvas->K2_DrawBox(panelPosition, panelSize, 1.f, FLinearColor(0.7f, 0.7f, 0.7f, 0.9f));

	UFont* font = GEngine != nullptr ? GEngine->GetSmallFont() : nullptr;
	const FVector2D textScale(0.9f, 0.9f);
	float rowY = OriginY + Padding;
	canvas->K2_DrawText(
		font,
		TEXT("Dungeon Generator Debug Legend"),
		FVector2D(OriginX + Padding, rowY),
		textScale,
		FLinearColor::White,
		0.f,
		FLinearColor::Black,
		FVector2D(1.f, 1.f),
		false,
		false,
		true,
		FLinearColor::Black
	);
	rowY += TitleHeight;

	for (const FLegendRow& row : rows)
	{
		if (row.IsSection)
		{
			canvas->K2_DrawText(
				font,
				row.Label,
				FVector2D(OriginX + Padding, rowY),
				textScale,
				FLinearColor(0.8f, 0.85f, 1.f, 1.f),
				0.f,
				FLinearColor::Black,
				FVector2D(1.f, 1.f),
				false,
				false,
				true,
				FLinearColor::Black
			);
		}
		else
		{
			const FVector2D swatchPosition(OriginX + Padding + 4.f, rowY + (RowHeight - SwatchSize) * 0.5f);
			const FVector2D swatchSize(SwatchSize, SwatchSize);
			canvas->K2_DrawTexture(
				nullptr,
				swatchPosition,
				swatchSize,
				FVector2D::ZeroVector,
				FVector2D::UnitVector,
				FLinearColor(row.Color)
			);
			canvas->K2_DrawBox(swatchPosition, swatchSize, 1.f, FLinearColor::White);
			canvas->K2_DrawText(
				font,
				row.Label,
				FVector2D(OriginX + Padding + 24.f, rowY),
				textScale,
				FLinearColor::White,
				0.f,
				FLinearColor::Black,
				FVector2D(1.f, 1.f),
				false,
				false,
				false,
				FLinearColor::Black
			);
		}
		rowY += RowHeight;
	}
}

/**
 * Draws partition geometry and distinguishes actual and requested activation states.
 * パーティション形状を描画し、実際のアクティブ状態と要求状態を区別します。
 */
void ADungeonMainLevelScriptActor::DrawDebugRuntimePartitionState() const
{
	constexpr double Margin = 10;
	auto drawPartitionBox = [this, Margin](const int32 partitionIndex, const FColor& color, const float thickness)
	{
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !IsValid(DungeonPartitions[partitionIndex]))
			return;

		const FBox& bounds = DungeonPartitions[partitionIndex]->GetBounds();
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounds.GetCenter(),
			bounds.GetExtent() - FVector(Margin),
			color,
			FRotator::ZeroRotator,
			0.f,
			thickness
		);
	};

	for (int32 partitionIndex = 0; partitionIndex < DungeonPartitions.Num(); ++partitionIndex)
	{
		const UDungeonPartition* partition = DungeonPartitions[partitionIndex];
		if (!IsValid(partition))
			continue;

		const bool actualActive = partition->IsPartitionActivate();
		const bool requestedActive = mDesiredPartitionActivation.IsValidIndex(partitionIndex) ?
			mDesiredPartitionActivation[partitionIndex] != 0 :
			actualActive;

		FColor stateColor;
		if (actualActive)
			stateColor = requestedActive ? DungeonDebugColors::RuntimeActive : DungeonDebugColors::RuntimePendingInactivation;
		else
			stateColor = requestedActive ? DungeonDebugColors::RuntimePendingActivation : DungeonDebugColors::RuntimeInactive;

		drawPartitionBox(partitionIndex, stateColor, actualActive != requestedActive ? dungeon::BoldThickness : dungeon::ThinThickness);
	}

	const UWorld* world = GetWorld();
	if (!IsValid(world))
		return;

	for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
	{
		const APlayerController* playerController = iterator->Get();
		if (!IsValid(playerController))
			continue;

		const APawn* playerPawn = playerController->GetPawn();
		if (!IsValid(playerPawn) || !playerPawn->IsPlayerControlled())
			continue;

		const FVector pawnLocation = playerPawn->GetActorLocation();
		const int32 currentPartitionIndex = FindPartitionIndex(pawnLocation);
		if (DungeonPartitions.IsValidIndex(currentPartitionIndex))
		{
			drawPartitionBox(currentPartitionIndex, DungeonDebugColors::PlayerPartition, dungeon::BoldThickness);
		}
		else
		{
			const bool insideBounding = mBounding.IsValid && mBounding.IsInsideOrOn(pawnLocation);
			UKismetSystemLibrary::DrawDebugSphere(
				GetWorld(),
				pawnLocation,
				30.f,
				16,
				insideBounding ? DungeonDebugColors::PlayerInsideBoundsWithoutPartition : DungeonDebugColors::PlayerOutsideBounds,
				0.f,
				4.f
			);
		}
	}
}

/**
 * Draws the world bounds and neutral cell structure used to construct partitions.
 * パーティション構築に使用したワールド境界とセル構造を描画します。
 */
void ADungeonMainLevelScriptActor::DrawDebugPartitionConstruction() const
{
	if (mBounding.IsValid)
	{
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			mBounding.GetCenter(),
			mBounding.GetExtent(),
			DungeonDebugColors::ConstructionBounds,
			FRotator::ZeroRotator,
			0.f,
			dungeon::ThinThickness
		);
	}

	if (ShowRuntimePartitionState)
		return;

	for (const UDungeonPartition* partition : DungeonPartitions)
	{
		constexpr double Margin = 10;
		if (!IsValid(partition))
			continue;

		const FBox& bounds = partition->GetBounds();
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounds.GetCenter(),
			bounds.GetExtent() - FVector(Margin),
			DungeonDebugColors::ConstructionCell,
			FRotator::ZeroRotator,
			0.f,
			dungeon::ThinThickness
		);
	}
}

/**
 * Draws the union of PVS partitions for all current players without duplicate boxes.
 * 現在の全プレイヤーのPVSパーティションを重複せずに描画します。
 */
void ADungeonMainLevelScriptActor::DrawDebugPVSResult() const
{
	const UWorld* world = GetWorld();
	if (!IsValid(world) || !HasPrecomputedPartitionVisibility())
		return;

	TSet<int32> sourcePartitionIndices;
	TSet<int32> visiblePartitionIndices;
	for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
	{
		const APlayerController* playerController = iterator->Get();
		if (!IsValid(playerController))
			continue;

		const APawn* playerPawn = playerController->GetPawn();
		if (!IsValid(playerPawn) || !playerPawn->IsPlayerControlled())
			continue;

		const int32 sourcePartitionIndex = FindPartitionIndex(playerPawn->GetActorLocation());
		if (!DungeonPartitions.IsValidIndex(sourcePartitionIndex))
			continue;

		sourcePartitionIndices.Add(sourcePartitionIndex);
		for (int32 targetPartitionIndex = 0; targetPartitionIndex < DungeonPartitions.Num(); ++targetPartitionIndex)
		{
			if (IsPrecomputedPartitionVisible(sourcePartitionIndex, targetPartitionIndex))
				visiblePartitionIndices.Add(targetPartitionIndex);
		}
	}

	constexpr double Margin = 10;
	auto drawPartitionBox = [this, Margin](const int32 partitionIndex, const FColor& color, const float thickness)
	{
		if (!DungeonPartitions.IsValidIndex(partitionIndex) || !IsValid(DungeonPartitions[partitionIndex]))
			return;

		const FBox& bounds = DungeonPartitions[partitionIndex]->GetBounds();
		UKismetSystemLibrary::DrawDebugBox(
			GetWorld(),
			bounds.GetCenter(),
			bounds.GetExtent() - FVector(Margin),
			color,
			FRotator::ZeroRotator,
			0.f,
			thickness
		);
	};

	for (const int32 partitionIndex : visiblePartitionIndices)
		drawPartitionBox(partitionIndex, DungeonDebugColors::PVSVisiblePartition, dungeon::ThinThickness);

	if (!ShowRuntimePartitionState)
	{
		for (const int32 partitionIndex : sourcePartitionIndices)
			drawPartitionBox(partitionIndex, DungeonDebugColors::PlayerPartition, dungeon::BoldThickness);
	}
}

/**
 * Draws the detailed source and roof samples used to build the PVS.
 * PVS構築に使用した詳細な始点サンプルと屋根サンプルを描画します。
 */
void ADungeonMainLevelScriptActor::DrawDebugPVSBuildSamples() const
{
	const UWorld* world = GetWorld();
	if (!IsValid(world))
		return;

	auto getSampleColor = [](const EPartitionVisibilitySampleType sampleType)
	{
		switch (sampleType)
		{
		case EPartitionVisibilitySampleType::Center:
			return DungeonDebugColors::SampleCenter;
		case EPartitionVisibilitySampleType::Edge:
			return DungeonDebugColors::SampleEdge;
		case EPartitionVisibilitySampleType::Roof:
			return DungeonDebugColors::SampleRoofVisible;
		case EPartitionVisibilitySampleType::SlopeLower:
			return DungeonDebugColors::SampleSlopeLower;
		case EPartitionVisibilitySampleType::SlopeUpper:
			return DungeonDebugColors::SampleSlopeUpper;
		default:
			return DungeonDebugColors::SampleCenter;
		}
	};

	TSet<int32> sourcePartitionIndices;
	for (FConstPlayerControllerIterator iterator = world->GetPlayerControllerIterator(); iterator; ++iterator)
	{
		const APlayerController* playerController = iterator->Get();
		if (!IsValid(playerController))
			continue;

		const APawn* playerPawn = playerController->GetPawn();
		if (!IsValid(playerPawn) || !playerPawn->IsPlayerControlled())
			continue;

		const int32 sourcePartitionIndex = FindPartitionIndex(playerPawn->GetActorLocation());
		if (DungeonPartitions.IsValidIndex(sourcePartitionIndex))
			sourcePartitionIndices.Add(sourcePartitionIndex);
	}

	for (const int32 sourcePartitionIndex : sourcePartitionIndices)
	{
		if (!mPartitionVisibilitySamples.IsValidIndex(sourcePartitionIndex))
			continue;

		for (const FPartitionVisibilitySample& sample : mPartitionVisibilitySamples[sourcePartitionIndex])
		{
			if (sample.SampleType == EPartitionVisibilitySampleType::Roof)
				continue;

			UKismetSystemLibrary::DrawDebugSphere(
				GetWorld(),
				sample.WorldLocation,
				18.f,
				12,
				getSampleColor(sample.SampleType),
				0.f,
				3.f
			);
		}
	}

	const double maximumVisibilityDistanceSquared =
		mTheoreticalMaxVisibilityDistance > 0.f ?
		FMath::Square(static_cast<double>(mTheoreticalMaxVisibilityDistance)) :
		TNumericLimits<double>::Max();
	TSet<int32> candidateRoofPartitionIndices;
	TSet<int32> visibleRoofPartitionIndices;
	for (const int32 sourcePartitionIndex : sourcePartitionIndices)
	{
		const UDungeonPartition* sourcePartition = DungeonPartitions[sourcePartitionIndex];
		if (!IsValid(sourcePartition))
			continue;

		for (int32 targetPartitionIndex = 0; targetPartitionIndex < DungeonPartitions.Num(); ++targetPartitionIndex)
		{
			if (!mPartitionVisibilitySamples.IsValidIndex(targetPartitionIndex) || !IsValid(DungeonPartitions[targetPartitionIndex]))
				continue;
			if (mPartitionConnectedComponents.IsValidIndex(sourcePartitionIndex) &&
				mPartitionConnectedComponents.IsValidIndex(targetPartitionIndex) &&
				mPartitionConnectedComponents[sourcePartitionIndex] != mPartitionConnectedComponents[targetPartitionIndex])
			{
				continue;
			}
			if (ComputeSquaredDistanceBetweenBounds(sourcePartition->GetBounds(), DungeonPartitions[targetPartitionIndex]->GetBounds()) > maximumVisibilityDistanceSquared)
				continue;

			candidateRoofPartitionIndices.Add(targetPartitionIndex);
			if (IsPrecomputedPartitionVisible(sourcePartitionIndex, targetPartitionIndex))
				visibleRoofPartitionIndices.Add(targetPartitionIndex);
		}
	}

	for (const int32 targetPartitionIndex : candidateRoofPartitionIndices)
	{
		const bool potentiallyVisible = visibleRoofPartitionIndices.Contains(targetPartitionIndex);
		const FColor roofSampleColor = potentiallyVisible ? DungeonDebugColors::SampleRoofVisible : DungeonDebugColors::SampleRoofCandidate;
		for (const FPartitionVisibilitySample& sample : mPartitionVisibilitySamples[targetPartitionIndex])
		{
			if (sample.SampleType != EPartitionVisibilitySampleType::Roof)
				continue;

			UKismetSystemLibrary::DrawDebugSphere(
				GetWorld(),
				sample.WorldLocation,
				potentiallyVisible ? 12.f : 8.f,
				8,
				roofSampleColor,
				0.f,
				potentiallyVisible ? 2.5f : 1.5f
			);
		}
	}
}

#endif
