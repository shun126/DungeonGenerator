/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Components/SceneComponent.h>
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
	const TArray<FString> GetInquireInteriorTags() const noexcept;

protected:
	/*
	interior tags (filter)
	For example, if the creator returns the tag `kitchen`, the interior with the kitchen tag will be selected.
	If multiple tags are set, actors containing all tags will be selected as spawn candidates.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		TArray<FString> InquireInteriorTags;
};

inline UDungeonInteriorLocationComponent::UDungeonInteriorLocationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline const TArray<FString> UDungeonInteriorLocationComponent::GetInquireInteriorTags() const noexcept
{
	return InquireInteriorTags;
}
