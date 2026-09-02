/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "DungeonDepthFromStartMeshSetSelector.generated.h"

/**
 * Depth-from-start mesh-set selector.
 * スタートからの深さでメッシュセットを選択するセレクターです。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonDepthFromStartMeshSetSelector : public UDungeonMeshSetSelectorBase
{
	GENERATED_BODY()

public:
	virtual int32 SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const override;
};
