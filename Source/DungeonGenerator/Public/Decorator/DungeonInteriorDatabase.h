/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorLocationComponent.h"
#include "DungeonInteriorParts.h"
#include <functional>
#include <memory>
#include "DungeonInteriorDatabase.generated.h"

namespace dungeon
{
	class Random;
}

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonInteriorDatabase : public UObject
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonInteriorDatabase(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonInteriorDatabase() = default;

	/*
	Selects FDungeonInteriorParts that match interiorTags
	*/
	TArray<FDungeonInteriorParts> Select(const TArray<FString>& interiorTags, const std::shared_ptr<dungeon::Random>& random) const;

	/*
	Locate the actor's own UDungeonInteriorLocationComponent and select the interior you need.
	*/
	void EachInteriorLocation(const AActor* actor, std::function<void(UDungeonInteriorLocationComponent* component)> function) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<FDungeonInteriorParts> Parts;
};

inline UDungeonInteriorDatabase::UDungeonInteriorDatabase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UCLASS(Blueprintable, BlueprintType, Deprecated, meta = (DeprecationMessage = "UDungeonInteriorAsset is deprecated, use UDungeonInteriorDatabase."))
class DUNGEONGENERATOR_API UDEPRECATED_DungeonInteriorAsset : public UDungeonInteriorDatabase
{
	GENERATED_BODY()
};
