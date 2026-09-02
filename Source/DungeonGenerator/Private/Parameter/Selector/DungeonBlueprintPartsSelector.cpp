/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonBlueprintPartsSelector.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"

namespace
{
	/*
	 * 無効な Blueprint 結果に対して一様なフォールバックインデックスを選択します。
	 */
	int32 SelectUniformRandomIndex(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates)
	{
		if (NumCandidates <= 0)
			return INDEX_NONE;
		if (Random != nullptr)
			return Random->Get<int32_t>(NumCandidates);
		return FMath::Abs(Query.SeedKey) % NumCandidates;
	}
}

int32 UDungeonBlueprintPartsSelector::SelectPartsIndex_Implementation(const FDungeonPartsQuery& Query, const int32 NumCandidates) const
{
	(void)Query;
	(void)NumCandidates;
	return INDEX_NONE;
}

int32 UDungeonBlueprintPartsSelector::SelectPartsIndexNative(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	const int32 customIndex = SelectPartsIndex(Query, NumCandidates);
	if (0 <= customIndex && customIndex < NumCandidates)
		return customIndex;

	static int32 InvalidSelectionWarningCount = 0;
	if (InvalidSelectionWarningCount < 8)
	{
		++InvalidSelectionWarningCount;
		DUNGEON_GENERATOR_WARNING(TEXT("Parts selector returned invalid index %d (NumCandidates=%d). Falling back to uniform random selection. (%d/8)"), customIndex, NumCandidates, InvalidSelectionWarningCount);
	}

	return SelectUniformRandomIndex(Query, Random, NumCandidates);
}
