/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomLocator.h"
#include "DungeonRoomDatabase.generated.h"

/**
Sub-level information to replace dungeon rooms class
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonRoomDatabase : public UObject
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonRoomDatabase(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonRoomDatabase() = default;

	/*
	Get the dungeon room information array
	\return		Array of FDungeonRoomLocator
	*/
	const TArray<const FDungeonRoomLocator>& GetDungeonRoomLocator() const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		TArray<FDungeonRoomLocator> DungeonRoomLocator;
};

inline UDungeonRoomDatabase::UDungeonRoomDatabase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline const TArray<const FDungeonRoomLocator>& UDungeonRoomDatabase::GetDungeonRoomLocator() const noexcept
{
	return reinterpret_cast<const TArray<const FDungeonRoomLocator>&>(DungeonRoomLocator);
}

UCLASS(Blueprintable, BlueprintType, Deprecated, meta = (DeprecationMessage = "UDungeonRoomAsset is deprecated, use UDungeonRoomDatabase."))
class DUNGEONGENERATOR_API UDEPRECATED_DungeonRoomAsset : public UDungeonRoomDatabase
{
	GENERATED_BODY()
};
