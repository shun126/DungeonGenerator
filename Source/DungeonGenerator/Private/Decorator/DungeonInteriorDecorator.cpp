/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Decorator/DungeonInteriorDecorator.h"
#include "DungeonGeneratorCore.h"
#include "Core/Math/Random.h"
#include "Core/Debug/Debug.h"
#include "Decorator/DungeonInteriorDatabase.h"
#include "Decorator/DungeonInteriorLocationComponent.h"
#include "Decorator/DungeonInteriorParts.h"
#include <utility>

void FDungeonInteriorDecorator::ShuffleDecorationLocation(const std::shared_ptr<dungeon::Random>& random)
{
	size_t count = mDecorationLocations.size() / 2;
	while (count > 0)
	{
		const size_t a = random->Get<size_t>(mDecorationLocations.size());
		const size_t b = random->Get<size_t>(mDecorationLocations.size());
		if (a != b)
		{
			std::swap(mDecorationLocations[a], mDecorationLocations[b]);
		}
		--count;
	}
}

void FDungeonInteriorDecorator::TrySpawnActors(UWorld* world, const TArray<FString>& tags, const std::shared_ptr<dungeon::Random>& random)
{
	auto i = mDecorationLocations.begin();
	while (i != mDecorationLocations.end())
	{
		const std::shared_ptr<DecorationLocation>& decorationLocation = *i;
		const FTransform transform(FRotator(0, decorationLocation->Yaw, 0), decorationLocation->Position);
		if (TrySpawnActorsImplement(world, tags, transform, random, 0))
			i = mDecorationLocations.erase(i);
		else
			++i;
	}
}

