/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <UObject/SoftObjectPath.h>
#include "DungeonRoomRegister.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRoomRegister
{
	GENERATED_BODY()

public:
	bool IsValid() const noexcept;

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
		bool GenerateRoofMesh = true;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		bool GenerateFloorMesh = false;
};

inline bool FDungeonRoomRegister::IsValid() const noexcept
{
	return LevelPath.IsValid();
}

inline const FSoftObjectPath& FDungeonRoomRegister::GetLevelPath() const noexcept
{
	return LevelPath;
}

inline FIntVector FDungeonRoomRegister::GetSize() const noexcept
{
	return FIntVector(Width, Depth, Height);
}

inline int32 FDungeonRoomRegister::GetWidth() const noexcept
{
	return Width;
}

inline int32 FDungeonRoomRegister::GetDepth() const noexcept
{
	return Depth;
}

inline int32 FDungeonRoomRegister::GetHeight() const noexcept
{
	return Height;
}

inline bool FDungeonRoomRegister::IsGenerateRoofMesh() const noexcept
{
	return GenerateRoofMesh;
}

inline bool FDungeonRoomRegister::IsGenerateFloorMesh() const noexcept
{
	return GenerateFloorMesh;
}
