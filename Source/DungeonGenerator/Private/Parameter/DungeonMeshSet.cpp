/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/DungeonMeshSet.h"
#include "Parameter/DungeonActorParts.h"
#include "Parameter/DungeonRandomActorParts.h"
#include "Parameter/DungeonSelectionPolicyUtility.h"
#include "Core/Debug/Debug.h"
#include "Core/Helper/Direction.h"
#include "Core/Math/Random.h"
#include "Core/Voxelization/Grid.h"
#include <UObject/Package.h>

namespace
{
	namespace meshSet
	{
		/*
		 * Builds the lightweight query passed to parts selectors.
		 * パーツセレクターへ渡す軽量クエリを構築します。
		 */
		FDungeonPartsQuery MakePartsSelectionQuery(const EDungeonPartsSelectorTarget target, const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const uint8 neighborMask6)
		{
			FDungeonPartsQuery query;
			query.Target = target;
			query.PieceType = static_cast<uint8>(grid.GetType());
			query.Rotation = static_cast<uint8>(grid.GetDirection().Get());
			query.NeighborMask6 = neighborMask6;
			query.GridX = gridLocation.X;
			query.GridY = gridLocation.Y;
			query.GridZ = gridLocation.Z;
			query.RoomId = static_cast<int32>(grid.GetIdentifier());
			query.RoomStructuralRole = grid.GetRoomStructuralRole();
			query.RoomGameplayRole = grid.GetRoomGameplayRole();
			query.ZoneIndex = grid.GetZoneIndex();
			query.DepthFromStart = static_cast<float>(grid.GetDepthRatioFromStart()) / 255.f;
			query.DistanceToGoal = 1.f - query.DepthFromStart;
			query.SeedKey = static_cast<int32>(gridIndex);
			return query;
		}

		/*
		 * Converts legacy policy data to the closest legacy method used by selector migration.
		 * selector 移行で使用するため、旧 policy データを最も近い旧 method に変換します。
		 */
		EDungeonPartsSelectionMethod ResolveLegacyMethod(const EDungeonSelectionPolicy policy, const EDungeonPartsSelectionMethod method)
		{
			if (method != EDungeonPartsSelectionMethod::Random)
				return method;

			return dungeon::selection::ToLegacyPartsMethod(policy);
		}

		/*
		 * Ensures a selector exists, creating a built-in selector from legacy fields when needed.
		 * 必要に応じて旧フィールドから組み込みセレクターを作成し、セレクターの存在を保証します。
		 */
		void EnsurePartsSelector(UObject* outer, TObjectPtr<UDungeonPartsSelectorBase>& selector, const EDungeonSelectionPolicy policy, const EDungeonPartsSelectionMethod method, UDungeonPartsSelectorBase* customSelector)
		{
			if (IsValid(selector))
				return;

			selector = UDungeonPartsSelectorBase::CreateFromLegacyMethod(outer, ResolveLegacyMethod(policy, method), customSelector);
		}

	}
}

const FDungeonMeshPartsWithDirection* FDungeonMeshSet::SelectFloorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridLocation, gridIndex, grid, random, FloorParts, FloorPartsSelector, EDungeonPartsSelectorTarget::Floor, neighborMask6);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectWallPartsByGrid(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridLocation, gridIndex, grid, random, WallParts, WallPartsSelector, EDungeonPartsSelectorTarget::Wall, neighborMask6);
}

const FDungeonMeshPartsWithDirection* FDungeonMeshSet::SelectRoofParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridLocation, gridIndex, grid, random, RoofParts, RoofPartsSelector, EDungeonPartsSelectorTarget::Roof, neighborMask6);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectSlopeParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridLocation, gridIndex, grid, random, SlopeParts, SlopePartsSelector, EDungeonPartsSelectorTarget::Slope, neighborMask6);
}

const FDungeonMeshParts* FDungeonMeshSet::SelectCatwalkParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectPartsByGrid(gridLocation, gridIndex, grid, random, CatwalkParts, CatwalkPartsSelector, EDungeonPartsSelectorTarget::Catwalk, neighborMask6);
}

const FDungeonRandomActorParts* FDungeonMeshSet::SelectChandelierParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const uint8 neighborMask6) const
{
	return SelectRandomActorParts(gridLocation, gridIndex, grid, random, ChandelierParts, ChandelierPartsSelector, EDungeonPartsSelectorTarget::Chandelier, neighborMask6);
}

