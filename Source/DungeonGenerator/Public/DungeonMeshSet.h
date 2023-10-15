/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <functional>
#include <memory>
#include <CoreMinimal.h>
#include "DungeonMeshSet.generated.h"

// forward declaration
class UClass;
class UStaticMesh;

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
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept;
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

	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
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

	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const std::shared_ptr<dungeon::Random>& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
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

USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshSet
{
	GENERATED_BODY()

public:
	static FDungeonRandomActorParts* SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);

	template<typename T = FDungeonMeshParts>
	static T* SelectParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);

	template<typename T = FDungeonMeshParts>
	static void EachParts(const TArray<T>& parts, std::function<void(const T&)> func);

private:
	static int32 SelectDungeonMeshPartsIndex(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod);
	static FDungeonActorParts* SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod);
};

// aka: SelectActorParts, SelectRandomActorParts
template <typename T>
inline T* FDungeonMeshSet::SelectParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod)
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	return const_cast<T*>(&parts[index]);
}

template <typename T>
inline void FDungeonMeshSet::EachParts(const TArray<T>& parts, std::function<void(const T&)> func)
{
	for (const auto& part : parts)
	{
		func(part);
	}
}
