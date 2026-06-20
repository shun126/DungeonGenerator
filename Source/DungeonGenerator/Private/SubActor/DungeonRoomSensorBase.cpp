/**
 * @author		Shun Moriya
 * @copyright	2023- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonRoomSensorBase.h"
#include "SubActor/DungeonDoorBase.h"
#include "DungeonGenerateBase.h"
#include "Core/Debug/Debug.h"
#include "Core/Helper/Crc.h"
#include "Core/Math/Random.h"
#include <Components/BoxComponent.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <cmath>

#include "Core/Math/Math.h"

#if WITH_EDITOR
#include <DrawDebugHelpers.h>

namespace
{
	static bool ForceShowDebugInformation = false;
}
#endif

const FName& ADungeonRoomSensorBase::GetDungeonGeneratorTag()
{
	return ADungeonGenerateBase::GetDungeonGeneratorTag();
}

const TArray<FName>& ADungeonRoomSensorBase::GetDungeonGeneratorTags()
{
	static const TArray<FName> tags = {
		GetDungeonGeneratorTag()
	};
	return tags;
}

ADungeonRoomSensorBase::ADungeonRoomSensorBase(const FObjectInitializer& initializer)
	: Super(initializer)
	, RoomSize(ForceInit)
	, mLocalRandom(std::make_shared<dungeon::Random>())
{
#if WITH_EDITOR
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;
#endif

	// レプリケーションしない前提のアクター
	bReplicates = false;
	SetReplicateMovement(false);

	// 進入検出センサー
	Bounding = initializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("Sensor"));
	check(IsValid(Bounding));
	Bounding->SetMobility(EComponentMobility::Static);
	Bounding->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Bounding->SetCollisionObjectType(ECC_WorldStatic);
	for (size_t i = 0; i < ECC_MAX; ++i)
	{
		if (static_cast<ECollisionChannel>(i) == ECC_Pawn)
			Bounding->SetCollisionResponseToChannel(static_cast<ECollisionChannel>(i), ECR_Overlap);
		else
			Bounding->SetCollisionResponseToChannel(static_cast<ECollisionChannel>(i), ECR_Ignore);
	}
	Bounding->SetGenerateOverlapEvents(true);
	SetRootComponent(Bounding);

}


int32 ADungeonRoomSensorBase::GetIdentifier() const noexcept
{
	return Identifier;
}

bool ADungeonRoomSensorBase::OnPrepare_Implementation(const float depthFromStartRatio)
{
	return true;
}

UBoxComponent* ADungeonRoomSensorBase::GetBounding()
{
	return Bounding;
}

const UBoxComponent* ADungeonRoomSensorBase::GetBounding() const
{
	return Bounding;
}

const FBox& ADungeonRoomSensorBase::GetRoomSize() const noexcept
{
	return RoomSize;
}

FDungeonGeneratedRoomInfo ADungeonRoomSensorBase::GetGeneratedRoomInfo() const noexcept
{
	return RoomInfo;
}

uint8 ADungeonRoomSensorBase::GetDoorAddingProbability() const noexcept
{
	return DoorAddingProbability;
}

void ADungeonRoomSensorBase::SetDoorAddingProbability(const uint8 doorAddingProbability) noexcept
{
	DoorAddingProbability = doorAddingProbability;
}

bool ADungeonRoomSensorBase::OnNativePrepare(const FVector& center)
{
	return true;
}

void ADungeonRoomSensorBase::OnNativeInitialize()
{
}

void ADungeonRoomSensorBase::OnNativeFinalize(const bool finish)
{
}

void ADungeonRoomSensorBase::OnNativeReset(const bool fallToAbyss)
{
}

void ADungeonRoomSensorBase::OnNativeResume()
{
}

void ADungeonRoomSensorBase::BeginPlay()
{
	Super::BeginPlay();

	Bounding->OnComponentBeginOverlap.AddDynamic(this, &ADungeonRoomSensorBase::OnBeginOverlap);
	Bounding->OnComponentEndOverlap.AddDynamic(this, &ADungeonRoomSensorBase::OnEndOverlap);
}

void ADungeonRoomSensorBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Bounding->OnComponentBeginOverlap.RemoveDynamic(this, &ADungeonRoomSensorBase::OnBeginOverlap);
	Bounding->OnComponentEndOverlap.RemoveDynamic(this, &ADungeonRoomSensorBase::OnEndOverlap);

	Super::EndPlay(EndPlayReason);
}

void ADungeonRoomSensorBase::BeginDestroy()
{
	InvokeFinalize();

	Super::BeginDestroy();
}

#if WITH_EDITOR
void ADungeonRoomSensorBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// cppcheck-suppress [knownConditionTrueFalse]
	if ((ShowDebugInformation && mOverlapCount > 0) || ForceShowDebugInformation)
	{
		TArray<FString> output;
		output.Add(TEXT("Identifier:") + FString::FromInt(Identifier));
		output.Add(TEXT("Parts:") + GetDungeonRoomPartsName(Parts));
		output.Add(TEXT("Item:") + GetDungeonRoomItemName(Item));
		output.Add(TEXT("StructuralRole:") + StaticEnum<EDungeonRoomStructuralRole>()->GetNameStringByValue(static_cast<int64>(RoomInfo.RoomStructuralRole)));
		output.Add(TEXT("GameplayRole:") + StaticEnum<EDungeonRoomGameplayRole>()->GetNameStringByValue(static_cast<int64>(RoomInfo.RoomGameplayRole)));
		output.Add(FString(TEXT("SecretRoom:")) + (RoomInfo.bSecretRoom ? TEXT("On") : TEXT("Off")));
		output.Add(FString(TEXT("DeadEndRoom:")) + (RoomInfo.bDeadEndRoom ? TEXT("On") : TEXT("Off")));
		output.Add(FString(TEXT("MainPathRoom:")) + (RoomInfo.bMainPathRoom ? TEXT("On") : TEXT("Off")));
		output.Add(FString(TEXT("LockedRouteRoom:")) + (RoomInfo.bLockedRouteRoom ? TEXT("On") : TEXT("Off")));
		output.Add(TEXT("BranchId:") + FString::FromInt(BranchId));
		output.Add(TEXT("DepthFromStart:") + FString::FromInt(DepthFromStart));
		output.Add(FString(TEXT("AutoReset:")) + (AutoReset ? TEXT("On") : TEXT("Off")));
		output.Add(TEXT("DoorAddingProbability:") + FString::FromInt(DoorAddingProbability));
		output.Add(TEXT("Doors:") + FString::FromInt(DungeonDoors.Num()));
		output.Add(TEXT("Torches:") + FString::FromInt(DungeonTorches.Num()));
		output.Add(TEXT("Chandeliers:") + FString::FromInt(DungeonChandeliers.Num()));

		FString message;
		for (const FString& line : output)
		{
			message.Append(line);
			message.Append(TEXT("\n"));
		}
		
		DrawDebugString(GetWorld(), GetActorLocation(), message, nullptr, FColor::White, 0, true, 1.f);
	}
}
#endif

bool ADungeonRoomSensorBase::InvokePrepare(
	const std::shared_ptr<dungeon::Random>& random,
	const int32 identifier,
	const FVector& center,
	const FVector& extents,
	const float horizontalGridSize,
	const EDungeonRoomParts parts,
	const EDungeonRoomItem item,
	const FDungeonGeneratedRoomInfo& roomInfo,
	const uint8 branchId,
	const uint8 depthFromStart,
	const uint8 deepestDepthFromStart)
{
	mSynchronizedRandom.SetOwner(random);
	mLocalRandom.SetSeed(random->Get<uint32_t>());

	const FVector margin(HorizontalMargin, VerticalMargin, HorizontalMargin);
	Bounding->SetBoxExtent(extents + margin);
	RoomSize = FBox::BuildAABB(Bounding->GetComponentLocation(), extents);

	mHorizontalGridSize = horizontalGridSize;
	Identifier = identifier;
	Parts = parts;
	Item = item;
	RoomInfo = roomInfo;
	BranchId = branchId;
	DepthFromStart = depthFromStart;
	DeepestDepthFromStart = deepestDepthFromStart;

	const float depthFromStartRatio = DeepestDepthFromStart > 0 ?
		static_cast<float>(DepthFromStart) / static_cast<float>(DeepestDepthFromStart) :
		0.0f;

	return OnNativePrepare(center) && OnPrepare(depthFromStartRatio);
}

void ADungeonRoomSensorBase::InvokeInitialize()
{
	if (mState != State::Initialized)
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Initialize"), *GetName());

		{
			if (Item == EDungeonRoomItem::Key)
			{
				DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Key spawned."), *GetName());
				if (AActor* spawnedActor = SpawnActorInRoomImpl(SpawnKeyActor, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
					DungeonRoomSpawnedActors.Add(spawnedActor);
			}
			if (Item == EDungeonRoomItem::UniqueKey)
			{
				DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Unique key spawned."), *GetName());
				if (AActor* spawnedActor = SpawnActorInRoomImpl(SpawnUniqueKeyActor, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
					DungeonRoomSpawnedActors.Add(spawnedActor);
			}

			SpawnActorsInRoomImpl();
		}

		{
			const float depthFromStartRatio = DeepestDepthFromStart > 0 ?
				static_cast<float>(DepthFromStart) / static_cast<float>(DeepestDepthFromStart) :
				0.0f;
			RoomInfo.Identifier = Identifier;
			RoomInfo.Parts = Parts;
			RoomInfo.Item = Item;
			RoomInfo.BranchId = BranchId;
			RoomInfo.DepthFromStart = DepthFromStart;
			RoomInfo.DeepestDepthFromStart = DeepestDepthFromStart;
			RoomInfo.DepthFromStartRatio = depthFromStartRatio;
			RoomInfo.bHasLockedDoor = HasLockedDoor();
			OnNativeInitialize();
			OnInitialize(RoomInfo);
		}

		mState = State::Initialized;
	}
}

void ADungeonRoomSensorBase::InvokeFinalize()
{
	if (mState == State::Initialized)
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Finalize"), *GetName());
		OnNativeFinalize(true);
		OnFinalize(true);
		mSynchronizedRandom.ResetOwner();
		mState = State::Finalized;
	}
}

void ADungeonRoomSensorBase::InvokeReset(const bool fallToAbyss)
{
	if (mState == State::Initialized && AutoReset == true)
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Reset"), *GetName());
		OnNativeReset(fallToAbyss);
		OnReset(fallToAbyss);
	}
}

void ADungeonRoomSensorBase::InvokeResume()
{
	if (mState == State::Initialized && AutoReset == true)
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Resume"), *GetName());
		OnNativeResume();
		OnResume();
	}
}

void ADungeonRoomSensorBase::AddDungeonDoor(ADungeonDoorBase* dungeonDoorBase)
{
	DungeonDoors.Add(dungeonDoorBase);
}

bool ADungeonRoomSensorBase::HasLockedDoor() const
{
	for (const auto& door : DungeonDoors)
	{
		if (IsValid(door) && door->IsLockedDoor())
			return true;
	}
	return false;
}

void ADungeonRoomSensorBase::AddDungeonTorch(AActor* actor)
{
	DungeonTorches.Add(actor);
}

void ADungeonRoomSensorBase::AddDungeonChandelier(AActor* actor)
{
	DungeonChandeliers.Add(actor);
}

float ADungeonRoomSensorBase::GetDefaultGameplayRoleEnemySpawnMultiplier(const EDungeonRoomGameplayRole role) noexcept
{
	switch (role)
	{
	case EDungeonRoomGameplayRole::None:
		return 0.5f;
	case EDungeonRoomGameplayRole::Combat:
		return 1.0f;
	case EDungeonRoomGameplayRole::Treasure:
		return 0.8f;
	case EDungeonRoomGameplayRole::Puzzle:
		return 0.5f;
	case EDungeonRoomGameplayRole::Rest:
		return 0.0f;
	case EDungeonRoomGameplayRole::Boss:
		return 2.0f;
	case EDungeonRoomGameplayRole::Secret:
		return 0.7f;
	default:
		return 1.0f;
	}
}

float ADungeonRoomSensorBase::GetDefaultStructuralRoleEnemySpawnMultiplier(const EDungeonRoomStructuralRole role) noexcept
{
	switch (role)
	{
	case EDungeonRoomStructuralRole::Start:
	case EDungeonRoomStructuralRole::Goal:
		return 0.0f;
	case EDungeonRoomStructuralRole::Hub:
	case EDungeonRoomStructuralRole::Connector:
	case EDungeonRoomStructuralRole::Branch:
	case EDungeonRoomStructuralRole::DeadEnd:
	default:
		return 1.0f;
	}
}

float ADungeonRoomSensorBase::FindGameplayRoleEnemySpawnMultiplier(const FDungeonGameplayRoleEnemySpawnMultipliers& multipliers, const EDungeonRoomGameplayRole role) noexcept
{
	switch (role)
	{
	case EDungeonRoomGameplayRole::None:
		return FMath::Max(0.f, multipliers.None_);
	case EDungeonRoomGameplayRole::Combat:
		return FMath::Max(0.f, multipliers.Combat_);
	case EDungeonRoomGameplayRole::Treasure:
		return FMath::Max(0.f, multipliers.Treasure_);
	case EDungeonRoomGameplayRole::Puzzle:
		return FMath::Max(0.f, multipliers.Puzzle_);
	case EDungeonRoomGameplayRole::Rest:
		return FMath::Max(0.f, multipliers.Rest_);
	case EDungeonRoomGameplayRole::Boss:
		return FMath::Max(0.f, multipliers.Boss_);
	case EDungeonRoomGameplayRole::Secret:
		return FMath::Max(0.f, multipliers.Secret_);
	default:
		return GetDefaultGameplayRoleEnemySpawnMultiplier(role);
	}
}

float ADungeonRoomSensorBase::FindStructuralRoleEnemySpawnMultiplier(const FDungeonStructuralRoleEnemySpawnMultipliers& multipliers, const EDungeonRoomStructuralRole role) noexcept
{
	switch (role)
	{
	case EDungeonRoomStructuralRole::Start:
		return FMath::Max(0.f, multipliers.Start);
	case EDungeonRoomStructuralRole::Goal:
		return FMath::Max(0.f, multipliers.Goal);
	case EDungeonRoomStructuralRole::Hub:
		return FMath::Max(0.f, multipliers.Hub);
	case EDungeonRoomStructuralRole::Connector:
		return FMath::Max(0.f, multipliers.Connector);
	case EDungeonRoomStructuralRole::Branch:
		return FMath::Max(0.f, multipliers.Branch);
	case EDungeonRoomStructuralRole::DeadEnd:
		return FMath::Max(0.f, multipliers.DeadEnd);
	default:
		return GetDefaultStructuralRoleEnemySpawnMultiplier(role);
	}
}

int32 ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCount(
	const int32 baseCount,
	const FDungeonGeneratedRoomInfo& roomInfo,
	const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
	const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept
{
	if (baseCount <= 0)
	{
		return 0;
	}

	const float scale =
		FindGameplayRoleEnemySpawnMultiplier(gameplayRoleEnemySpawnMultipliers, roomInfo.RoomGameplayRole) *
		FindStructuralRoleEnemySpawnMultiplier(structuralRoleEnemySpawnMultipliers, roomInfo.RoomStructuralRole);
	return FMath::Max(0, FMath::RoundToInt(static_cast<float>(baseCount) * scale));
}

float ADungeonRoomSensorBase::GetGameplayRoleEnemySpawnMultiplier(const EDungeonRoomGameplayRole role) const noexcept
{
	return FindGameplayRoleEnemySpawnMultiplier(GameplayRoleEnemySpawnMultipliers, role);
}

float ADungeonRoomSensorBase::GetStructuralRoleEnemySpawnMultiplier(const EDungeonRoomStructuralRole role) const noexcept
{
	return FindStructuralRoleEnemySpawnMultiplier(StructuralRoleEnemySpawnMultipliers, role);
}

int32 ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCount() const
{
	const int32 baseCount = CalculateAreaBasedActorCount(
		Bounding->Bounds.GetBox(),
		AreaRequiredPerPerson,
		MaxNumberOfActor
	);
	return CalculateSpawnActorsInRoomCount(
		baseCount,
		RoomInfo,
		GameplayRoleEnemySpawnMultipliers,
		StructuralRoleEnemySpawnMultipliers
	);
}

/*
 * Calculates the unscaled actor count from room area only.
 * 部屋面積のみから倍率適用前のアクター数を計算します。
 */
