/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Decorator/DungeonInteriorUnplaceableBounds.h"
#include <memory>
#include "DungeonInteriorDecorator.generated.h"

struct FDungeonInteriorParts;
class UDungeonInteriorDatabase;
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
	explicit FDungeonInteriorDecorator(UDungeonInteriorDatabase* asset);

	// constructor
	FDungeonInteriorDecorator(UDungeonInteriorDatabase* asset, const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& unplaceableBounds);

	// destructor
	virtual ~FDungeonInteriorDecorator();

	// initialize
	void Initialize(UDungeonInteriorDatabase* asset);

	// finalize 
	void Finalize();

	// check valid
	bool IsValid() const noexcept;

	UDungeonInteriorDatabase* GetAsset();
	const UDungeonInteriorDatabase* GetAsset() const;


	void ResetUnplacatableBounds(const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& interiorUnplaceableBounds);
	std::shared_ptr<CDungeonInteriorUnplaceableBounds> GetUnplacatableBounds() const noexcept;
	bool IntersectUnplacatableBounds(const FBox& bounds) const;
	bool IsInsideOrOnUnplacatableBounds(const FVector& location) const;


	void ClearDecorationLocation(const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& interiorUnplaceableBounds = nullptr);
	void AddDecorationLocation(const FVector& location, const double yaw);
	size_t GetDecorationLocationSize() const noexcept;
	void ShuffleDecorationLocation(const std::shared_ptr<dungeon::Random>& random);

	/*
	Spawns an actor at the added decoration position.
	Decoration positions that have been successfully spawned are deleted.
	*/
	void TrySpawnActors(UWorld* world, const TArray<FString>& tags, const std::shared_ptr<dungeon::Random>& random);




	/*
	Selects FDungeonInteriorParts that match interiorTags
	*/
	TArray<FDungeonInteriorParts> Select(const TArray<FString>& interiorTags, const std::shared_ptr<dungeon::Random>& random) const;



	/*
	Extract DecorationLocation in bounds
	*/
	std::shared_ptr<FDungeonInteriorDecorator> CreateInteriorDecorator(const FBox& placeableBounds) const;

private:
	bool TrySpawnActorsImplement(UWorld* world, const TArray<FString>& tags, const FTransform& parentTransform, const std::shared_ptr<dungeon::Random>& random, const size_t depth);
	bool Intersects(const FBox& bounds) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		UDungeonInteriorDatabase* Asset;
	// TObjectPtr not used for UE4 compatibility

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
			return *this;
		}
		DecorationLocation& operator=(DecorationLocation&& other) noexcept
		{
			Position = std::move(other.Position);
			Yaw = other.Yaw;
			return *this;
		}
	};
	std::vector<std::shared_ptr<DecorationLocation>> mDecorationLocations;

	// Placed actor bounds
	TArray<FBox> mPlacedBounds;

	// Unplaceable actor bounds
	std::shared_ptr<CDungeonInteriorUnplaceableBounds> mUnplaceableBounds;
};

inline FDungeonInteriorDecorator::FDungeonInteriorDecorator(UDungeonInteriorDatabase* asset)
	: Asset(asset)
	, mUnplaceableBounds(std::make_shared<CDungeonInteriorUnplaceableBounds>())
{
}

inline FDungeonInteriorDecorator::FDungeonInteriorDecorator(UDungeonInteriorDatabase* asset, const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& unplaceableBounds)
	: Asset(asset)
	, mUnplaceableBounds(unplaceableBounds)
{
}

inline FDungeonInteriorDecorator::~FDungeonInteriorDecorator()
{
	Finalize();
}

inline void FDungeonInteriorDecorator::Initialize(UDungeonInteriorDatabase* asset)
{
	Asset = asset;
}

inline void FDungeonInteriorDecorator::Finalize()
{
	Asset = nullptr;
}

inline bool FDungeonInteriorDecorator::IsValid() const noexcept
{
	return Asset != nullptr;
}

inline UDungeonInteriorDatabase* FDungeonInteriorDecorator::GetAsset()
{
	return Asset;
}

inline const UDungeonInteriorDatabase* FDungeonInteriorDecorator::GetAsset() const
{
	return Asset;
}

inline void FDungeonInteriorDecorator::ResetUnplacatableBounds(const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& interiorUnplaceableBounds)
{
	mUnplaceableBounds = interiorUnplaceableBounds;
}

inline std::shared_ptr<CDungeonInteriorUnplaceableBounds> FDungeonInteriorDecorator::GetUnplacatableBounds() const noexcept
{
	return mUnplaceableBounds;
}

inline bool FDungeonInteriorDecorator::IntersectUnplacatableBounds(const FBox& bounds) const
{
	return mUnplaceableBounds ? mUnplaceableBounds->Intersect(bounds) : false;
}

inline bool FDungeonInteriorDecorator::IsInsideOrOnUnplacatableBounds(const FVector& location) const
{
	return mUnplaceableBounds ? mUnplaceableBounds->IsInsideOrOn(location) : false;
}

inline void FDungeonInteriorDecorator::ClearDecorationLocation(const std::shared_ptr<CDungeonInteriorUnplaceableBounds>& interiorUnplaceableBounds)
{
	ResetUnplacatableBounds(interiorUnplaceableBounds);
	mDecorationLocations.clear();
}

inline void FDungeonInteriorDecorator::AddDecorationLocation(const FVector& location, const double yaw)
{
	mDecorationLocations.push_back(std::make_shared<DecorationLocation>(location, yaw));
}

inline size_t FDungeonInteriorDecorator::GetDecorationLocationSize() const noexcept
{
	return mDecorationLocations.size();
}
