/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonAisleMeshSetDatabase.h"
#include "DungeonRoomMeshSetDatabase.h"
#include "SubActor/DungeonRoomSensor.h"
#include "SubLevel/DungeonRoomLocator.h"
#include "SubLevel/DungeonRoomRegister.h"
#include <functional>
#include <memory>
#include "DungeonGenerateParameter.generated.h"

// forward declaration
class UDungeonInteriorDatabase;
class UDungeonRoomDatabase;
struct FDungeonRoomLocator;

/**
Dungeon generation parameter class
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonGenerateParameter() = default;

	int32 GetRandomSeed() const;
	int32 GetGeneratedRandomSeed() const;

	int32 GetNumberOfCandidateRooms() const;

	const FInt32Interval& GetRoomWidth() const noexcept;
	const FInt32Interval& GetRoomDepth() const noexcept;
	const FInt32Interval& GetRoomHeight() const noexcept;

	int32 GetHorizontalRoomMargin() const noexcept;
	int32 GetVerticalRoomMargin() const noexcept;

	float GetGridSize() const;

	bool IsMovePlayerStartToStartingPoint() const noexcept;

	const FDungeonRoomRegister& GetStartRoom() const;
	const FDungeonRoomRegister& GetGoalRoom() const;

	void EachDungeonRoomLocator(std::function<void(const FDungeonRoomLocator&)> func) const;

	UClass* GetRoomSensorClass() const;

	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const FIntVector& location) const;
	
	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const;

	// 	Converts from a grid coordinate system to a world coordinate system
	FIntVector ToGrid(const FVector& location) const;

private:
	void SetRandomSeed(const int32 generateRandomSeed);
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

#if WITH_EDITOR
	void DumpToJson() const;
	FString GetJsonDefaultDirectory() const;
#endif

private:
	const UDungeonRoomMeshSetDatabase* GetDungeonRoomPartsDatabase() const noexcept;
	const UDungeonAisleMeshSetDatabase* GetDungeonAislePartsDatabase() const noexcept;

	const FDungeonMeshPartsWithDirection* SelectFloorParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectWallParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonRandomActorParts* SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonDoorActorParts* SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;

	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const;

private:
	void EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	void EachRoomFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachRoomWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachRoomRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	void EachAisleFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachAisleWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachAisleRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

protected:
	//! Seed of random number (if 0, auto-generated)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
		int32 RandomSeed = 0;

	//! Seed of random numbers used for the last generation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
		int32 GeneratedRandomSeed = 0;

	/**
	Candidate number of rooms to be generated
	This is the initial number of rooms to be generated, not the final number of rooms to be generated.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
		uint8 NumberOfCandidateRooms = 20;

	//! Room Width
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomWidth = { 3, 8 };

	//! Room depth
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomDepth = { 3, 8 };

	//! Room height
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomHeight = { 2, 4 };

	//! Horizontal room margins
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"), DisplayName = "Room Horizontal Margin")
		int32 RoomMargin = 2;

	//! Horizontal room-to-room coupling
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		bool MergeRooms = false;

	// Move PlayerStart to the starting point.
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		bool MovePlayerStartToStartingPoint = true;

	//! Vertical room-to-room margins
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 VerticalRoomMargin = 0;

	//! voxel size
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		float GridSize = 100.f;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts", BlueprintReadWrite)
		UDungeonRoomMeshSetDatabase* DungeonRoomPartsDatabase;
		// TObjectPtr not used for UE4 compatibility

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts", BlueprintReadWrite)
		UDungeonAisleMeshSetDatabase* DungeonAislePartsDatabase;
		// TObjectPtr not used for UE4 compatibility

	// How to generate parts of pillar
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Pillar", BlueprintReadWrite)
		EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// Pillar Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Pillar", BlueprintReadWrite)
		TArray<FDungeonMeshParts> PillarParts;

	// How to generate parts for torch
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Torch", BlueprintReadWrite)
		EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// Torch (pillar lighting) parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Torch", BlueprintReadWrite)
		TArray<FDungeonRandomActorParts> TorchParts;

	// How to generate door parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Door", BlueprintReadWrite)
		EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// Door Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Door", BlueprintReadWrite)
		TArray<FDungeonDoorActorParts> DoorParts;

	// Interior Asset
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Interior")
		UDungeonInteriorDatabase* DungeonInteriorDatabase;
	// TObjectPtr not used for UE4 compatibility

	//! sublevel placement
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|SubLevel")
		UDungeonRoomDatabase* DungeonRoomDatabase;
	// TObjectPtr not used for UE4 compatibility

	/*
	Places the specified sublevel in the starting room
	If LevelPath is not set, the room is automatically generated.
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|SubLevel", BlueprintReadWrite)
		FDungeonRoomRegister StartRoom;

	/*
	Places the specified sublevel in the goal room
	If LevelPath is not set, the room is automatically generated.
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|SubLevel", BlueprintReadWrite)
		FDungeonRoomRegister GoalRoom;

	// Room Sensor Class
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomSensor", BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensor"))
		UClass* DungeonRoomSensorClass;
	// TObjectPtr not used for UE4 compatibility

	friend class ADungeonGenerateActor;
	friend class CDungeonGeneratorCore;
};

inline int32 UDungeonGenerateParameter::GetRandomSeed() const
{
	return RandomSeed;
}

inline int32 UDungeonGenerateParameter::GetGeneratedRandomSeed() const
{
	return GeneratedRandomSeed;
}

inline void UDungeonGenerateParameter::SetRandomSeed(const int32 generateRandomSeed)
{
	RandomSeed = generateRandomSeed;
}

inline void UDungeonGenerateParameter::SetGeneratedRandomSeed(const int32 generatedRandomSeed)
{
	GeneratedRandomSeed = generatedRandomSeed;
}

inline int32 UDungeonGenerateParameter::GetNumberOfCandidateRooms() const
{
	return NumberOfCandidateRooms;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomWidth() const noexcept
{
	return RoomWidth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomDepth() const noexcept
{
	return RoomDepth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomHeight() const noexcept
{
	return RoomHeight;
}

inline int32 UDungeonGenerateParameter::GetHorizontalRoomMargin() const noexcept
{
	return RoomMargin;
}

inline int32 UDungeonGenerateParameter::GetVerticalRoomMargin() const noexcept
{
	return VerticalRoomMargin;
}

inline float UDungeonGenerateParameter::GetGridSize() const
{
	return GridSize;
}

inline bool UDungeonGenerateParameter::IsMovePlayerStartToStartingPoint() const noexcept
{
	return MovePlayerStartToStartingPoint;
}

inline const UDungeonRoomMeshSetDatabase* UDungeonGenerateParameter::GetDungeonRoomPartsDatabase() const noexcept
{
	return DungeonRoomPartsDatabase;
}

inline const UDungeonAisleMeshSetDatabase* UDungeonGenerateParameter::GetDungeonAislePartsDatabase() const noexcept
{
	return DungeonAislePartsDatabase;
}

inline const FDungeonRoomRegister& UDungeonGenerateParameter::GetStartRoom() const
{
	return StartRoom;
}

inline const FDungeonRoomRegister& UDungeonGenerateParameter::GetGoalRoom() const
{
	return GoalRoom;
}

inline UClass* UDungeonGenerateParameter::GetRoomSensorClass() const
{
	return DungeonRoomSensorClass;
}

inline void UDungeonGenerateParameter::EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachRoomFloorParts(func);
	EachAisleFloorParts(func);
}

inline void UDungeonGenerateParameter::EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachRoomWallParts(func);
	EachAisleWallParts(func);
}

inline void UDungeonGenerateParameter::EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachRoomRoofParts(func);
	EachAisleRoofParts(func);
}
