/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorDecorator.h"
#include "DungeonGeneratorCore.h"
#include "Core/Math/Random.h"
#include "Core/Debug/Debug.h"
#include <utility>

FDungeonInteriorDecorator::FDungeonInteriorDecorator(UDungeonInteriorAsset* asset)
	: Asset(asset)
{
}

FDungeonInteriorDecorator::~FDungeonInteriorDecorator()
{
	Finalize();
}

void FDungeonInteriorDecorator::Initialize(UDungeonInteriorAsset* asset)
{
	Asset = asset;
}

void FDungeonInteriorDecorator::Finalize()
{
	Asset = nullptr;
}

bool FDungeonInteriorDecorator::IsValid() const noexcept
{
	return Asset != nullptr;
}

UDungeonInteriorAsset* FDungeonInteriorDecorator::GetAsset()
{
	return Asset;
}

const UDungeonInteriorAsset* FDungeonInteriorDecorator::GetAsset() const
{
	return Asset;
}

void FDungeonInteriorDecorator::ClearDecorationLocation()
{
	mDecorationLocations.clear();
}

void FDungeonInteriorDecorator::AddDecorationLocation(const FVector& location, const double yaw)
{
	mDecorationLocations.push_back(std::make_shared<DecorationLocation>(location, yaw));
}

size_t FDungeonInteriorDecorator::GetDecorationLocationSize() const noexcept
{
	return mDecorationLocations.size();
}

void FDungeonInteriorDecorator::ShuffleDecorationLocation(dungeon::Random& random)
{
	size_t count = mDecorationLocations.size() / 2;
	while (count > 0)
	{
		const size_t a = random.Get<size_t>(mDecorationLocations.size());
		const size_t b = random.Get<size_t>(mDecorationLocations.size());
		if (a != b)
		{
			std::swap(mDecorationLocations[a], mDecorationLocations[b]);
		}
		--count;
	}
}

void FDungeonInteriorDecorator::SpawnActors(UWorld* world, const TArray<FString>& tags, dungeon::Random& random)
{
	for (const std::shared_ptr<DecorationLocation>& decorationLocation : mDecorationLocations)
	{
		const FTransform transform(FRotator(0, decorationLocation->Yaw, 0), decorationLocation->Position);
		SpawnActorsImplement(world, tags, transform, random, 0);
	}
}