bool FDungeonInteriorDecorator::TrySpawnActorsImplement(UWorld* world, const TArray<FString>& tags, const FTransform& parentTransform, const std::shared_ptr<dungeon::Random>& random, const size_t depth)
{
	if (depth >= 5)
		return true;

	if (!::IsValid(world))
		return false;

	// Select the parts to be placed
	const TArray<FDungeonInteriorParts> parts = Select(tags, random);
	if (parts.Num() <= 0)
		return false;

	const int32 partsIndex = random->Get(parts.Num());
	const FDungeonInteriorParts& selectedParts = parts[partsIndex];
	if (selectedParts.Class == nullptr)
	{
		DUNGEON_GENERATOR_LOG(TEXT("Set the class of interior parts"));
		return false;
	}

	// Placement angle error
	double spawnYaw = 0;
	switch (selectedParts.DungeonAngleTypeOfPlacement)
	{
	case EDungeonAngleTypeOfPlacement::None:
		break;
	case EDungeonAngleTypeOfPlacement::Every180Degrees:
		if (random->Get<uint8_t>() & 1)
			spawnYaw = 180.;
		break;
	case EDungeonAngleTypeOfPlacement::Every90Degrees:
		spawnYaw = (static_cast<float>(random->Get<uint8_t>() & 3) * 90.f);
		break;
	case EDungeonAngleTypeOfPlacement::Free:
		spawnYaw = random->Get<float>(-180.f, 180.f);
		break;
	}
	const FQuat spawnRotation = FRotator(0, spawnYaw, 0).Quaternion();

	// Calculate placement transforms
	FTransform spawnTransform(spawnRotation);
	spawnTransform *= parentTransform;

	// in a unplaceable bounds, interrupt.
	if (IsInsideOrOnUnplacatableBounds(spawnTransform.GetLocation()))
		return false;

	// Spawn actor
	FActorSpawnParameters actorSpawnParameters;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	//actorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	actorSpawnParameters.bDeferConstruction = true;
	AActor* actor = world->SpawnActor(selectedParts.Class, &spawnTransform, actorSpawnParameters);
	if (!::IsValid(actor))
	{
		DUNGEON_GENERATOR_LOG(TEXT("Failed to spawn interior actor %s"), *selectedParts.Class->GetName());
		return false;
	}

	actor->Tags.Add(CDungeonGeneratorCore::GetDungeonGeneratorTag());
#if WITH_EDITOR
	actor->SetFolderPath(TEXT("Dungeon/Interiors"));
#endif

	actor->FinishSpawning(spawnTransform, true);

	/*
	NOTE:I would like to know how to get the bounding box of the mesh before spawning
	*/
	FBox bounds = actor->GetComponentsBoundingBox();
	if (!bounds.IsValid)
	{
		DUNGEON_GENERATOR_LOG(TEXT("GetComponentsBoundingBox failed, %s"), *actor->GetName());
		bounds.Min = actor->GetActorLocation() - FVector(100, 100, 100);
		bounds.Max = actor->GetActorLocation() + FVector(100, 100, 100);
	}

	// in a unplaceable bounds, interrupt.
	if (IntersectUnplacatableBounds(bounds))
		return false;

	{
		// Bounding box size away from the generation position
		FVector boundsCenter = bounds.GetCenter();
		const FVector& boundsExtent = bounds.GetExtent();
		const FVector& wallPosition = parentTransform.GetLocation();

		// Placement position error
		if (selectedParts.PercentageOfErrorInPlacement > 0)
		{
			const float percentageOfErrorInPlacement = static_cast<float>(selectedParts.PercentageOfErrorInPlacement) / 100.f;
			boundsCenter.X += boundsExtent.X * (random->Get<float>(-1.f, 1.f) * percentageOfErrorInPlacement);
			boundsCenter.Y += boundsExtent.Y * (random->Get<float>(-1.f, 1.f) * percentageOfErrorInPlacement);
		}

		// Move to avoid getting stuck in the around terrain
		FVector movementDirection;
		if (depth == 0)
		{
			movementDirection = boundsExtent * parentTransform.GetRotation().GetRightVector();

			// Collision check
			FCollisionQueryParams queryParams(TEXT("FDungeonInteriorDecorator:SpawnActorsImplement"), false, actor);
			FHitResult hitResult;

			const FVector end = wallPosition + movementDirection;
			const FVector start = end + FVector::UpVector * (bounds.Max.Z - bounds.Min.Z);
			if (world->LineTraceSingleByChannel(hitResult, start, end, ECC_Visibility, queryParams))
			{
				const auto delta = hitResult.ImpactPoint.Z - end.Z;
				movementDirection.Z += delta;
			}

			{
				const FVector endX = start + FVector::ForwardVector * (bounds.Max.X - bounds.Min.X);
				if (world->LineTraceSingleByChannel(hitResult, start, endX, ECC_Visibility, queryParams))
				{
					const auto delta = hitResult.ImpactPoint.X - endX.X;
					movementDirection.X = delta;
				}
			}
			{
				const FVector endX = start - FVector::ForwardVector * (bounds.Max.X - bounds.Min.X);
				if (world->LineTraceSingleByChannel(hitResult, start, endX, ECC_Visibility, queryParams))
				{
					const auto delta = hitResult.ImpactPoint.X - endX.X;
					movementDirection.X = delta;
				}
			}

			{
				const FVector endY = start + FVector::RightVector * (bounds.Max.Y - bounds.Min.Y);
				if (world->LineTraceSingleByChannel(hitResult, start, endY, ECC_Visibility, queryParams))
				{
					const auto delta = hitResult.ImpactPoint.Y - endY.Y;
					movementDirection.Y = delta;
				}
			}
			{
				const FVector endY = start - FVector::RightVector * (bounds.Max.Y - bounds.Min.Y);
				if (world->LineTraceSingleByChannel(hitResult, start, endY, ECC_Visibility, queryParams))
				{
					const auto delta = hitResult.ImpactPoint.Y - endY.Y;
					movementDirection.Y = delta;
				}
			}

			const auto pivotZ = boundsExtent.Z - (boundsCenter.Z - wallPosition.Z);
			movementDirection.Z += pivotZ;
		}
		else
		{
			const auto pivotZ = boundsExtent.Z - (boundsCenter.Z - wallPosition.Z);
			movementDirection.Set(0, 0, pivotZ);
		}

		// Calculate new transform
		const FVector targetPosition = wallPosition + movementDirection;
		const FVector spawnOffset = targetPosition - boundsCenter;
		bounds = bounds.ShiftBy(spawnOffset);
	}

	bool result = false;
	{
		// Collision check
		FCollisionQueryParams queryParams(TEXT("FDungeonInteriorDecorator:SpawnActorsImplement"), false, actor);
		queryParams.bFindInitialOverlaps = true;
		if (selectedParts.OverlapCheck && world->OverlapBlockingTestByChannel(bounds.GetCenter(), spawnRotation, ECC_Visibility, FCollisionShape::MakeBox(bounds.GetExtent() - FVector(10)), queryParams))
		{
			DUNGEON_GENERATOR_LOG(TEXT("Overlap with other collisions, destroy %s"), *actor->GetName());
			actor->Destroy();
			result = false;
		}
		else
		{
			// Decorating Decorations (Fillables)
			// http://www.archmagerises.com/news/2021/6/12/how-to-procedurally-generate-and-decorate-3d-dungeon-rooms-in-unity-c

			const FVector boundsCenter = bounds.GetCenter();

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
				result = false;
			}
			else
			{
				USceneComponent* rootComponent = actor->GetRootComponent();
				if (::IsValid(rootComponent))
				{
					const EComponentMobility::Type mobility = rootComponent->Mobility;
					rootComponent->SetMobility(EComponentMobility::Movable);
					actor->SetActorLocation(boundsCenter);
					rootComponent->SetMobility(mobility);
				}
				else
				{
					actor->SetActorLocation(boundsCenter);
				}

				Asset->EachInteriorLocation(actor, [this, world, random, depth](UDungeonInteriorLocationComponent* component)
					{
						TrySpawnActorsImplement(world, component->GetInquireInteriorTags(), component->GetComponentTransform(), random, depth + 1);
					}
				);

				mPlacedBounds.Emplace(bounds);
				result = true;
			}
		}
	}
	return result;
}

TArray<FDungeonInteriorParts> FDungeonInteriorDecorator::Select(const TArray<FString>& interiorTags, const std::shared_ptr<dungeon::Random>& random) const
{
	return ::IsValid(Asset) ? Asset->Select(interiorTags, random) : TArray<FDungeonInteriorParts>();
}

bool FDungeonInteriorDecorator::Intersects(const FBox& bounds) const
{
	return std::any_of(mPlacedBounds.begin(), mPlacedBounds.end(), [&bounds](const FBox& sensor)
		{
			return sensor.Intersect(bounds);
		}
	);
}

std::shared_ptr<FDungeonInteriorDecorator> FDungeonInteriorDecorator::CreateInteriorDecorator(const FBox& placeableBounds) const
{
	std::shared_ptr<FDungeonInteriorDecorator> result = std::make_shared<FDungeonInteriorDecorator>(
		Asset,
		mUnplaceableBounds
	);
	for (const auto& i : mDecorationLocations)
	{
		if (placeableBounds.IsInsideOrOn(i->Position))
		{
			result->mDecorationLocations.push_back(i);
		}
	}

	return result;
}
