/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/Selector/DungeonPartsSelectorBase.h"
#include "DungeonBlueprintPartsSelector.generated.h"

/*
 * Blueprint-extensible parts selector.
 * Blueprint で拡張できるパーツセレクターです。
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonBlueprintPartsSelector : public UDungeonPartsSelectorBase
{
	GENERATED_BODY()

public:
	/*
	 * Selects an index from parts candidates in Blueprint.
	 * Blueprint でパーツ候補からインデックスを選択します。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "DungeonGenerator|Advanced", meta = (DisplayName = "Select Parts Index", ToolTip = "Called for parts selection. Return an index in [0, NumCandidates). Returning an out-of-range value falls back to uniform random selection. Use Query.SeedKey for deterministic Blueprint logic."))
	int32 SelectPartsIndex(const FDungeonPartsQuery& Query, int32 NumCandidates) const;

	virtual int32 SelectPartsIndexNative(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const override;
};