void FDungeonMeshSet::MigrateSelectionPolicies(UObject* Outer)
{
	UObject* selectorOuter = Outer != nullptr ? Outer : static_cast<UObject*>(GetTransientPackage());

	meshSet::EnsurePartsSelector(selectorOuter, FloorPartsSelector, FloorPartsSelectionPolicy, FloorPartsSelectionMethod, DungeonPartsSelector);
	meshSet::EnsurePartsSelector(selectorOuter, WallPartsSelector, WallPartsSelectionPolicy, WallPartsSelectionMethod, DungeonPartsSelector);
	meshSet::EnsurePartsSelector(selectorOuter, RoofPartsSelector, RoofPartsSelectionPolicy, RoofPartsSelectionMethod, DungeonPartsSelector);
	meshSet::EnsurePartsSelector(selectorOuter, SlopePartsSelector, SlopePartsSelectionPolicy, SloopPartsSelectionMethod, DungeonPartsSelector);
	meshSet::EnsurePartsSelector(selectorOuter, ChandelierPartsSelector, ChandelierPartsSelectionPolicy, ChandelierPartsSelectionMethod, DungeonPartsSelector);
	meshSet::EnsurePartsSelector(selectorOuter, CatwalkPartsSelector, CatwalkPartsSelectionPolicy, CatwalkPartsSelectionMethod, DungeonPartsSelector);

	FloorPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(FloorPartsSelectionPolicy);
	FloorPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(FloorPartsSelectionPolicy);
	WallPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(WallPartsSelectionPolicy);
	WallPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(WallPartsSelectionPolicy);
	RoofPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(RoofPartsSelectionPolicy);
	RoofPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(RoofPartsSelectionPolicy);
	SlopePartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(SlopePartsSelectionPolicy);
	SloopPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(SlopePartsSelectionPolicy);
	ChandelierPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(ChandelierPartsSelectionPolicy);
	ChandelierPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(ChandelierPartsSelectionPolicy);
	CatwalkPartsSelectionPolicy = dungeon::selection::SanitizePartsPolicy(CatwalkPartsSelectionPolicy);
	CatwalkPartsSelectionMethod = dungeon::selection::ToLegacyPartsMethod(CatwalkPartsSelectionPolicy);

	bSelectionPoliciesMigrated = true;
}

int32 FDungeonMeshSet::SelectDungeonMeshPartsIndexBySelector(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const UDungeonPartsSelectorBase* selector, const EDungeonPartsSelectorTarget target, const uint8 neighborMask6)
{
	if (size <= 0)
		return INDEX_NONE;

	const FDungeonPartsQuery query = meshSet::MakePartsSelectionQuery(target, gridLocation, gridIndex, grid, neighborMask6);
	if (IsValid(selector))
	{
		const int32 index = selector->SelectPartsIndexNative(query, random, size);
		if (0 <= index && index < size)
			return index;
	}

	if (random != nullptr)
		return random->Get<int32_t>(size);

	return static_cast<int32>(gridIndex % size);
}

int32 FDungeonMeshSet::SelectDungeonMeshPartsIndexByFace(const FIntVector& gridLocation, const dungeon::Direction& direction, const int32 size)
{
	const int32 offset = gridLocation.Z & 1;
	if (direction.IsNorthSouth())
		return (gridLocation.X + offset) % size;
	return (gridLocation.Y + offset) % size;
}

FDungeonActorParts* FDungeonMeshSet::SelectActorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const UDungeonPartsSelectorBase* selector, const EDungeonPartsSelectorTarget target, const uint8 neighborMask6)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndexBySelector(gridLocation, gridIndex, grid, random, size, selector, target, neighborMask6);
	FDungeonActorParts* actorParts = const_cast<FDungeonActorParts*>(&parts[index]);
	return IsValid(actorParts->ActorClass) ? actorParts : nullptr;
}

FDungeonRandomActorParts* FDungeonMeshSet::SelectRandomActorParts(const FIntVector& gridLocation, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const UDungeonPartsSelectorBase* selector, const EDungeonPartsSelectorTarget target, const uint8 neighborMask6)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndexBySelector(gridLocation, gridIndex, grid, random, size, selector, target, neighborMask6);
	FDungeonRandomActorParts* actorParts = const_cast<FDungeonRandomActorParts*>(&parts[index]);
	if (!IsValid(actorParts->ActorClass))
		return nullptr;

	if (random != nullptr)
	{
		if (actorParts->SpawnChance <= 0.f ||
			(actorParts->SpawnChance < 100.f && random->Get<float>(100.f) >= actorParts->SpawnChance))
			return nullptr;
	}

	return actorParts;
}
