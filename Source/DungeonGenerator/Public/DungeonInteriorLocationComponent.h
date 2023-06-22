/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorLocationComponent.generated.h"

/*
Additional interior component class
Interior can be added to a component position by adding it to the interior actor

Draws and spawns actors with "fillables" in their interior tag
*/
UCLASS(Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonInteriorLocationComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonInteriorLocationComponent(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonInteriorLocationComponent() = default;

	/*
	Get interior tags
	*/
	const TArray<FString> GetInteriorTags() const noexcept;

protected:
	// interior tags (filter)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		TArray<FString> InteriorTags;
};

inline UDungeonInteriorLocationComponent::UDungeonInteriorLocationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline const TArray<FString> UDungeonInteriorLocationComponent::GetInteriorTags() const noexcept
{
	return InteriorTags;
}
