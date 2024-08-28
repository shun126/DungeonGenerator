/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Parameter/DungeonPartsTransform.h"
#include "DungeonMeshParts.generated.h"

/**
Mesh Parts
メッシュのパーツ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

#if WITH_EDITOR
public:
	// Debug
	virtual FString DumpToJson(const uint32 indent) const override;
#endif

public:
	/**
	Class of static mesh to spawn
	スポーンするスタティックメッシュ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;
};
