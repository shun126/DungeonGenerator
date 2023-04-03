/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGenerateParameter.h"
#include "DungeonRoomAsset.h"
#include "PluginInfomation.h"
#include "Core/Debug/BuildInfomation.h"
#include "../Private/Core/Direction.h"
#include "../Private/Core/Grid.h"
#include "../Private/Core/Math/Random.h"

#if WITH_EDITOR
#include <JsonUtilities/Public/JsonUtilities.h>
#include <Runtime/Json/Public/Serialization/JsonReader.h>
#endif

namespace
{
	static FTransform CalculateWorldTransform_(const FTransform& toWorldTransform, const FTransform& relativeTransform)
	{
		return relativeTransform * toWorldTransform;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FTransform FDungeonPartsTransform::CalculateWorldTransform(const FTransform& transform) const noexcept
{
	return CalculateWorldTransform_(transform, RelativeTransform);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept
{
	const FTransform transform(rotator, position);
	return CalculateWorldTransform(transform);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const float yaw) const noexcept
{
	const FRotator rotator(0, yaw, 0);
	return CalculateWorldTransform(position, rotator);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return CalculateWorldTransform(position, direction.ToDegree());
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(dungeon::Random& random, const FTransform& transform, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	if (static_cast<uint8>(placementDirection) < static_cast<uint8>(EDungeonPartsPlacementDirection::West))
	{
		const FRotator rotator(0., static_cast<double>(placementDirection) * 90., 0.);
		FTransform result(transform);
		result.SetRotation(rotator.Quaternion());
		return CalculateWorldTransform_(result, RelativeTransform);
	}
	else if (placementDirection == EDungeonPartsPlacementDirection::RandomDirection)
	{
		const FRotator rotator(0., static_cast<double>(random.Get<uint8_t>(4)) * 90., 0.);
		FTransform result(transform);
		result.SetRotation(rotator.Quaternion());
		return CalculateWorldTransform_(result, RelativeTransform);
	}
	else if (placementDirection == EDungeonPartsPlacementDirection::FollowGridDirection)
	{
		return CalculateWorldTransform_(transform, RelativeTransform);
	}

	// Error if you have come here
	return transform;
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	const FTransform transform(rotator, position);
	return CalculateWorldTransform(random, transform, placementDirection);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	const FRotator rotator(0, yaw, 0);
	return CalculateWorldTransform(random, position, rotator, placementDirection);
}

FTransform FDungeonPartsTransform::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction, const EDungeonPartsPlacementDirection placementDirection) const noexcept
{
	return CalculateWorldTransform(random, position, direction.ToDegree(), placementDirection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, transform, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, rotator, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, yaw, PlacementDirection);
}

FTransform FDungeonActorPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, direction, PlacementDirection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, transform, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, rotator, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, yaw, PlacementDirection);
}

FTransform FDungeonMeshPartsWithDirection::CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept
{
	return FDungeonPartsTransform::CalculateWorldTransform(random, position, direction, PlacementDirection);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UDungeonGenerateParameter::UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UDungeonGenerateParameter::SelectDungeonMeshPartsIndex(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const int32 size, const EDungeonPartsSelectionMethod partsSelectionMethod) const
{
	switch (partsSelectionMethod)
	{
	case EDungeonPartsSelectionMethod::GridIndex:
		return gridIndex % size;

	case EDungeonPartsSelectionMethod::Direction:
		return grid.GetDirection().Get() % size;

	case EDungeonPartsSelectionMethod::Random:
		return random.Get<uint32_t>(size);

	default:
		return gridIndex % size;
	}
}

// aka: SelectParts, SelectRandomActorParts
FDungeonActorParts* UDungeonGenerateParameter::SelectActorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	FDungeonActorParts* actorParts = const_cast<FDungeonActorParts*>(&parts[index]);
	return IsValid(actorParts->ActorClass) ? actorParts : nullptr;
}

// aka: SelectParts, SelectActorParts
FDungeonRandomActorParts* UDungeonGenerateParameter::SelectRandomActorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonRandomActorParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	const int32 index = SelectDungeonMeshPartsIndex(gridIndex, grid, random, size, partsSelectionMethod);
	FDungeonRandomActorParts* actorParts = const_cast<FDungeonRandomActorParts*>(&parts[index]);
	if (!IsValid(actorParts->ActorClass))
		return nullptr;

