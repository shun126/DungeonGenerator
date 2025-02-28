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
		output.Add(TEXT("Torches:") + FString::FromInt(DungeonTorchs.Num()));

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
	DungeonTorchs.Add(actor);
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

namespace
{
	/*
	正規乱数
	https://ja.wikipedia.org/wiki/%E3%83%9C%E3%83%83%E3%82%AF%E3%82%B9%EF%BC%9D%E3%83%9F%E3%83%A5%E3%83%A9%E3%83%BC%E6%B3%95
	*/
	inline void NormalRandom(float& x, float& y, CDungeonRandom& random)
	{
		const auto A = random.GetNumber();
		const auto B = random.GetNumber();
		const auto X = std::sqrt(-2.f * std::log(A));
		const auto Y = 2.f * 3.14159265359f * B;
		x = X * std::cos(Y);
		y = X * std::sin(Y);
	}
}

bool ADungeonRoomSensorBase::RandomPoint(FVector& result, const float offsetHeight, const bool useLocalRandom) const
{
	float ratioX, ratioY;
	if (useLocalRandom)
		NormalRandom(ratioX, ratioY, const_cast<ADungeonRoomSensorBase*>(this)->mLocalRandom);
	else
		NormalRandom(ratioX, ratioY, const_cast<ADungeonRoomSensorBase*>(this)->mSynchronizedRandom);

	static constexpr float range = 0.9f;
	ratioX = std::max(-range, std::min(ratioX / (3.9f / range), range));
	ratioY = std::max(-range, std::min(ratioY / (3.9f / range), range));

	const FVector& center = Bounding->Bounds.Origin;
	const FVector& extent = Bounding->Bounds.BoxExtent;
	const FVector startPosition(
		center.X + extent.X * ratioX,
		center.Y + extent.Y * ratioY,
		center.Z
	);
	const FVector endPosition(
		startPosition.X,
		startPosition.Y,
		center.Z - extent.Z * 2.
	);

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

	// look outward from the center
	const FVector& center = Bounding->Bounds.Origin;
	const double dx = center.X - position.X;
	const double dy = center.Y - position.Y;
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
	FHitResult hitResult(ForceInit);
	FCollisionQueryParams params("ADungeonRoomSensorBase::FindFloorHeightPosition", true);
	const ECollisionChannel traceChannel = ECollisionChannel::ECC_Visibility;
	if (!GetWorld()->LineTraceSingleByChannel(hitResult, startPosition, endPosition, traceChannel, params))
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
