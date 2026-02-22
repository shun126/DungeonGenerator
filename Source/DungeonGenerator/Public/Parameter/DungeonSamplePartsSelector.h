/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "Parameter/DungeonPartsSelector.h"
#include "DungeonSamplePartsSelector.generated.h"


/*
 * C++ sample custom selector for plugin users.
 * Demonstrates:
 * - MeshSet selection by deterministic weighted lottery (SeedKey)
 * - Mesh parts selection by NeighborMask6
 * - Deterministic weighted lottery using SeedKey
 *
 * プラグイン利用者向けの C++ カスタムセレクタサンプルです。
 * 以下を実演します:
 * - SeedKey による決定的な重み付き MeshSet 選択
 * - NeighborMask6 によるメッシュパーツ選択
 * - SeedKey を用いた決定的な重み付き抽選
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonSamplePartsSelector : public UDungeonPartsSelector
{
	GENERATED_BODY()

};
