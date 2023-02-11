/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonRoomSensor.h"
#include "DungeonGenerator.h"
#include "Core/Identifier.h"
#include <Components/BoxComponent.h>
#include <random>

#if WITH_EDITOR
#include <Kismet/KismetSystemLibrary.h>

namespace
{
	static bool ForceShowDebugInfomation = true;
}
#endif

const FName& ADungeonRoomSensor::GetDungeonGeneratorTag()
{
	return CDungeonGenerator::GetDungeonGeneratorTag();
}

const TArray<FName>& ADungeonRoomSensor::GetDungeonGeneratorTags()
{
	static const TArray<FName> tags = {
		GetDungeonGeneratorTag()
	};
	return tags;
}

ADungeonRoomSensor::ADungeonRoomSensor(const FObjectInitializer& initializer)
	: Super(initializer)
{
#if WITH_EDITOR
	// ティック有効化
	PrimaryActorTick.bCanEverTick = PrimaryActorTick.bStartWithTickEnabled = true;
#endif

	Bounding = initializer.CreateDefaultSubobject<UBoxComponent>(this, TEXT("Sensor"));
	check(IsValid(Bounding));
	Bounding->SetMobility(EComponentMobility::Static);
	Bounding->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounding->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Bounding->SetGenerateOverlapEvents(true);
	SetRootComponent(Bounding);
}

void ADungeonRoomSensor::BeginDestroy()
{
	Super::BeginDestroy();
	Finalize();
}

#if WITH_EDITOR
void ADungeonRoomSensor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ShowDebugInfomation || ForceShowDebugInfomation)
	{
		TArray<FString> output;
		output.Add(TEXT("Identifier:") + FString::FromInt(Identifier));
		output.Add(TEXT("Parts:") + GetDungeonRoomPartsName(Parts));
		output.Add(TEXT("Item:") + GetDungeonRoomItemName(Item));
		output.Add(TEXT("BranchId:") + FString::FromInt(BranchId));
		output.Add(TEXT("DepthFromStart:") + FString::FromInt(DepthFromStart));

		// 文字列を展開
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
		UKismetSystemLibrary::DrawDebugString(GetWorld(), location, message, nullptr, FLinearColor::White);
	}
}
#endif

void ADungeonRoomSensor::Initialize(
	const int32 identifier,
	const FVector& extents,
	const EDungeonRoomParts parts,
	const EDungeonRoomItem item,
	const uint8 branchId,
	const uint8 depthFromStart,
	const uint8 deepestDepthFromStart)
{
	Finalize();
	
	Identifier = identifier;
	//Bounding->InitBoxExtent(extents);
	Bounding->SetBoxExtent(extents);
	Parts = parts;
	Item = item;
	BranchId = branchId;
	DepthFromStart = depthFromStart;
	DeepestDepthFromStart = deepestDepthFromStart;

	OnInitialize();

	mState = State::Initialized;
}

void ADungeonRoomSensor::OnInitialize_Implementation()
{
}

void ADungeonRoomSensor::Finalize()
{
	if (mState == State::Initialized)
	{
		OnFinalize();
		mState = State::Finalized;
	}
}

void ADungeonRoomSensor::OnFinalize_Implementation()
{
}

void ADungeonRoomSensor::Reset()
{
	OnReset();
}

void ADungeonRoomSensor::OnReset_Implementation()
{
}

UBoxComponent* ADungeonRoomSensor::GetBounding()
{
	return Bounding;
}

const UBoxComponent* ADungeonRoomSensor::GetBounding() const
{
	return Bounding;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// BluePrint便利関数
int32 ADungeonRoomSensor::IdealNumberOfActor(const float areaRequiredPerPerson, const int32 maxNumberOfActor) const
{
	const FBox& bounds = Bounding->Bounds.GetBox();
	const float deltaX = (bounds.Max.X - bounds.Min.X);
	const float deltaY = (bounds.Max.Y - bounds.Min.Y);
	const float squaredArea = deltaX * deltaY;
	const float squaredAreaRequiredPerPerson = areaRequiredPerPerson * areaRequiredPerPerson;
	int32 numberOfActor = std::max(0, static_cast<int32>(squaredArea / squaredAreaRequiredPerPerson));
	return std::min(numberOfActor, maxNumberOfActor);
}

bool ADungeonRoomSensor::RandomPoint(FVector& result, const float offsetHeight) const
{
	static std::random_device seed;
	static std::mt19937 engine(seed());				// メルセンヌ・ツイスター法
	//static std::minstd_rand0 engine(seed());		// 線形合同法
	//static std::ranlux24_base engine(seed());		// キャリー付き減算法
	std::normal_distribution<> distribution(0., 1.);
	double ratioX = distribution(engine);
	double ratioY = distribution(engine);

	static constexpr double range = 0.8;
	ratioX = std::max(-range, std::min(ratioX, range));
	ratioY = std::max(-range, std::min(ratioY, range));

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

	// ホームを最も近い地上の位置を調べる
	return FindFloorHeightPosition(result, startPosition, endPosition, offsetHeight);
}

bool ADungeonRoomSensor::RandomTransform(FTransform& result, const float offsetHeight) const
{
	FVector position;
	if (!RandomPoint(position, offsetHeight))
		return false;

	// look outward from the center
	const FVector& center = Bounding->Bounds.Origin;
	const double dx = center.X - position.X;
	const double dy = center.Y - position.Y;
	const double yaw = std::atan2(dy, dx) * 57.295779513082320876798154814105;
	const FRotator rotator(0.f, yaw, 0.f);

	result = FTransform(rotator, position, FVector::OneVector);
	return true;
}

bool ADungeonRoomSensor::GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight) const
{
	const FBox& bounds = Bounding->Bounds.GetBox();

	FVector endPosition = startPosition;
	endPosition.Z -= (bounds.GetExtent().Z * 2);

	return FindFloorHeightPosition(result, startPosition, endPosition, offsetHeight);
}

bool ADungeonRoomSensor::FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const
{
	FHitResult hitResult(ForceInit);
	FCollisionQueryParams params("ADungeonRoomSensor::FindFloorHeightPosition", true);
	const ECollisionChannel traceChannel = ECollisionChannel::ECC_Visibility;
	if (!GetWorld()->LineTraceSingleByChannel(hitResult, startPosition, endPosition, traceChannel, params))
		return false;

	//result = result.Location;
	result = hitResult.ImpactPoint;
	result.Z += offsetHeight;
	return true;
}

float ADungeonRoomSensor::GetDepthRatioFromStart() const
{
	if (DeepestDepthFromStart <= 0.f)
		return 0.f;
	return static_cast<float>(DepthFromStart) / static_cast<float>(DeepestDepthFromStart);
}
