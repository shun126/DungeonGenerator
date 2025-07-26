/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
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
	if (ShowDebugInformation || ForceShowDebugInformation)
	{
		TArray<FString> output;
		output.Add(TEXT("Identifier:") + FString::FromInt(Identifier));
		output.Add(TEXT("Parts:") + GetDungeonRoomPartsName(Parts));
		output.Add(TEXT("Item:") + GetDungeonRoomItemName(Item));
		output.Add(TEXT("BranchId:") + FString::FromInt(BranchId));
		output.Add(TEXT("DepthFromStart:") + FString::FromInt(DepthFromStart));
		output.Add(FString(TEXT("AutoReset:")) + (AutoReset ? TEXT("On") : TEXT("Off")));
		output.Add(TEXT("DoorAddingProbability:") + FString::FromInt(DoorAddingProbability));
		output.Add(TEXT("Doors:") + FString::FromInt(DungeonDoors.Num()));
		output.Add(TEXT("Torches:") + FString::FromInt(DungeonTorches.Num()));

		FString message;
		for (const FString& line : output)
		{
			message.Append(line);
			message.Append(TEXT("\n"));
		}
		
		FVector location = GetActorLocation();
		static constexpr double offsetHeight = 100;
		location.Z -= Bounding->GetScaledBoxExtent().Z;
		location.Z += offsetHeight;
		DrawDebugString(GetWorld(), location, message, nullptr, FColor::White, 0, true, 1.f);
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

		const float depthFromStartRatio = DeepestDepthFromStart > 0 ?
			static_cast<float>(DepthFromStart) / static_cast<float>(DeepestDepthFromStart) :
			0.0f;
		OnNativeInitialize();
		OnInitialize(Parts, Item, DepthFromStart, depthFromStartRatio);

		if (Item == EDungeonRoomItem::Key)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Key spawned."), *GetName());
			SpawnActorInRoomImpl(SpawnKeyActor, true);
		}
		if (Item == EDungeonRoomItem::UniqueKey)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Unique key spawned."), *GetName());
			SpawnActorInRoomImpl(SpawnUniqueKeyActor, true);
		}
		SpawnActorsInRoomImpl();

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

void ADungeonRoomSensorBase::SpawnActorsInRoomImpl()
{
	if (HasAuthority() == false)
		return;
	if (Parts != EDungeonRoomParts::Hall && Parts != EDungeonRoomParts::Hanare)
		return;
	if (SpawnActors.IsEmpty())
		return;

	const int32 idealNumberOfActor = IdealNumberOfActor(AreaRequiredPerPerson, MaxNumberOfActor);
	for (int32 i = 0; i < idealNumberOfActor; ++i)
	{
		const auto& spawnActorPath = SpawnActors[mSynchronizedRandom.GetInteger(SpawnActors.Num())];
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Actor(%s) spawned."), *GetName(), *spawnActorPath.GetAssetName());
		SpawnActorInRoomImpl(spawnActorPath, false);
	}
}

void ADungeonRoomSensorBase::SpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, const bool force)
{
	if (HasAuthority() == false)
		return;
	if (spawnActorPath.IsValid() == false)
		return;

	do {
		FTransform transform;
		if (RandomTransform(transform, 100.f, false))
		{
			const FSoftObjectPath path(spawnActorPath.ToString() + "_C");
			const TSoftClassPtr<AActor> softClassPointer(path);
			auto* actorClass = softClassPointer.LoadSynchronous();
			SpawnActorFromClass(actorClass, transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding, nullptr, true);
			break;
		}
	} while (force);
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
	const FBox& bounds = Bounding->Bounds.GetBox();
	const float deltaX = (bounds.Max.X - bounds.Min.X);
	const float deltaY = (bounds.Max.Y - bounds.Min.Y);
	const float squaredArea = deltaX * deltaY;
	const float squaredAreaRequiredPerPerson = areaRequiredPerPerson * areaRequiredPerPerson;
	int32 numberOfActor = std::max(0, static_cast<int32>(squaredArea / squaredAreaRequiredPerPerson));
	return std::min(numberOfActor, maxNumberOfActor);
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

AActor* ADungeonRoomSensorBase::SpawnActorFromClass(TSubclassOf<class AActor> actorClass, const FTransform transform, const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride, APawn* instigator, const bool transient)
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
		if (transient)
			actorSpawnParameters.ObjectFlags |= RF_Transient;
		actor = world->SpawnActor(actorClass, &transform, actorSpawnParameters);
		if (IsValid(actor))
		{
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