void FDungeonInteriorDecorator::SpawnActorsImplement(UWorld* world, const TArray<FString>& tags, const FTransform& parentTransform, dungeon::Random& random, const size_t depth)
{
	if (depth >= 5)
		return;

	if (!::IsValid(world))
		return;

	// Select the parts to be placed
	const TArray<FDungeonInteriorParts> parts = Select(tags, random);
	if (parts.IsEmpty())
		return;

	const int32 partsIndex = random.Get(parts.Num());
	const FDungeonInteriorParts& selectedParts = parts[partsIndex];
	if (selectedParts.Class == nullptr)
	{
		DUNGEON_GENERATOR_LOG(TEXT("Set the class of interior parts"));
		return;
	}

	// Placement angle error
	double spawnYaw = 0;
	switch (selectedParts.DungeonAngleTypeOfPlacement)
	{
	case EDungeonAngleTypeOfPlacement::None:
		break;
	case EDungeonAngleTypeOfPlacement::Every180Degrees:
		if (random.Get<uint8_t>() & 1)
			spawnYaw = 180.;
		break;
	case EDungeonAngleTypeOfPlacement::Every90Degrees:
		spawnYaw = (static_cast<float>(random.Get<uint8_t>() & 3) * 90.f);
		break;
	case EDungeonAngleTypeOfPlacement::Free:
		spawnYaw = random.Get<float>(-180.f, 180.f);
		break;
	}
	const FQuat spawnRotation = FRotator(0, spawnYaw, 0).Quaternion();

	// Calculate placement transforms
	FTransform spawnTransform(spawnRotation);
	spawnTransform *= parentTransform;

	// Spawn actor
	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	actorSpawnParameters.bDeferConstruction = true;
	AActor* actor = world->SpawnActor(selectedParts.Class, &spawnTransform, actorSpawnParameters);
	if (!::IsValid(actor))
	{
		DUNGEON_GENERATOR_LOG(TEXT("Failed to spawn interior actor %s"), *selectedParts.Class->GetName());
		return;
	}

	actor->Tags.Add(CDungeonGeneratorCore::GetDungeonGeneratorTag());
#if WITH_EDITOR
	actor->SetFolderPath(TEXT("Dungeon/Interiors"));
#endif

	actor->FinishSpawning(spawnTransform, true);

	FBox bounds = actor->GetComponentsBoundingBox();
	if (!bounds.IsValid)
	{
		DUNGEON_GENERATOR_LOG(TEXT("Overlap with other collisions, destroy %s"), *actor->GetName());
		actor->Destroy();
		return;
	}

	{
		// Bounding box size away from the generation position
		FVector boundsCenter = bounds.GetCenter();
		const FVector& boundsExtent = bounds.GetExtent();
		const FVector& wallPosition = parentTransform.GetLocation();
		FVector movementDirection = (depth == 0)
			? boundsExtent * parentTransform.GetRotation().GetRightVector()
			: FVector::Zero();
		movementDirection.Z = boundsExtent.Z;

		// Placement position error
		if (selectedParts.PercentageOfErrorInPlacement > 0)
		{
			const float percentageOfErrorInPlacement = static_cast<float>(selectedParts.PercentageOfErrorInPlacement) / 100.f;
			boundsCenter.X += boundsExtent.X * (random.Get<float>() * percentageOfErrorInPlacement);
			boundsCenter.Y += boundsExtent.Y * (random.Get<float>() * percentageOfErrorInPlacement);
		}

		// Calculate new transform
		const FVector targetPosition = wallPosition + movementDirection;
		const FVector spawnOffset = targetPosition - boundsCenter;
		bounds = bounds.ShiftBy(spawnOffset);
	}

	{
		// Collision check
		FCollisionQueryParams queryParams(TEXT("FDungeonInteriorDecorator:SpawnActorsImplement"), false, actor);
		queryParams.bFindInitialOverlaps = true;
		if (selectedParts.OverlapCheck && world->OverlapBlockingTestByChannel(bounds.GetCenter(), spawnRotation, ECC_Visibility, FCollisionShape::MakeBox(bounds.GetExtent() - FVector(10)), queryParams))
		{
			DUNGEON_GENERATOR_LOG(TEXT("Overlap with other collisions, destroy %s"), *actor->GetName());
			actor->Destroy();
		}
		else
		{
			// Decorating Decorations (Fillables)
			// http://www.archmagerises.com/news/2021/6/12/how-to-procedurally-generate-and-decorate-3d-dungeon-rooms-in-unity-c

			if (selectedParts.AdditionalExtentToProhibitPlacement > 0)
			{
				bounds.Min.X -= selectedParts.AdditionalExtentToProhibitPlacement;
				bounds.Min.Y -= selectedParts.AdditionalExtentToProhibitPlacement;
				bounds.Max.X += selectedParts.AdditionalExtentToProhibitPlacement;
				bounds.Max.Y += selectedParts.AdditionalExtentToProhibitPlacement;
			}

			if (Intersects(bounds))
			{
				DUNGEON_GENERATOR_LOG(TEXT("Location of prohibited placement of surrounding interior actors, destroy %s"), *actor->GetName());
				actor->Destroy();
			}
			else
			{
				actor->SetActorLocation(bounds.GetCenter());

				Asset->EachInteriorLocation(actor, [this, world, &tags, &random, depth](UDungeonInteriorLocationComponent* component)
					{
						SpawnActorsImplement(world, component->GetInteriorTags(), component->GetComponentTransform(), random, depth + 1);
					}
				);

				mSensors.Emplace(bounds);
			}
		}
	}
}

TArray<FDungeonInteriorParts> FDungeonInteriorDecorator::Select(const TArray<FString>& interiorTags, dungeon::Random& random) const
{
	return ::IsValid(Asset) ? Asset->Select(interiorTags, random) : TArray<FDungeonInteriorParts>();
}

bool FDungeonInteriorDecorator::Intersects(const FBox& bounds) const
{
	for (const FBox& sensor : mSensors)
	{
		if (sensor.Intersect(bounds))
			return true;
	}

	return false;
}

std::shared_ptr<FDungeonInteriorDecorator> FDungeonInteriorDecorator::Room(const FBox& bounds) const
{
	std::shared_ptr<FDungeonInteriorDecorator> result = std::make_shared<FDungeonInteriorDecorator>(Asset);
	for (const auto& i : mDecorationLocations)
	{
		if (bounds.IsInsideOrOn(i->Position))
		{
			result->mDecorationLocations.push_back(i);
		}
	}
	return result;
}
