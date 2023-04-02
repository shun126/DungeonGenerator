/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomParts.h"
#include "DungeonRoomLocator.generated.h"

// cppcheck-suppress [unknownMacro]
UENUM(BlueprintType)
enum class EDungeonRoomSizeCondition : uint8
{
	Equal,
	EqualGreater,
};

/**
Sub-level information to replace dungeon rooms
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomLocator
{
	GENERATED_BODY()

public:
	const FSoftObjectPath& GetLevelPath() const noexcept;
	FIntVector GetSize() const noexcept;
	int32 GetWidth() const noexcept;
	int32 GetDepth() const noexcept;
	int32 GetHeight() const noexcept;
	EDungeonRoomSizeCondition GetWidthCondition() const noexcept;
	EDungeonRoomSizeCondition GetDepthCondition() const noexcept;
	EDungeonRoomSizeCondition GetHeightCondition() const noexcept;
	EDungeonRoomParts GetDungeonParts() const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (AllowedClasses = "World"))
		FSoftObjectPath LevelPath;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Width;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Depth;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Height;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition WidthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition DepthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition HeightCondition = EDungeonRoomSizeCondition::EqualGreater;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomParts DungeonParts = EDungeonRoomParts::Any;
};

inline const FSoftObjectPath& FDungeonRoomLocator::GetLevelPath() const noexcept
{
	return LevelPath;
}

inline FIntVector FDungeonRoomLocator::GetSize() const noexcept
{
	return FIntVector(Width, Depth, Height);
}

inline int32 FDungeonRoomLocator::GetWidth() const noexcept
{
	return Width;
}

inline int32 FDungeonRoomLocator::GetDepth() const noexcept
{
	return Depth;
}

inline int32 FDungeonRoomLocator::GetHeight() const noexcept
{
	return Height;
}

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

inline EDungeonRoomParts FDungeonRoomLocator::GetDungeonParts() const noexcept
{
	return DungeonParts;
}
