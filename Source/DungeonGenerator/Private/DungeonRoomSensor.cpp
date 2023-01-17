/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonRoomSensor.h"
#include "DungeonGenerator.h"
#include <Components/BoxComponent.h>

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

void ADungeonRoomSensor::Initialize(const int32 identifier, const FVector& extents, const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 branchId)
{
	Finalize();
	
	Identifier = identifier;
	//Bounding->InitBoxExtent(extents);
	Bounding->SetBoxExtent(extents);
	Parts = parts;
	Item = item;
	BranchId = branchId;

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
int32 ADungeonRoomSensor::IdealNumberOfActor() const
{
	const FBox& bounds = Bounding->Bounds.GetBox();
	const float deltaX = (bounds.Max.X - bounds.Min.X);
	const float deltaY = (bounds.Max.Y - bounds.Min.Y);
	const float area = deltaX * deltaY;

	static constexpr float AreaRequiredPerPerson = 800.f * 800.f;
	return std::max(0, static_cast<int32>(area / AreaRequiredPerPerson));
}

bool ADungeonRoomSensor::RandomPoint(FVector& result, const float offsetHeight) const
{
	const FVector& center = Bounding->Bounds.Origin;
	const FVector& extent = Bounding->Bounds.BoxExtent;

	const double ratioX = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX) * 0.8;
	const double ratioY = static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX) * 0.8;

	const FVector startPosition(
		center.X + extent.Y * ratioX,
		center.Y + extent.Y * ratioY,
		center.Z
	);
	const FVector endPosition(
		startPosition.X,
		startPosition.Y,
		center.Z - extent.Z * 2
	);

	// ホームを最も近い地上の位置を調べる
	return FindFloorHeightPosition(result, startPosition, endPosition, offsetHeight);
}

bool ADungeonRoomSensor::RandomTransform(FTransform& result, const float offsetHeight) const
{
	FVector position;
	if (!RandomPoint(position, offsetHeight))
		return false;

	const FVector& center = Bounding->Bounds.Origin;
	const double yaw = std::atan2(position.Y - center.Y, position.X - center.X) * 57.295779513082320876798154814105;
	FRotator rotator(0.f, yaw, 0.f);
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
