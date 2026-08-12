/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonBlueprintMeshSetSelector.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"

namespace
{
	/*
	 * 無効な Blueprint 結果に対して一様なフォールバックインデックスを選択します。
	 */
	int32 SelectUniformRandomIndex(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates)
	{
		if (NumCandidates <= 0)
			return INDEX_NONE;
		if (Random != nullptr)
			return Random->Get<int32_t>(NumCandidates);
		return FMath::Abs(Query.SeedKey) % NumCandidates;
	}
}

int32 UDungeonBlueprintMeshSetSelector::SelectMeshSetIndex_Implementation(const FDungeonMeshSetQuery& Query, const int32 NumCandidates) const
{
	(void)Query;
	(void)NumCandidates;
	return INDEX_NONE;
}

int32 UDungeonBlueprintMeshSetSelector::SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	const int32 customIndex = SelectMeshSetIndex(Query, NumCandidates);
	if (0 <= customIndex && customIndex < NumCandidates)
		return customIndex;

	static int32 InvalidSelectionWarningCount = 0;
	if (InvalidSelectionWarningCount < 8)
	{
		++InvalidSelectionWarningCount;
		DUNGEON_GENERATOR_WARNING(TEXT("Mesh-set selector returned invalid index %d (NumCandidates=%d). Falling back to uniform random selection. (%d/8)"), customIndex, NumCandidates, InvalidSelectionWarningCount);
	}

	return SelectUniformRandomIndex(Query, Random, NumCandidates);
}
