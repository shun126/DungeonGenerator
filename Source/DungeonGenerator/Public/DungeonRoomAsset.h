/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomHeightCondition.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomAsset.generated.h"

UCLASS(Blueprintable)
class DUNGEONGENERATOR_API UDungeonRoomAsset : public UObject
{
	GENERATED_BODY()

public:
	UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonRoomAsset() = default;

protected:
public:
	UPROPERTY(EditAnywhere)
		FName LevelName;

	UPROPERTY(EditAnywhere)
		FSoftObjectPath LevelPath;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
		int32 Width;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
		int32 Depth;

	UPROPERTY(EditAnywhere, meta = (ClampMin = 1))
		int32 Height;

	UPROPERTY(EditAnywhere)
		EDungeonRoomHeightCondition HeightCondition = EDungeonRoomHeightCondition::Equal;

	UPROPERTY(EditAnywhere)
		EDungeonRoomParts DungeonParts = EDungeonRoomParts::Any;
};
