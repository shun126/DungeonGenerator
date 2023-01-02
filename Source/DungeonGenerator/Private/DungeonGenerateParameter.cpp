/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonGenerateParameter.h"
#include "DungeonRoomAsset.h"

namespace
{
	static FTransform GetWorldTransform_(const FTransform& toWorldTransform, const FTransform& relativeTransform)
	{
		return relativeTransform * toWorldTransform;
	}
}

FTransform FDungeonMeshParts::GetWorldTransform(const FTransform& toWorldTransform) const
{
	return GetWorldTransform_(toWorldTransform, RelativeTransform);
}

FTransform FDungeonMeshParts::GetWorldTransform(const float yaw, const FVector& position) const
{
	const FTransform toWorldTransform(FRotator(0.f, yaw, 0.f).Quaternion(), position);
	return GetWorldTransform(toWorldTransform);
}

FTransform FDungeonActorParts::GetWorldTransform(const FTransform& toWorldTransform) const
{
	return GetWorldTransform_(toWorldTransform, RelativeTransform);
}

FTransform FDungeonActorParts::GetWorldTransform(const float yaw, const FVector& position) const
{
	const FTransform toWorldTransform(FRotator(0.f, yaw, 0.f).Quaternion(), position);
	return GetWorldTransform(toWorldTransform);
}

FTransform FDungeonObjectParts::GetWorldTransform(const FTransform& toWorldTransform) const
{
	return GetWorldTransform_(toWorldTransform, RelativeTransform);
}

FTransform FDungeonObjectParts::GetWorldTransform(const float yaw, const FVector& position) const
{
	const FTransform toWorldTransform(FRotator(0.f, yaw, 0.f).Quaternion(), position);
	return GetWorldTransform(toWorldTransform);
}

UClass* FDungeonObjectParts::GetActorClass()
{
	if (IsValid(ActorClass) == false && ActorPath.IsValid() == true)
	{
		ActorClass = TSoftClassPtr<AActor>(FSoftObjectPath(ActorPath.ToString() + "_C")).LoadSynchronous();
	}
	return ActorClass;
}


UDungeonGenerateParameter::UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FDungeonMeshParts* UDungeonGenerateParameter::SelectDungeonMeshParts(dungeon::Random& random, const TArray<FDungeonMeshParts>& parts) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;
	const int32 index = random.Get<uint32_t>() % size;
	return const_cast<FDungeonMeshParts*>(&parts[index]);
}

void UDungeonGenerateParameter::EachMeshParts(const TArray<FDungeonMeshParts>& parts, std::function<void(const FDungeonMeshParts&)> func) const
{
	for (const FDungeonMeshParts& part : parts)
	{
		func(part);
	}
}

FDungeonObjectParts* UDungeonGenerateParameter::SelectDungeonObjectParts(dungeon::Random& random, const TArray<FDungeonObjectParts>& parts) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;
	const int32 index = random.Get<uint32_t>() % size;
	FDungeonObjectParts* actorParts = const_cast<FDungeonObjectParts*>(&parts[index]);
	return IsValid(actorParts->GetActorClass()) ? actorParts : nullptr;
}

FDungeonRandomObjectParts* UDungeonGenerateParameter::SelectDungeonRandomObjectParts(dungeon::Random& random, const TArray<FDungeonRandomObjectParts>& parts) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;
	const int32 index = random.Get<uint32_t>() % size;
	FDungeonRandomObjectParts* actorParts = const_cast<FDungeonRandomObjectParts*>(&parts[index]);
	if (!IsValid(actorParts->GetActorClass()))
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

int32 UDungeonGenerateParameter::GetUnderfloorHeight() const
{
	return UnderfloorHeight;
}

int32 UDungeonGenerateParameter::GetRoomMargin() const
{
	return RoomMargin;
}

float UDungeonGenerateParameter::GetGridSize() const
{
	return GridSize;
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectFloorParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, FloorParts);
}

void UDungeonGenerateParameter::EachFloorParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(FloorParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectSlopeParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, SlopeParts);
}

void UDungeonGenerateParameter::EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(SlopeParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectWallParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, WallParts);
}

void UDungeonGenerateParameter::EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(WallParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectRoomRoofParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, RoomRoofParts);
}

void UDungeonGenerateParameter::EachRoomRoofParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(RoomRoofParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectAisleRoofParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, AisleRoofParts);
}

void UDungeonGenerateParameter::EachAisleRoofParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(AisleRoofParts, func);
}

const FDungeonMeshParts* UDungeonGenerateParameter::SelectPillarParts(dungeon::Random& random) const
{
	return SelectDungeonMeshParts(random, PillarParts);
}

void UDungeonGenerateParameter::EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachMeshParts(PillarParts, func);
}

const FDungeonRandomObjectParts* UDungeonGenerateParameter::SelectTorchParts(dungeon::Random& random) const
{
	return SelectDungeonRandomObjectParts(random, TorchParts);
}
#if 0
const FDungeonObjectParts* UDungeonGenerateParameter::SelectChandelierParts(dungeon::Random& random) const
{
	return SelectDungeonRandomObjectParts(random, ChandelierParts);
}
#endif
const FDungeonObjectParts* UDungeonGenerateParameter::SelectDoorParts(dungeon::Random& random) const
{
	return SelectDungeonObjectParts(random, DoorParts);
}

const FDungeonActorParts& UDungeonGenerateParameter::GetStartParts() const
{
	return StartParts;
}

const FDungeonActorParts& UDungeonGenerateParameter::GetGoalParts() const
{
	return GoalParts;
}

void UDungeonGenerateParameter::EachDungeonRoomAsset(std::function<void(const UDungeonRoomAsset*)> func) const
{
	for (const UDungeonRoomAsset* dungeonRoomAsset : DungeonRoomAssets)
	{
		if (IsValid(dungeonRoomAsset) && dungeonRoomAsset->LevelName.IsValid())
			func(dungeonRoomAsset);
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
