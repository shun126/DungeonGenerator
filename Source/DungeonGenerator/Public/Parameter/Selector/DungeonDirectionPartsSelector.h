/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/Selector/DungeonPartsSelectorBase.h"
#include "DungeonDirectionPartsSelector.generated.h"

/**
 * Direction parts selector.
 * 方向でパーツを選択するセレクターです。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonDirectionPartsSelector : public UDungeonPartsSelectorBase
{
	GENERATED_BODY()

public:
	virtual int32 SelectPartsIndexNative(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const override;
};
