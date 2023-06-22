/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonInteriorAsset.h"
#include <memory>
#include <vector>
#include "DungeonInteriorDecorator.generated.h"

namespace dungeon
{
	class Random;
}

/*
Interior decorator class
http://www.archmagerises.com/news/2021/6/12/how-to-procedurally-generate-and-decorate-3d-dungeon-rooms-in-unity-c

interior filter tags
 room: aisle, slope, hall, hanare, start, goal
 location: center, wall, roof
 priority: major
*/
USTRUCT(Blueprintable, BlueprintType)
struct DUNGEONGENERATOR_API FDungeonInteriorDecorator
{
	GENERATED_BODY()

public:
	// constructor
	FDungeonInteriorDecorator() = default;

	// constructor
	explicit FDungeonInteriorDecorator(UDungeonInteriorAsset* asset);

	// destructor
	virtual ~FDungeonInteriorDecorator();

	// initialize
	void Initialize(UDungeonInteriorAsset* asset);

	// finalize 
	void Finalize();

	// check valid
	bool IsValid() const noexcept;

	UDungeonInteriorAsset* GetAsset();
	const UDungeonInteriorAsset* GetAsset() const;


	void ClearDecorationLocation();
	void AddDecorationLocation(const FVector& location, const double yaw);
	size_t GetDecorationLocationSize() const noexcept;
	void ShuffleDecorationLocation(dungeon::Random& random);


	void SpawnActors(UWorld* world, const TArray<FString>& tags, dungeon::Random& random);




	/*
	Selects FDungeonInteriorParts that match interiorTags
	*/
	TArray<FDungeonInteriorParts> Select(const TArray<FString>& interiorTags, dungeon::Random& random) const;



	/*
	Extract DecorationLocation in bounds
	*/
	std::shared_ptr<FDungeonInteriorDecorator> Room(const FBox& bounds) const;

private:
	void SpawnActorsImplement(UWorld* world, const TArray<FString>& tags, const FTransform& parentTransform, dungeon::Random& random, const size_t depth);
	bool Intersects(const FBox& bounds) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		UDungeonInteriorAsset* Asset = nullptr;

private:
	struct DecorationLocation final
	{
		FVector Position;
		double Yaw;

		DecorationLocation(const FVector& position, const double yaw)
			: Position(position), Yaw(yaw)
		{}
		DecorationLocation(const DecorationLocation& other) noexcept
			: Position(other.Position), Yaw(other.Yaw)
		{}
		DecorationLocation(DecorationLocation&& other) noexcept
			: Position(std::move(other.Position)), Yaw(other.Yaw)
		{}
		DecorationLocation& operator=(const DecorationLocation& other) noexcept
		{
			Position = other.Position;
			Yaw = other.Yaw;
		}
		DecorationLocation& operator=(DecorationLocation&& other) noexcept
		{
			Position = std::move(other.Position);
			Yaw = other.Yaw;
		}
	};
	std::vector<std::shared_ptr<DecorationLocation>> mDecorationLocations;

	TArray<FBox> mSensors;
};
