/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "DungeonIdentifierMeshSetSelector.generated.h"

/*
 * Identifier mesh-set selector.
 * 識別子でメッシュセットを選択するセレクターです。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonIdentifierMeshSetSelector : public UDungeonMeshSetSelectorBase
{
	GENERATED_BODY()

public:
	virtual int32 SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const override;
};
