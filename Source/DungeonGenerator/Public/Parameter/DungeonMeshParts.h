/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Parameter/DungeonPartsTransform.h"
#include "DungeonMeshParts.generated.h"

/**
 * Mesh Parts
 * メッシュのパーツ
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	/**
	 * Class of static mesh to spawn
	 * スポーンするスタティックメッシュ
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Class of static mesh to spawn"))
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;
};
