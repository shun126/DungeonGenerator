/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomRegister.h"
#include "Mission/DungeonRoomParts.h"
#include "DungeonRoomLocator.generated.h"

// cppcheck-suppress [unknownMacro]
UENUM(BlueprintType)
enum class EDungeonRoomSizeCondition : uint8
{
	Equal,
	EqualGreater,
};

/**
Sub-level information to replace dungeon rooms class
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomLocator : public FDungeonRoomRegister
{
	GENERATED_BODY()

public:
	/*
	Gets the width condition for placing sublevels
	*/
	EDungeonRoomSizeCondition GetWidthCondition() const noexcept;

	/*
	Gets the depth condition for placing sublevels
	*/
	EDungeonRoomSizeCondition GetDepthCondition() const noexcept;

	/*
	Gets the height condition for placing sublevels
	*/
	EDungeonRoomSizeCondition GetHeightCondition() const noexcept;

	/*
	Get the parts of the dungeon (room roles) where the sublevels will be placed
	*/
	EDungeonRoomLocatorParts GetDungeonParts() const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition WidthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition DepthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition HeightCondition = EDungeonRoomSizeCondition::EqualGreater;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomLocatorParts DungeonParts = EDungeonRoomLocatorParts::Any;
};

inline EDungeonRoomSizeCondition FDungeonRoomLocator::GetWidthCondition() const noexcept
{
	return WidthCondition;
}

inline EDungeonRoomSizeCondition FDungeonRoomLocator::GetDepthCondition() const noexcept
{
	return DepthCondition;
}

inline EDungeonRoomSizeCondition FDungeonRoomLocator::GetHeightCondition() const noexcept
{
	return HeightCondition;
}

inline EDungeonRoomLocatorParts FDungeonRoomLocator::GetDungeonParts() const noexcept
{
	return DungeonParts;
}
