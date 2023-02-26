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
ダンジョンの部屋に置き換える部屋アセット情報クラス
*/
UCLASS(Blueprintable)
class DUNGEONGENERATOR_API UDungeonRoomAsset : public UObject
{
	GENERATED_BODY()

public:
	/**
	コンストラクタ
	*/
	explicit UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer);

	/**
	デストラクタ
	*/
	virtual ~UDungeonRoomAsset() = default;

protected:
	// TODO:publicを止めて下さい。必要に応じてアクセサを用意して下さい
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
