/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "Parameter/Selector/DungeonDepthFromStartMeshSetSelector.h"
#include "Parameter/Selector/DungeonIdentifierMeshSetSelector.h"
#include "Parameter/Selector/DungeonUniformRandomMeshSetSelector.h"
#include "Core/Debug/Debug.h"
#include "Core/Math/Random.h"
#include <UObject/Package.h>

namespace Dungeon::MeshSet
{
	/*
	 * 同期乱数ストリームがない場合に決定論的なフォールバックインデックスを選択します。
	 */
	int32 SelectFallbackIndex(const int32 SeedKey, const int32 NumCandidates)
	{
		if (NumCandidates <= 0)
			return INDEX_NONE;
		return FMath::Abs(SeedKey) % NumCandidates;
	}

	/*
	 * 利用可能な場合は同期乱数ストリームを使用して一様なインデックスを選択します。
	 */
	int32 SelectUniformRandomIndex(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates)
	{
		if (NumCandidates <= 0)
			return INDEX_NONE;
		if (Random != nullptr)
			return Random->Get<int32_t>(NumCandidates);
		return SelectFallbackIndex(Query.SeedKey, NumCandidates);
	}
}

int32 UDungeonMeshSetSelectorBase::SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, const int32 NumCandidates) const
{
	return Dungeon::MeshSet::SelectUniformRandomIndex(Query, Random, NumCandidates);
}

UDungeonMeshSetSelectorBase* UDungeonMeshSetSelectorBase::CreateFromLegacyMethod(UObject* Outer, const EDungeonMeshSetSelectionMethod Method, UDungeonMeshSetSelectorBase* CustomSelector)
{
	UObject* selectorOuter = Outer != nullptr ? Outer : static_cast<UObject*>(GetTransientPackage());

	switch (Method)
	{
	case EDungeonMeshSetSelectionMethod::Identifier:
		return NewObject<UDungeonIdentifierMeshSetSelector>(selectorOuter);

	case EDungeonMeshSetSelectionMethod::DepthFromStart:
		return NewObject<UDungeonDepthFromStartMeshSetSelector>(selectorOuter);

	case EDungeonMeshSetSelectionMethod::Custom:
		if (IsValid(CustomSelector))
			return CustomSelector;

		DUNGEON_GENERATOR_WARNING(TEXT("Legacy Custom mesh-set selector could not be migrated because no mesh-set selector object was assigned. Falling back to Uniform Random."));
		return NewObject<UDungeonUniformRandomMeshSetSelector>(selectorOuter);

	case EDungeonMeshSetSelectionMethod::Random:
	default:
		return NewObject<UDungeonUniformRandomMeshSetSelector>(selectorOuter);
	}
}