	const float value = random.Get<float>();
	if (value > actorParts->Frequency)
		return nullptr;

	return actorParts;
}

int32 UDungeonGenerateParameter::GetRandomSeed() const
{
	return RandomSeed;
}

int32 UDungeonGenerateParameter::GetGeneratedRandomSeed() const
{
	return GeneratedRandomSeed;
}

void UDungeonGenerateParameter::SetGeneratedRandomSeed(const int32 generatedRandomSeed)
{
	GeneratedRandomSeed = generatedRandomSeed;
}

int32 UDungeonGenerateParameter::GetNumberOfCandidateRooms() const
{
	return NumberOfCandidateRooms;
}

const FInt32Interval& UDungeonGenerateParameter::GetRoomWidth() const noexcept
{
	return RoomWidth;
}

const FInt32Interval& UDungeonGenerateParameter::GetRoomDepth() const noexcept
{
	return RoomDepth;
}

const FInt32Interval& UDungeonGenerateParameter::GetRoomHeight() const noexcept
{
	return RoomHeight;
}

int32 UDungeonGenerateParameter::GetHorizontalRoomMargin() const noexcept
{
	return RoomMargin;
}

int32 UDungeonGenerateParameter::GetVerticalRoomMargin() const noexcept
{
	return VerticalRoomMargin;
}

float UDungeonGenerateParameter::GetGridSize() const
{
	return GridSize;
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, FloorParts, FloorPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachParts(FloorParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, SlopeParts, SloopPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachParts(SlopeParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, WallParts, WallPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachParts(WallParts, func);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectRoomRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, RoomRoofParts, RoomRoofPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachRoomRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachParts(RoomRoofParts, func);
}

const FDungeonMeshPartsWithDirection* UDungeonGenerateParameter::SelectAisleRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, AisleRoofParts, AisleRoofPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachAisleRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachParts(AisleRoofParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, PillarParts, PillarPartsSelectionMethod);
}

void UDungeonGenerateParameter::EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachParts(PillarParts, func);
}

const FDungeonRandomActorParts* UDungeonGenerateParameter::SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectRandomActorParts(gridIndex, grid, random, TorchParts, TorchPartsSelectionMethod);
}

#if 0
const FDungeonActorParts* UDungeonGenerateParameter::SelectChandelierParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectRandomActorParts(gridIndex, grid, random, ChandelierParts, ChandelierPartsSelectionMethod);
}
#endif

const FDungeonDoorActorParts* UDungeonGenerateParameter::SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const
{
	return SelectParts(gridIndex, grid, random, DoorParts, DoorPartsSelectionMethod);
}

const FDungeonActorPartsWithDirection& UDungeonGenerateParameter::GetStartParts() const
{
	return StartParts;
}

const FDungeonActorPartsWithDirection& UDungeonGenerateParameter::GetGoalParts() const
{
	return GoalParts;
}

void UDungeonGenerateParameter::EachDungeonRoomLocator(std::function<void(const FDungeonRoomLocator&)> func) const
{
	if (IsValid(DungeonRoomAsset))
	{
		for (const FDungeonRoomLocator& dungeonRoomLocator : DungeonRoomAsset->GetDungeonRoomLocator())
		{
			if (dungeonRoomLocator.GetLevelPath().IsValid())
			{
				func(dungeonRoomLocator);
			}
		}
	}
}

UClass* UDungeonGenerateParameter::GetRoomSensorClass() const
{
	return DungeonRoomSensorClass;
}

/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const FIntVector& location) const
{
	return FVector(
		static_cast<float>(location.X),
		static_cast<float>(location.Y),
		static_cast<float>(location.Z)
	) * GridSize;
}

/*
2D空間は（X軸:前 Y軸:右）
3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
*/
FVector UDungeonGenerateParameter::ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const
{
	return FVector(
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(z)
	) * GridSize;
}

