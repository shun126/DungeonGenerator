/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonPartsSelectionMethod.h"
#include "Parameter/DungeonSelectionQuery.h"
#include <CoreMinimal.h>
#include <memory>
#include "DungeonPartsSelectorBase.generated.h"

namespace dungeon
{
	class Random;
}

/*
 * Base class for C++ parts selectors that can use the synchronized dungeon random stream.
 * 同期済みのダンジョン乱数ストリームを使用できる C++ パーツセレクターの基底クラスです。
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonPartsSelectorBase : public UObject
{
	GENERATED_BODY()

public:
	/*
	 * Selects an index from parts candidates using synchronized generation context.
	 * 同期された生成コンテキストを使用してパーツ候補からインデックスを選択します。
	 */
	virtual int32 SelectPartsIndexNative(const FDungeonPartsQuery& Query, const std::shared_ptr<dungeon::Random>& Random, int32 NumCandidates) const;

	/*
	 * Creates a selector matching a legacy v1 parts selection method for v2.0.0 migration only.
	 * Do not use this helper for new v2 code; assign a selector object directly instead.
	 * v2.0.0 の移行専用に、v1 の旧パーツ選択方式に対応するセレクターを生成します。
	 * 新しい v2 コードでは使用せず、セレクターオブジェクトを直接設定してください。
	 */
	static UDungeonPartsSelectorBase* CreateFromLegacyMethod(UObject* Outer, EDungeonPartsSelectionMethod Method, UDungeonPartsSelectorBase* CustomSelector = nullptr);
};
