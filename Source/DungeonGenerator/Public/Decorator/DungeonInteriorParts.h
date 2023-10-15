/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorSystemTags.h"
#include "DungeonInteriorParts.generated.h"

// Angle of freedom
UENUM(BlueprintType)
enum class EDungeonAngleTypeOfPlacement : uint8
{
	None,
	Every180Degrees,
	Every90Degrees,
	Free
};

/*
Dungeon interior parts class
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonInteriorParts
{
	GENERATED_BODY()

public:
	/*
	Checks if InteriorTags contains the entire contents of tags
	*/
	bool Match(const TArray<FString>& tags) const noexcept;

public:
	/*
	Actor to spawn as interior
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "Actor"))
		UClass* Class;
	// TObjectPtr not used for UE4 compatibility

	/*
	List of tags to be set from the decorator
	- priority  : major
	- room parts: aisle, slope, hall, hanare, start, goal
	- location  : center, wall

	Please let us know your opinion on whether additional tags, such as ceilings, are needed.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|InteriorTags")
		FDungeonInteriorSystemTags InteriorSystemTags;

	/*
	If decorator contains a query tag, this part will be selected as a candidate.
	If the InteriorTags of the DuneInteriorLocationComponent is set to 'cup',
	The part with 'cup' set will be selected as a candidate for spawning.

	You can set a tag for a room type, e.g., 'kitchen'. This can be specified from the DungeonRoomSensor class.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|InteriorTags")
		TSet<FString> InteriorAdditionalTags;

	/*
	Additional Extent to prohibit placement
	Enlarge the bounding box in the planar direction
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = 0))
		float AdditionalExtentToProhibitPlacement = 0.f;

	/*
	After placement, an overlap check is performed, and if there is overlap, the actor is destroyed.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		bool OverlapCheck = false;

	/*
	Lottery Results
	After a query that contains InteriorTags, a lottery is drawn to see if spawning is possible.
	If 100, the lottery always succeeds.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = 1, ClampMax = 100))
		uint8 Frequency = 100;

	/*
	Error in position to spawn as interior
	Specify the ratio of collision size.
	0 means the center of the collision, 100 means collision size.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ClampMin = 0, ClampMax = 200))
		uint8 PercentageOfErrorInPlacement = 10;

	/*
	How to rotate an actor when placing
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		EDungeonAngleTypeOfPlacement DungeonAngleTypeOfPlacement = EDungeonAngleTypeOfPlacement::Every90Degrees;
};