int32 ADungeonRoomSensorBase::CalculateAreaBasedActorCount(const FBox& bounds, const float areaRequiredPerPerson, const int32 maxNumberOfActor) noexcept
{
	const float deltaX = (bounds.Max.X - bounds.Min.X);
	const float deltaY = (bounds.Max.Y - bounds.Min.Y);
	const float squaredArea = deltaX * deltaY;
	const float squaredAreaRequiredPerPerson = areaRequiredPerPerson * areaRequiredPerPerson;
	const int32 numberOfActor = std::max(0, static_cast<int32>(squaredArea / squaredAreaRequiredPerPerson));
	return std::min(numberOfActor, maxNumberOfActor);
}

#if WITH_DEV_AUTOMATION_TESTS
int32 ADungeonRoomSensorBase::CalculateSpawnActorsInRoomCountForTest(
	const int32 baseCount,
	const FDungeonGeneratedRoomInfo& roomInfo,
	const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
	const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept
{
	return CalculateSpawnActorsInRoomCount(
		baseCount,
		roomInfo,
		gameplayRoleEnemySpawnMultipliers,
		structuralRoleEnemySpawnMultipliers);
}

int32 ADungeonRoomSensorBase::CalculateIdealNumberOfActorForTest(
	const FBox& bounds,
	const float areaRequiredPerPerson,
	const int32 maxNumberOfActor,
	const FDungeonGeneratedRoomInfo& roomInfo,
	const FDungeonGameplayRoleEnemySpawnMultipliers& gameplayRoleEnemySpawnMultipliers,
	const FDungeonStructuralRoleEnemySpawnMultipliers& structuralRoleEnemySpawnMultipliers) noexcept
{
	const int32 baseCount = CalculateAreaBasedActorCount(bounds, areaRequiredPerPerson, maxNumberOfActor);
	return CalculateSpawnActorsInRoomCount(
		baseCount,
		roomInfo,
		gameplayRoleEnemySpawnMultipliers,
		structuralRoleEnemySpawnMultipliers);
}
#endif

void ADungeonRoomSensorBase::SpawnActorsInRoomImpl()
{
	if (HasAuthority() == false)
		return;
	if (SpawnActors.IsEmpty())
		return;

	const int32 spawnCount = CalculateSpawnActorsInRoomCount();
	if (spawnCount <= 0)
		return;

	for (int32 i = 0; i < spawnCount; ++i)
	{
		const auto& spawnActorPath = SpawnActors[mSynchronizedRandom.GetInteger(SpawnActors.Num())];
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Actor(%s) spawned."), *GetName(), *spawnActorPath.GetAssetName());
		if (AActor* spawnedActor = SpawnActorInRoomImpl(spawnActorPath, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding))
			DungeonRoomSpawnedActors.Add(spawnedActor);
	}
}

AActor* ADungeonRoomSensorBase::SpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod)
{
	if (HasAuthority() == false)
		return nullptr;
	if (spawnActorPath.IsValid() == false)
		return nullptr;

	check(
		spawnActorCollisionHandlingMethod == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn ||
		spawnActorCollisionHandlingMethod == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding
	);

	AActor* spawnedActor = nullptr;
	do {
		FTransform transform;
		if (RandomTransform(transform, 100.f, false))
		{
			const FSoftObjectPath path(spawnActorPath.ToString() + "_C");
			const TSoftClassPtr<AActor> softClassPointer(path);
			auto* actorClass = softClassPointer.LoadSynchronous();
			spawnedActor = SpawnActorFromClass(actorClass, transform, spawnActorCollisionHandlingMethod, nullptr);
			break;
		}
	} while (spawnActorCollisionHandlingMethod == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	return spawnedActor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void ADungeonRoomSensorBase::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 最初のプレイヤー入場ならResumeを呼ぶ
	if (const APawn* otherPawn = Cast<APawn>(OtherActor))
	{
		if (IsValid(otherPawn->GetController<APlayerController>()))
		{
			if (mOverlapCount == 0)
				InvokeResume();
			++mOverlapCount;
		}
	}
}

void ADungeonRoomSensorBase::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 奈落面から落ちた？
	bool fallToAbyss = false;
	if (IsValid(OverlappedComponent) && IsValid(OtherComp))
	{
		const auto elevation = OtherComp->GetComponentLocation().Z;
		const auto boundary = OverlappedComponent->Bounds.BoxExtent.Z;
		fallToAbyss = elevation < boundary;
	}

	// 最後のプレイヤー退場ならResetを呼ぶ
	if (const APawn* otherPawn = Cast<APawn>(OtherActor))
	{
		if (IsValid(otherPawn->GetController<APlayerController>()))
		{
			--mOverlapCount;
			if (mOverlapCount == 0)
				InvokeReset(fallToAbyss);
		}
	}

	// 奈落落下通知
	if (fallToAbyss)
	{
		OnFallToAbyss.Broadcast();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint functions
int32 ADungeonRoomSensorBase::IdealNumberOfActor(const float areaRequiredPerPerson, const int32 maxNumberOfActor) const
{
	const int32 baseCount = CalculateAreaBasedActorCount(Bounding->Bounds.GetBox(), areaRequiredPerPerson, maxNumberOfActor);
	return CalculateSpawnActorsInRoomCount(
		baseCount,
		RoomInfo,
		GameplayRoleEnemySpawnMultipliers,
		StructuralRoleEnemySpawnMultipliers
	);
}

bool ADungeonRoomSensorBase::RandomPoint(FVector& result, const float offsetHeight, const bool useLocalRandom) const
{
	auto& random = useLocalRandom ?
		const_cast<ADungeonRoomSensorBase*>(this)->mLocalRandom :
		const_cast<ADungeonRoomSensorBase*>(this)->mSynchronizedRandom;
	const FVector& center = Bounding->Bounds.Origin;
	const FVector& extent = Bounding->Bounds.BoxExtent;
	const auto halfHorizontalGridSize = mHorizontalGridSize * 0.5f;
	const auto offsetX = random.GetNumber(-extent.X + halfHorizontalGridSize, extent.X - halfHorizontalGridSize);
	const auto offsetY = random.GetNumber(-extent.Y + halfHorizontalGridSize, extent.Y - halfHorizontalGridSize);
	const FVector startPosition(center.X + offsetX, center.Y + offsetY, center.Z);
	const FVector endPosition(startPosition.X, startPosition.Y, center.Z - extent.Z * 2.);

	// Find the nearest ground location
	return FindFloorHeightPosition(result, startPosition, endPosition, offsetHeight);
}

bool ADungeonRoomSensorBase::RandomTransform(FTransform& result, const float offsetHeight, const bool useLocalRandom) const
{
	FVector position;
	const bool found = RandomPoint(position, offsetHeight, useLocalRandom);
	if (found == false)
	{
		position = Bounding->Bounds.Origin;
		position.Z -= Bounding->Bounds.BoxExtent.Z;
		position.Z += offsetHeight;
	}

	// 部屋の中心から外側へ向く
	const FVector& center = Bounding->Bounds.Origin;
	const double dx = position.X - center.X;
	const double dy = position.Y - center.Y;
	const double yaw = std::atan2(dy, dx) * 57.295779513082320876798154814105;
	const FRotator rotator(0.f, yaw, 0.f);

	result = FTransform(rotator, position, FVector::OneVector);
	return found;
}

bool ADungeonRoomSensorBase::GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight) const
{
	const FBox& bounds = Bounding->Bounds.GetBox();

	FVector endPosition = startPosition;
	endPosition.Z -= (bounds.GetExtent().Z * 2);

	// Find the nearest ground location
	return FindFloorHeightPosition(result, startPosition, endPosition, offsetHeight);
}

bool ADungeonRoomSensorBase::FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const
{
	UWorld* world = GetWorld();
	if (!IsValid(world))
		return false;

	FHitResult hitResult(ForceInit);
	FCollisionQueryParams params("ADungeonRoomSensorBase::FindFloorHeightPosition");
	constexpr ECollisionChannel traceChannel = ECollisionChannel::ECC_Pawn;
	if (!world->LineTraceSingleByChannel(hitResult, startPosition, endPosition, traceChannel, params))
		return false;

	// 埋没している？
	if (hitResult.bStartPenetrating)
		return false;

	// 床ではない？
	if (FVector::DotProduct(FVector::UpVector, hitResult.ImpactNormal) < std::cos(dungeon::math::ToRadian(45.0)))
		return false;

	//result = result.Location;
	result = hitResult.ImpactPoint;
	result.Z += offsetHeight;

	return true;
}

float ADungeonRoomSensorBase::GetDepthRatioFromStart() const
{
	return (DeepestDepthFromStart == 0) ? 0.f : static_cast<float>(DepthFromStart) / static_cast<float>(DeepestDepthFromStart);
}

AActor* ADungeonRoomSensorBase::SpawnActorFromClass(TSubclassOf<class AActor> actorClass, const FTransform transform, const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride, APawn* instigator)
{
	AActor* actor = nullptr;

	UWorld* world = GetWorld();
	if (IsValid(world))
	{
		FActorSpawnParameters actorSpawnParameters;
		actorSpawnParameters.Owner = this;
		actorSpawnParameters.Instigator = instigator;
		actorSpawnParameters.SpawnCollisionHandlingOverride = spawnCollisionHandlingOverride;
		//actorSpawnParameters.TransformScaleMethod = transformScaleMethod;
		actor = world->SpawnActor(actorClass, &transform, actorSpawnParameters);
		if (IsValid(actor))
		{
#if WITH_EDITOR
			actor->SetFolderPath(FName(dungeon::GetBaseDirectoryName() + TEXT("/Actors")));
#endif

			actor->Tags.Reserve(1);
			actor->Tags.Emplace(GetDungeonGeneratorTag());

			// 子アクターとして登録
			Children.Add(actor);
		}
	}

	return actor;
}

uint32_t ADungeonRoomSensorBase::GenerateCrc32(uint32_t crc) const noexcept
{
	if (IsValid(this))
	{
		const FTransform& transform = GetTransform();
		crc = ADungeonVerifiableActor::GenerateCrc32(transform, crc);
		crc = ADungeonVerifiableActor::GenerateCrc32(RoomSize, crc);
		crc = dungeon::GenerateCrc32FromData(&HorizontalMargin, sizeof(float), crc);
		crc = dungeon::GenerateCrc32FromData(&VerticalMargin, sizeof(float), crc);
		crc = dungeon::GenerateCrc32FromData(&Identifier, sizeof(int32), crc);
		crc = dungeon::GenerateCrc32FromData(&Parts, sizeof(EDungeonRoomParts), crc);
		crc = dungeon::GenerateCrc32FromData(&Item, sizeof(EDungeonRoomItem), crc);
		crc = dungeon::GenerateCrc32FromData(&BranchId, sizeof(uint8), crc);
		crc = dungeon::GenerateCrc32FromData(&DepthFromStart, sizeof(uint8), crc);
		crc = dungeon::GenerateCrc32FromData(&DeepestDepthFromStart, sizeof(uint8), crc);
		crc = dungeon::GenerateCrc32FromData(&AutoReset, sizeof(bool), crc);
	}
	return crc;
}

CDungeonRandom& ADungeonRoomSensorBase::GetSynchronizedRandom()
{
	return mSynchronizedRandom;
}

CDungeonRandom& ADungeonRoomSensorBase::GetLocalRandom()
{
	return mLocalRandom;
}
