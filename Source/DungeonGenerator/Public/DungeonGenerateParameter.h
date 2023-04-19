/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomAsset.h"
#include "DungeonRoomSensor.h"
#include <functional>
#include "DungeonGenerateParameter.generated.h"

// forward declaration
class UDungeonRoomAsset;

namespace dungeon
{
	class Direction;
	class Grid;
	class Random;
}

/**
Part placement direction
*/
UENUM(BlueprintType)
enum class EDungeonPartsPlacementDirection : uint8
{
	North,
	East,
	South,
	West,
	FollowGridDirection,
	RandomDirection
};

/**
Part Selection Method
*/
UENUM(BlueprintType)
enum class EDungeonPartsSelectionMethod : uint8
{
	Random,
	GridIndex,
	Direction,
};

/**
Parts transform
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		FTransform RelativeTransform;

	FTransform CalculateWorldTransform(const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept;

protected:
	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
};

/**
Actor Parts
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "Actor"))
		UClass* ActorClass = nullptr;
};

/**
Door actor Parts
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonDoorActorParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "DungeonDoor"))
		UClass* ActorClass = nullptr;
};

/**
Mesh Parts
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshParts : public FDungeonPartsTransform
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		UStaticMesh* StaticMesh = nullptr;
};

/**
Actor parts with direction specification
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorPartsWithDirection : public FDungeonActorParts
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		EDungeonPartsPlacementDirection PlacementDirection = EDungeonPartsPlacementDirection::RandomDirection;

	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
};

/**
Mesh parts with direction specification
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshPartsWithDirection : public FDungeonMeshParts
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		EDungeonPartsPlacementDirection PlacementDirection = EDungeonPartsPlacementDirection::RandomDirection;

	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
};

/**
Actor parts with Probability
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRandomActorParts : public FDungeonActorPartsWithDirection
{
	GENERATED_BODY()

public:
	// Production frequency
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0.", ClampMax = "1."))
		float Frequency = 1.f;
};

/**
Dungeon generation parameters
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);

	/*
	destructor
	*/
	virtual ~UDungeonGenerateParameter() = default;

	int32 GetRandomSeed() const;
	int32 GetGeneratedRandomSeed() const;
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

	int32 GetNumberOfCandidateRooms() const;

	const FInt32Interval& GetRoomWidth() const noexcept;
	const FInt32Interval& GetRoomDepth() const noexcept;
	const FInt32Interval& GetRoomHeight() const noexcept;

	int32 GetHorizontalRoomMargin() const noexcept;
	int32 GetVerticalRoomMargin() const noexcept;

	float GetGridSize() const;

	const FDungeonMeshPartsWithDirection* SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectWallParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshPartsWithDirection* SelectRoomRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachRoomRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshPartsWithDirection* SelectAisleRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachAisleRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshParts* SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonRandomActorParts* SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	//const FDungeonRandomActorParts* SelectChandelierParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;

	const FDungeonDoorActorParts* SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;

	bool IsMovePlayerStartToStartingPoint() const noexcept;

	const FDungeonActorPartsWithDirection& GetStartParts() const;

	const FDungeonActorPartsWithDirection& GetGoalParts() const;

	void EachDungeonRoomLocator(std::function<void(const FDungeonRoomLocator&)> func) const;

	UClass* GetRoomSensorClass() const;

	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const FIntVector& location) const;
	
	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const;

	// 	Converts from a grid coordinate system to a world coordinate system
	FIntVector ToGrid(const FVector& location) const;

#if WITH_EDITOR
	void DumpToJson() const;
#endif

private:
	int32 SelectDungeonMeshPartsIndex(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod) const;
		
	template<typename T = FDungeonMeshParts>
	T* SelectParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	FDungeonActorParts* SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	FDungeonRandomActorParts* SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	template<typename T = FDungeonMeshParts>
	void EachParts(const TArray<T>& parts, std::function<void(const T&)> func) const;

#if WITH_EDITOR
	FString GetJsonDefaultDirectory() const;
#endif

protected:
	//! Seed of random number (if 0, auto-generated)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
		int32 RandomSeed = 0;

	//! Seed of random numbers used for the last generation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
		int32 GeneratedRandomSeed = 0;

	// dungeon level
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "1"))
		uint8 NumberOfCandidateFloors = 3;

	/**
	Candidate number of rooms to be generated
	This is the initial number of rooms to be generated, not the final number of rooms to be generated.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
		uint8 NumberOfCandidateRooms = 8;

	//! Room Width
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomWidth = { 4, 8 };

	//! Room depth
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomDepth = { 4, 8 };

	//! Room height
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomHeight = { 2, 3 };

	//! Horizontal room margins
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"), DisplayName = "Room Horizontal Margin")
		int32 RoomMargin = 1;

	//! Horizontal room-to-room coupling
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		bool MergeRooms = false;

	//! Vertical room-to-room margins
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 VerticalRoomMargin = 0;

	//! voxel size
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		float GridSize = 100.f;

	//!	How to generate floor parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Floor parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> FloorParts;

	//!	How to generate parts for stairs and ramps
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
		EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Stairs and ramp parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
		TArray<FDungeonMeshParts> SlopeParts;

	//!	How to generate wall parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Wall parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		TArray<FDungeonMeshParts> WallParts;

	//!	How to generate room parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomRoof", BlueprintReadWrite)
		EDungeonPartsSelectionMethod RoomRoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Room Roof Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomRoof", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> RoomRoofParts;

	//!	How to generate aisle parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|AisleRoof", BlueprintReadWrite)
		EDungeonPartsSelectionMethod AisleRoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Roof parts of aisle
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|AisleRoof", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> AisleRoofParts;

	//!	How to generate parts of columns
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Pillar", BlueprintReadWrite)
		EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Pillar Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Pillar", BlueprintReadWrite)
		TArray<FDungeonMeshParts> PillarParts;

	//!	How to generate parts for torch (pillar lighting)
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Torch", BlueprintReadWrite)
		EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! Torch (pillar lighting) parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Torch", BlueprintReadWrite)
		TArray<FDungeonRandomActorParts> TorchParts;

/*
	シャンデリア（天井の照明）のパーツ
	UPROPERTY(EditAnywhere, Category="DungeonGenerator", BlueprintReadWrite)
		TArray<FDungeonRandomActorParts> ChandelierParts;
*/

	//!	How to generate door parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Door", BlueprintReadWrite)
		EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// Door Parts
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Door", BlueprintReadWrite)
		TArray<FDungeonDoorActorParts> DoorParts;

	// Move PlayerStart to the starting point.
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Door", BlueprintReadWrite)
		bool MovePlayerStartToStartingPoint = true;

	// starting point
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		FDungeonActorPartsWithDirection StartParts;

	// goal position
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		FDungeonActorPartsWithDirection GoalParts;

	// Room Sensor Class
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensor"))
		UClass* DungeonRoomSensorClass = ADungeonRoomSensor::StaticClass();

	//! sublevel placement
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		UDungeonRoomAsset* DungeonRoomAsset;
	// TObjectPtr<UDungeonRoomAsset> not used for UE4 compatibility

	friend class CDungeonGeneratorCore;
};

// aka: SelectActorParts, SelectRandomActorParts
template <typename T>
inline T* UDungeonGenerateParameter::SelectParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	return const_cast<T*>(&parts[index]);
}

template <typename T>
inline void UDungeonGenerateParameter::EachParts(const TArray<T>& parts, std::function<void(const T&)> func) const
{
	for (const auto& part : parts)
	{
		func(part);
	}
}
