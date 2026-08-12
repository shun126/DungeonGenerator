/**
 * @author      Shun Moriya
 * @copyright   2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Helper/DungeonDirection.h"
#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <functional>
#include <map>
#include "DungeonAisleGridMap.generated.h"

/**
 * Grid of aisle
 * 通路グリッド
 */
USTRUCT(BlueprintType)
struct FDungeonAisleGrid
{
	GENERATED_BODY()

	/**
	 * Identifier of the generated aisle that owns this grid.
	 * このグリッドを所有する生成通路の識別子です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Identifier of the generated aisle that owns this grid."))
	int32 Identifier = INDEX_NONE;

	/**
	 * Aisle grid direction
	 * 通路グリッドの方向
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Aisle grid direction"))
	EDungeonDirection Direction = EDungeonDirection::North;

	/**
	 * Start-to-goal depth ratio of the owning aisle, stored as 0 to 255.
	 * 所有する通路のスタートからゴールまでの深度比です。0 から 255 で保持します。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Start-to-goal depth ratio of the owning aisle, stored as 0 to 255."))
	uint8 DepthRatioFromStart = 0;

	/**
	 * Zone index assigned to the owning aisle.
	 * 所有する通路に割り当てられた Zone 番号です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Zone index assigned to the owning aisle."))
	int32 ZoneIndex = INDEX_NONE;

	/**
	 * Center position of aisle grid (height is floor position)
	 * 通路グリッドの中心位置（高さは床の位置）
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Center position of aisle grid (height is floor position)"))
	FVector Location = FVector::ZeroVector;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FDungeonAisleGridMapLoopSignature, const TArray<FDungeonAisleGrid>&, AisleGridArray);

/**
 * Record aisle grid for each aisle
 * 通路毎に通路グリッドを記録します
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonAisleGridMap : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Represents UDungeonAisleGridMap.
	 * コンストラクタ
	 */
	explicit UDungeonAisleGridMap(const FObjectInitializer& initializer);

	/**
	 * Destroys the ~UDungeonAisleGridMap instance.
	 * デストラクタ
	 */
	virtual ~UDungeonAisleGridMap() override = default;

	/**
	 * Register aisle grid
	 * 通路グリッドを登録します
	 */
	void Register(int32 identifier, EDungeonDirection direction, const FVector& location, uint8 depthRatioFromStart, int32 zoneIndex);

	/**
	 * Update aisle grid
	 * 通路グリッドを更新します
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "DungeonGenerator", meta = (ToolTip = "Update aisle grid"))
	void ForEach(const FDungeonAisleGridMapLoopSignature& OnLoop) const;

	/**
	 * Update aisle grid
	 * 通路グリッドを更新します
	 */
	void Each(const std::function<void(const TArray<FDungeonAisleGrid>& aisleGridArray)>& func) const
	{
		for (const auto& aisleGrid : mAisleGridMap)
			func(aisleGrid.second);
	}

private:
	std::map<uint16_t, TArray<FDungeonAisleGrid>> mAisleGridMap;

	friend class UDungeonAisleGridMapBlueprintFunctionLibrary;
};

/**
 * Class for BluePrint function to manipulate UDungeonAisleGridMap
 * UDungeonAisleGridMapを操作するBluePrint関数のクラス
 */
UCLASS(ClassGroup = "DungeonGenerator")
class UDungeonAisleGridMapBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Update aisle grid
	 * 通路グリッドを更新します
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Update aisle grid"))
	static void ForEach(const UDungeonAisleGridMap* aisleGridArray, const FDungeonAisleGridMapLoopSignature& OnLoop);
};
