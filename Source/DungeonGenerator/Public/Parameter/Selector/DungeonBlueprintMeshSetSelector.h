/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/Selector/DungeonMeshSetSelectorBase.h"
#include "DungeonBlueprintMeshSetSelector.generated.h"

/*
 * Blueprint-extensible mesh-set selector.
 * Blueprint で拡張できるメッシュセットセレクターです。
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonBlueprintMeshSetSelector : public UDungeonMeshSetSelectorBase
{
	GENERATED_BODY()

public:
	/*
	 * Selects an index from mesh-set candidates in Blueprint.
	 * Blueprint でメッシュセット候補からインデックスを選択します。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "DungeonGenerator|Advanced", meta = (DisplayName = "Select Mesh Set Index", ToolTip = "Called for mesh-set selection. Return an index in [0, NumCandidates). Returning an out-of-range value falls back to uniform random selection. Use Query.SeedKey for deterministic Blueprint logic."))
	int32 SelectMeshSetIndex(const FDungeonMeshSetQuery& Query, int32 NumCandidates) const;

	virtual int32 SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const override;
};