FIntVector UDungeonGenerateParameter::ToGrid(const FVector& location) const
{
	return FIntVector(
		static_cast<int32>(location.X / GridSize),
		static_cast<int32>(location.Y / GridSize),
		static_cast<int32>(location.Z / GridSize)
	);
}

#if WITH_EDITOR
void UDungeonGenerateParameter::DumpToJson() const
{
	// TSharedPtr<FJsonObject> jsonRoot = MakeShareable(new FJsonObject);
	// 
	// FString outPutString;
	// TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&outPutString);
	// FJsonSerializer::Serialize(jsonRoot.ToSharedRef(), writer);

	FString jsonString(TEXT("{\n"));
	{
		jsonString += TEXT("RandomSeed:") + FString::FromInt(RandomSeed) + TEXT(",\n");
		jsonString += TEXT("GeneratedRandomSeed:") + FString::FromInt(GeneratedRandomSeed) + TEXT(",\n");
		jsonString += TEXT("NumberOfCandidateRooms:") + FString::FromInt(NumberOfCandidateRooms) + TEXT(",\n");
		jsonString += TEXT("RoomWidth:{\n");
		jsonString += TEXT("Min:") + FString::FromInt(RoomWidth.Min) + TEXT(",\n");
		jsonString += TEXT("Max:") + FString::FromInt(RoomWidth.Max);
		jsonString += TEXT("},");
		jsonString += TEXT("RoomDepth:{\n");
		jsonString += TEXT("Min:") + FString::FromInt(RoomDepth.Min) + TEXT(",\n");
		jsonString += TEXT("Max:") + FString::FromInt(RoomDepth.Max);
		jsonString += TEXT("},");
		jsonString += TEXT("RoomHeight:{\n");
		jsonString += TEXT("Min:") + FString::FromInt(RoomHeight.Min) + TEXT(",\n");
		jsonString += TEXT("Max:") + FString::FromInt(RoomHeight.Max);
		jsonString += TEXT("},");
		jsonString += TEXT("RoomMargin:") + FString::FromInt(RoomMargin) + TEXT(",\n");
		jsonString += TEXT("MergeRooms:");
		if(MergeRooms)
			jsonString += TEXT("true,\n");
		else
			jsonString += TEXT("false,\n");
		jsonString += TEXT("GridSize:") + FString::SanitizeFloat(GridSize) + TEXT(",\n");
		// TArray<FDungeonMeshPartsWithDirection> FloorParts;
		// TArray<FDungeonMeshParts> SlopeParts;
		// TArray<FDungeonMeshParts> WallParts;
		// TArray<FDungeonMeshPartsWithDirection> RoomRoofParts;
		// TArray<FDungeonMeshPartsWithDirection> AisleRoofParts;
		// TArray<FDungeonMeshParts> PillarParts;
		// TArray<FDungeonRandomActorParts> TorchParts;
		// TArray<FDungeonRandomActorParts> ChandelierParts;
		// TArray<FDungeonActorParts> DoorParts;
		// FDungeonActorPartsWithDirection StartParts;
		// FDungeonActorPartsWithDirection GoalParts;
		// UClass* DungeonRoomSensorClass = ADungeonRoomSensor::StaticClass();
		// TArray<UDungeonRoomAsset*> DungeonRoomAsset;
		jsonString += TEXT("Version:\"DUNGENERATOR_PLUGIN_VERSION_NAME\",\n");
		jsonString += TEXT("Beta:\"DUNGENERATOR_PLUGIN_BETA_VERSION\",\n");
		jsonString += TEXT("Tag:\"JENKINS_JOB_TAG\",\n");
		jsonString += TEXT("UUID:\"JENKINS_UUID\",\n");
		jsonString += TEXT("License:\"JENKINS_LICENSE\"\n");
	}
	jsonString += TEXT("}");
	
	const FString fileName(GetName() + ".json");
	const FString filePath(GetJsonDefaultDirectory() / fileName);
	FFileHelper::SaveStringToFile(jsonString, *filePath);
}

FString UDungeonGenerateParameter::GetJsonDefaultDirectory() const
{
	return FString(FPaths::ProjectSavedDir() / TEXT("DungeonGenerator"));
}
#endif
