/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorLocationComponent.h"
#include "DungeonInteriorParts.h"
#include <functional>
#include "DungeonInteriorAsset.generated.h"

namespace dungeon
{
	class Random;
}

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonInteriorAsset : public UObject
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonInteriorAsset(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonInteriorAsset() = default;

	/*
	Selects FDungeonInteriorParts that match interiorTags
	*/
	TArray<FDungeonInteriorParts> Select(const TArray<FString>& interiorTags, dungeon::Random& random) const;

	/*
	Locate the actor's own UDungeonInteriorLocationComponent and select the interior you need.
	*/
	void EachInteriorLocation(const AActor* actor, std::function<void(UDungeonInteriorLocationComponent* component)> function) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<FDungeonInteriorParts> Parts;
};

inline UDungeonInteriorAsset::UDungeonInteriorAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
