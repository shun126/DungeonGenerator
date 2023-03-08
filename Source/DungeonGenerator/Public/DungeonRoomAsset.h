/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomHeightCondition.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomAsset.generated.h"

/**
Sub-level information to replace dungeon rooms
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonRoomAsset : public UObject
{
	GENERATED_BODY()

public:
	/**
	constructor
	*/
	explicit UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer);

	/**
	destructor
	*/
	virtual ~UDungeonRoomAsset() = default;

protected:
	// TODO:Stop public. Provide accessors as needed.
public:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		FName LevelName;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		FSoftObjectPath LevelPath;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Width;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Depth;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Height;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomHeightCondition HeightCondition = EDungeonRoomHeightCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomParts DungeonParts = EDungeonRoomParts::Any;
};
