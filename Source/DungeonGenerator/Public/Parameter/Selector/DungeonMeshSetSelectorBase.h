/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonMeshSetSelectionMethod.h"
#include "Parameter/DungeonSelectionQuery.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMeshSetSelectorBase.generated.h"

namespace dungeon
{
	class Random;
}

/*
 * Base class for C++ mesh-set selectors that can use the synchronized dungeon random stream.
 * 同期済みのダンジョン乱数ストリームを使用できる C++ メッシュセットセレクターの基底クラスです。
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonMeshSetSelectorBase : public UObject
{
	GENERATED_BODY()

public:
	/*
	 * Selects an index from mesh-set candidates using synchronized generation context.
	 * 同期された生成コンテキストを使用してメッシュセット候補からインデックスを選択します。
	 */
	virtual int32 SelectMeshSetIndexNative(const FDungeonMeshSetQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const;

	/*
	 * Creates a selector matching a legacy v1 mesh-set selection method for v2.0.0 migration only.
	 * Do not use this helper for new v2 code; assign a selector object directly instead.
	 * v2.0.0 の移行専用に、v1 の旧メッシュセット選択方式に対応するセレクターを生成します。
	 * 新しい v2 コードでは使用せず、セレクターオブジェクトを直接設定してください。
	 */
	static UDungeonMeshSetSelectorBase* CreateFromLegacyMethod(UObject* Outer, EDungeonMeshSetSelectionMethod Method, UDungeonMeshSetSelectorBase* CustomSelector = nullptr);
};
