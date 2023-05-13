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
Sub-level information to replace dungeon rooms class
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomLocator
{
	GENERATED_BODY()

public:
	/*
	Get sublevel paths
	*/
	const FSoftObjectPath& GetLevelPath() const noexcept;

	/*
	Gets the sub-level grid size
	*/
	FIntVector GetSize() const noexcept;

	/*
	Gets the grid width of the sublevel
	*/
	int32 GetWidth() const noexcept;

	/*
	Gets the grid depth of the sublevel
	*/
	int32 GetDepth() const noexcept;

	/*
	Gets the grid height of the sublevel
	*/
	int32 GetHeight() const noexcept;

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
	EDungeonRoomParts GetDungeonParts() const noexcept;

	/*
	Generates or retrieves the ceiling mesh of the grid for sublevel placement
	*/
	bool IsGenerateRoofMesh() const noexcept;

	/*
	Generates or retrieves the floor mesh of the grid for sublevel placement
	*/
	bool IsGenerateFloorMesh() const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (AllowedClasses = "World"))
		FSoftObjectPath LevelPath;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Width = 1;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Depth = 1;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (ClampMin = 1))
		int32 Height = 1;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition WidthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition DepthCondition = EDungeonRoomSizeCondition::Equal;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomSizeCondition HeightCondition = EDungeonRoomSizeCondition::EqualGreater;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		EDungeonRoomParts DungeonParts = EDungeonRoomParts::Any;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		bool GenerateRoofMesh = true;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		bool GenerateFloorMesh = false;
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

inline bool FDungeonRoomLocator::IsGenerateRoofMesh() const noexcept
{
	return GenerateRoofMesh;
}

inline bool FDungeonRoomLocator::IsGenerateFloorMesh() const noexcept
{
	return GenerateFloorMesh;
}
