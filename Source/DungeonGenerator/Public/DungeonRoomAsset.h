/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomLocator.h"
#include "DungeonRoomAsset.generated.h"

/**
Sub-level information to replace dungeon rooms class
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonRoomAsset : public UObject
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonRoomAsset() = default;

	/*
	Get the dungeon room information array
	\return		Array of FDungeonRoomLocator
	*/
	const TArray<const FDungeonRoomLocator>& GetDungeonRoomLocator() const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		TArray<FDungeonRoomLocator> DungeonRoomLocator;
};

inline UDungeonRoomAsset::UDungeonRoomAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline const TArray<const FDungeonRoomLocator>& UDungeonRoomAsset::GetDungeonRoomLocator() const noexcept
{
	return reinterpret_cast<const TArray<const FDungeonRoomLocator>&>(DungeonRoomLocator);
}
