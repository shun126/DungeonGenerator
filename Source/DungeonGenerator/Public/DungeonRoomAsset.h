/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomLocator.h"
#include "DungeonRoomAsset.generated.h"

/**
Sub-level information to replace dungeon rooms
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonRoomAsset : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonRoomAsset() = default;

	const TArray<const FDungeonRoomLocator>& GetDungeonRoomLocator() const;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		TArray<FDungeonRoomLocator> DungeonRoomLocator;
};

inline UDungeonRoomAsset::UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline const TArray<const FDungeonRoomLocator>& UDungeonRoomAsset::GetDungeonRoomLocator() const
{
	return reinterpret_cast<const TArray<const FDungeonRoomLocator>&>(DungeonRoomLocator);
}
