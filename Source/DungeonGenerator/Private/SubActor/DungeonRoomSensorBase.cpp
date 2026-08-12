/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonRoomSensorBase.h"
#include "SubActor/DungeonDoorBase.h"
#include "DungeonGenerateBase.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "Core/Debug/Debug.h"
#include "Core/Helper/Crc.h"
#include "Core/Math/Random.h"
#include "SubActor/DungeonPointLightComponent.h"
#include "SubActor/DungeonSpotLightComponent.h"
#include "SubActor/DungeonRoomLightingActor.h"
#include <Components/BoxComponent.h>
#include <Components/PointLightComponent.h>
#include <Components/SpotLightComponent.h>
#include <Engine/Scene.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <UObject/UnrealType.h>
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

	BaseFillLightActivator = initializer.CreateDefaultSubobject<UDungeonComponentActivatorComponent>(this, TEXT("BaseFillLightActivator"));
	check(IsValid(BaseFillLightActivator));
	BaseFillLightActivator->SetEnableOwnerActorTickControl(false);
	BaseFillLightActivator->SetEnableOwnerActorAiControl(false);
	BaseFillLightActivator->SetEnableComponentActivationControl(false);
	BaseFillLightActivator->SetEnableComponentVisibilityControl(true);
	BaseFillLightActivator->SetEnableCollisionEnableControl(false);
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

void ADungeonRoomSensorBase::SetBaseFillLightIntensity(const float intensity)
{
	if (!HasAuthority())
	{
		DUNGEON_GENERATOR_WARNING(TEXT("SetBaseFillLightIntensity ignored because %s does not have authority."), *GetName());
		return;
	}
	BaseFillLightIntensity = FMath::Max(0.f, intensity);
	if (IsValid(RoomLightingActor))
		RoomLightingActor->SetBaseFillLightIntensity(BaseFillLightIntensity);
	else
		for (UPointLightComponent* component : BaseFillLightComponents)
			if (IsValid(component))
				component->SetIntensity(BaseFillLightIntensity);
}

void ADungeonRoomSensorBase::SetGuidanceLightIntensity(const float intensity)
{
	if (!HasAuthority())
	{
		DUNGEON_GENERATOR_WARNING(TEXT("SetGuidanceLightIntensity ignored because %s does not have authority."), *GetName());
		return;
	}
	GuidanceLightIntensity = FMath::Max(0.f, intensity);
	if (IsValid(RoomLightingActor))
		RoomLightingActor->SetGuidanceLightIntensity(GuidanceLightIntensity);
	else
		RebuildGuidanceLightComponents();
}

void ADungeonRoomSensorBase::SetRoomLightIntensities(const float baseFillIntensity, const float guidanceIntensity)
{
	if (!HasAuthority())
	{
		DUNGEON_GENERATOR_WARNING(TEXT("SetRoomLightIntensities ignored because %s does not have authority."), *GetName());
		return;
	}
	SetBaseFillLightIntensity(baseFillIntensity);
	SetGuidanceLightIntensity(guidanceIntensity);
}

float ADungeonRoomSensorBase::GetBaseFillLightIntensity() const noexcept
{
	return BaseFillLightIntensity;
}

float ADungeonRoomSensorBase::GetGuidanceLightIntensity() const noexcept
{
	return GuidanceLightIntensity;
}

void ADungeonRoomSensorBase::SetRoomLightingActor(ADungeonRoomLightingActor* lightingActor)
{
	RoomLightingActor = lightingActor;
	if (IsValid(RoomLightingActor))
	{
		DestroyBaseFillLightComponents();
		DestroyGuidanceLightComponents();
	}
}

float ADungeonRoomSensorBase::GetDoorSpawnChance() const noexcept
{
	return DoorSpawnChance;
}

void ADungeonRoomSensorBase::PostLoad()
{
	Super::PostLoad();
	if (DoorSpawnChance == 100.f && DoorAddingProbability != 100)
	{
		DoorSpawnChance = static_cast<float>(DoorAddingProbability);
		DoorAddingProbability = 100;
	}
}

void ADungeonRoomSensorBase::SetDoorSpawnChance(const float doorSpawnChance) noexcept
{
	DoorSpawnChance = FMath::Clamp(doorSpawnChance, 0.f, 100.f);
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
			output.Add(TEXT("DoorSpawnChance:") + FString::SanitizeFloat(DoorSpawnChance));
			output.Add(TEXT("Doors:") + FString::FromInt(DungeonDoors.Num()));
			output.Add(TEXT("Torches:") + FString::FromInt(DungeonTorches.Num()));
			output.Add(TEXT("Chandeliers:") + FString::FromInt(DungeonChandeliers.Num()));
			output.Add(TEXT("BaseFillLights:") + FString::FromInt(BaseFillLightComponents.Num()));
			output.Add(TEXT("GuidanceTargets:") + FString::FromInt(GuidanceTargets.Num()));
			output.Add(TEXT("GuidanceLights:") + FString::FromInt(GuidanceLightComponents.Num()));

			FString message;
			for (const FString& line : output)
			{
				message.Append(line);
				message.Append(TEXT("\n"));
			}

			DrawDebugString(GetWorld(), GetActorLocation(), message, nullptr, FColor::White, 0, true, 1.f);
		}

		if (IsValid(Bounding))
		{
			DrawDebugBox(
				GetWorld(),
				Bounding->GetComponentLocation(),
				Bounding->GetScaledBoxExtent(),
				Bounding->GetComponentQuat(),
				FColor::Cyan,
				false,
				0.f,
				0,
				dungeon::ThinThickness
			);
		}
	}
}

void ADungeonRoomSensorBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName propertyName = PropertyChangedEvent.GetPropertyName();
	const bool baseFillLightPropertyChanged =
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, bEnableBaseFillLights) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightIntensityUnits) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightIntensity) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightColor) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightCellSize) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, MaxBaseFillLightCountX) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, MaxBaseFillLightCountY) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightHeightFromFloorRatio) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightAttenuationScale) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightMinAttenuationRadius) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, BaseFillLightMaxAttenuationRadius);
	const bool guidanceLightPropertyChanged =
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, bEnableGuidanceLights) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, bEnableDoorGuidanceLights) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, bEnableStairGuidanceLights) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightIntensityUnits) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightIntensity) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightColor) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightAttenuationRadius) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightInnerConeAngle) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightOuterConeAngle) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightHorizontalOffset) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, DoorGuidanceLightHeightOffsetScale) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, StairGuidanceLightHeightBelowCeiling) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, GuidanceLightTargetHeightAboveFloor) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, DoorGuidanceLightIntensityMultiplier) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, StairGuidanceLightIntensityMultiplier) ||
		propertyName == GET_MEMBER_NAME_CHECKED(ADungeonRoomSensorBase, MaxGuidanceLightCount);

	const UWorld* world = GetWorld();
	if (baseFillLightPropertyChanged && RoomSize.IsValid && world != nullptr && world->IsGameWorld() == false)
		RebuildBaseFillLightComponents();
	if (guidanceLightPropertyChanged && RoomSize.IsValid && world != nullptr && world->IsGameWorld() == false)
		RebuildGuidanceLightComponents();
}
#endif

/*
 * Destroys all generated base fill light components before rebuilding or removing them.
 * 再構築または削除の前に、生成済みのベース補助光コンポーネントをすべて破棄します。
 */
void ADungeonRoomSensorBase::DestroyBaseFillLightComponents()
{
	for (UPointLightComponent* pointLightComponent : BaseFillLightComponents)
	{
		if (IsValid(pointLightComponent))
			pointLightComponent->DestroyComponent();
	}
	BaseFillLightComponents.Reset();
}

/*
 * Calculates the number of base fill light cells required along each horizontal room axis.
 * 部屋の水平方向の各軸に必要なベース補助光セル数を計算します。
 */
FIntPoint ADungeonRoomSensorBase::CalculateBaseFillLightCounts(const FVector& roomSize, const float cellSize, const int32 maxCountX, const int32 maxCountY) noexcept
{
	if (roomSize.X <= 0.f || roomSize.Y <= 0.f)
		return FIntPoint::ZeroValue;

	const float safeCellSize = FMath::Max(1.f, cellSize);
	return FIntPoint(
		FMath::Clamp(FMath::CeilToInt(roomSize.X / safeCellSize), 1, FMath::Max(1, maxCountX)),
		FMath::Clamp(FMath::CeilToInt(roomSize.Y / safeCellSize), 1, FMath::Max(1, maxCountY)));
}

/*
 * Calculates the base fill light height from a clamped room-height ratio.
 * Clampした部屋高さ比率からベース補助光の床上高さを計算します。
 */
float ADungeonRoomSensorBase::CalculateBaseFillLightHeightAboveFloor(const float roomHeight, const float heightFromFloorRatio) noexcept
{
	return FMath::Max(0.f, roomHeight) * FMath::Clamp(heightFromFloorRatio, 0.f, 1.f);
}

/*
 * Calculates an attenuation radius that reaches the farthest corner of the assigned cell floor.
 * 担当セル床面の最遠角まで届く減衰半径を計算します。
 */
float ADungeonRoomSensorBase::CalculateBaseFillLightAttenuationRadius(
	const float cellWidth,
	const float cellDepth,
	const float heightAboveFloor,
	const float attenuationScale,
	const float minRadius,
	const float maxRadius) noexcept
{
	const float safeMinRadius = FMath::Max(0.f, minRadius);
	const float safeMaxRadius = FMath::Max(safeMinRadius, maxRadius);
	const float halfCellWidth = FMath::Max(0.f, cellWidth) * 0.5f;
	const float halfCellDepth = FMath::Max(0.f, cellDepth) * 0.5f;
	const float safeHeightAboveFloor = FMath::Max(0.f, heightAboveFloor);
	const float farthestFloorCornerDistance = FMath::Sqrt(
		halfCellWidth * halfCellWidth +
		halfCellDepth * halfCellDepth +
		safeHeightAboveFloor * safeHeightAboveFloor);

	return FMath::Clamp(farthestFloorCornerDistance * FMath::Max(0.f, attenuationScale), safeMinRadius, safeMaxRadius);
}

/*
 * Applies the fixed rendering settings used by every generated base fill Point Light.
 * 生成されるすべてのベース補助用 Point Light に共通の固定レンダリング設定を適用します。
 */
void ADungeonRoomSensorBase::ConfigureBaseFillLightComponent(
	UPointLightComponent* pointLightComponent,
	const ELightUnits intensityUnits,
	const float intensity,
	const FLinearColor& color,
	const float attenuationRadius) noexcept
{
	if (!IsValid(pointLightComponent))
		return;

	const bool useInverseSquaredFalloff = intensityUnits == ELightUnits::Candelas || intensityUnits == ELightUnits::Lumens;
	const ELightUnits supportedIntensityUnits = useInverseSquaredFalloff ? intensityUnits : ELightUnits::Unitless;
	pointLightComponent->SetMobility(EComponentMobility::Movable);
	pointLightComponent->SetUseInverseSquaredFalloff(useInverseSquaredFalloff);
	pointLightComponent->SetIntensityUnits(supportedIntensityUnits);
	pointLightComponent->SetLightFalloffExponent(2.f);
	pointLightComponent->SetIntensity(FMath::Max(0.f, intensity));
	pointLightComponent->SetLightColor(color);
	pointLightComponent->SetAttenuationRadius(FMath::Max(0.f, attenuationRadius));
	pointLightComponent->SetCastShadows(false);

	if (UDungeonPointLightComponent* dungeonPointLightComponent = Cast<UDungeonPointLightComponent>(pointLightComponent))
		dungeonPointLightComponent->SetDefaultIntensity(FMath::Max(0.f, intensity));
}

/*
 * Rebuilds shadow-free Point Light components from the prepared room bounds and Blueprint settings.
 * 準備済みの部屋境界とBlueprint設定から、影なしの Point Light コンポーネントを再構築します。
 */
void ADungeonRoomSensorBase::RebuildBaseFillLightComponents()
{
	DestroyBaseFillLightComponents();
	if (!bEnableBaseFillLights || !RoomSize.IsValid || !IsValid(Bounding))
		return;

	const FVector roomSize = RoomSize.GetSize();
	const FIntPoint lightCounts = CalculateBaseFillLightCounts(roomSize, BaseFillLightCellSize, MaxBaseFillLightCountX, MaxBaseFillLightCountY);
	if (lightCounts.X <= 0 || lightCounts.Y <= 0)
		return;

	const float cellWidth = roomSize.X / static_cast<float>(lightCounts.X);
	const float cellDepth = roomSize.Y / static_cast<float>(lightCounts.Y);
	const float heightAboveFloor = CalculateBaseFillLightHeightAboveFloor(roomSize.Z, BaseFillLightHeightFromFloorRatio);
	const float attenuationRadius = CalculateBaseFillLightAttenuationRadius(
		cellWidth,
		cellDepth,
		heightAboveFloor,
		BaseFillLightAttenuationScale,
		BaseFillLightMinAttenuationRadius,
		BaseFillLightMaxAttenuationRadius);
	const FVector roomExtent = RoomSize.GetExtent();

	BaseFillLightComponents.Reserve(lightCounts.X * lightCounts.Y);
	for (int32 y = 0; y < lightCounts.Y; ++y)
	{
		for (int32 x = 0; x < lightCounts.X; ++x)
		{
			const FName componentName = MakeUniqueObjectName(
				this,
				UDungeonPointLightComponent::StaticClass(),
				FName(*FString::Printf(TEXT("BaseFillLight_%d_%d"), x, y)));
			UPointLightComponent* pointLightComponent = NewObject<UDungeonPointLightComponent>(this, componentName);
			if (!IsValid(pointLightComponent))
				continue;

			pointLightComponent->SetupAttachment(Bounding);
			pointLightComponent->SetRelativeLocation(FVector(
				-roomExtent.X + (static_cast<float>(x) + 0.5f) * cellWidth,
				-roomExtent.Y + (static_cast<float>(y) + 0.5f) * cellDepth,
				-roomExtent.Z + heightAboveFloor));
			ConfigureBaseFillLightComponent(pointLightComponent, BaseFillLightIntensityUnits, BaseFillLightIntensity, BaseFillLightColor, attenuationRadius);
			AddInstanceComponent(pointLightComponent);
			pointLightComponent->RegisterComponent();
			BaseFillLightComponents.Add(pointLightComponent);
		}
	}
}

/*
 * Destroys all generated guidance-light components before rebuilding or removing them.
 * 再構築または削除の前に、生成済みの誘導光コンポーネントをすべて破棄します。
 */
void ADungeonRoomSensorBase::DestroyGuidanceLightComponents()
{
	for (USpotLightComponent* spotLightComponent : GuidanceLightComponents)
	{
		if (IsValid(spotLightComponent))
			spotLightComponent->DestroyComponent();
	}
	GuidanceLightComponents.Reset();
}

/*
 * Adds one unique door or stair landmark for later guidance-light generation.
 * 後で誘導光を生成するために、一意なドアまたは階段の目印を追加します。
 */
void ADungeonRoomSensorBase::AddGuidanceTarget(
	const EDungeonGuidanceTargetType type,
	const FVector& location,
	const FVector& direction,
	const float floorHeight)
{
	FVector horizontalDirection(direction.X, direction.Y, 0.f);
	if (!horizontalDirection.Normalize())
		horizontalDirection = FVector::ForwardVector;

	const float duplicateDistance = FMath::Max(1.f, mHorizontalGridSize * 0.5f);
	FDungeonGuidanceTarget candidate;
	candidate.Type = type;
	candidate.Location = FVector(location.X, location.Y, floorHeight);
	candidate.Direction = horizontalDirection;
	candidate.FloorHeight = floorHeight;
	for (const FDungeonGuidanceTarget& target : GuidanceTargets)
	{
		if (AreGuidanceTargetsEquivalent(target, candidate, duplicateDistance))
			return;
	}
	GuidanceTargets.Add(candidate);
}

/*
 * Returns whether two collected landmarks should be merged into one guidance target.
 * 収集された2つの目印を1つの誘導ターゲットへ統合するか返します。
 */
bool ADungeonRoomSensorBase::AreGuidanceTargetsEquivalent(
	const FDungeonGuidanceTarget& left,
	const FDungeonGuidanceTarget& right,
	const float duplicateDistance) noexcept
{
	return left.Type == right.Type &&
		FVector::DistSquared2D(left.Location, right.Location) <= FMath::Square(FMath::Max(0.f, duplicateDistance)) &&
		FMath::Abs(left.FloorHeight - right.FloorHeight) <= 1.f &&
		FVector::DotProduct(left.Direction.GetSafeNormal2D(), right.Direction.GetSafeNormal2D()) >= 0.95f;
}

/*
 * Calculates the number of enabled guidance targets after applying the room limit.
 * 有効な誘導ターゲット数へ部屋単位の上限を適用した誘導光数を計算します。
 */
int32 ADungeonRoomSensorBase::CalculateGuidanceLightCount(
	const TArray<FDungeonGuidanceTarget>& targets,
	const bool enableDoors,
	const bool enableStairs,
	const int32 maxCount) noexcept
{
	int32 enabledCount = 0;
	for (const FDungeonGuidanceTarget& target : targets)
	{
		if ((target.Type == EDungeonGuidanceTargetType::Door && enableDoors) ||
			(target.Type == EDungeonGuidanceTargetType::Stair && enableStairs))
		{
			++enabledCount;
		}
	}
	return FMath::Min(FMath::Max(0, maxCount), enabledCount);
}

/*
 * Calculates a target-specific guidance-light location offset toward the room center.
 * ターゲット種別に応じた誘導光位置を部屋中央側へずらして計算します。
 */
FVector ADungeonRoomSensorBase::CalculateGuidanceLightLocation(
	const FBox& roomSize,
	const FDungeonGuidanceTarget& target,
	const float horizontalOffset,
	const float verticalGridSize,
	const float doorHeightOffsetScale,
	const float stairHeightBelowCeiling) noexcept
{
	FVector inward = roomSize.GetCenter() - target.Location;
	inward.Z = 0.f;
	if (!inward.Normalize())
		inward = -target.Direction.GetSafeNormal2D();

	const float minimumHeight = FMath::Min(target.FloorHeight, roomSize.Max.Z);
	const float lightHeight = target.Type == EDungeonGuidanceTargetType::Door ?
		target.FloorHeight + FMath::Max(0.f, verticalGridSize) * (1.f + FMath::Max(0.f, doorHeightOffsetScale)) :
		FMath::Clamp(roomSize.Max.Z - FMath::Max(0.f, stairHeightBelowCeiling), minimumHeight, roomSize.Max.Z);
	return FVector(target.Location.X, target.Location.Y, lightHeight) + inward * FMath::Max(0.f, horizontalOffset);
}

/*
 * Calculates the rotation that aims a guidance light at a useful height above the target floor.
 * ターゲット床面から見やすい高さへ誘導光を向ける回転を計算します。
 */
FRotator ADungeonRoomSensorBase::CalculateGuidanceLightRotation(
	const FVector& lightLocation,
	const FDungeonGuidanceTarget& target,
	const float targetHeightAboveFloor) noexcept
{
	const FVector aimLocation(target.Location.X, target.Location.Y, target.FloorHeight + FMath::Max(0.f, targetHeightAboveFloor));
	return (aimLocation - lightLocation).Rotation();
}

/*
 * Applies the fixed rendering settings used by generated guidance Spot Lights.
 * 生成される誘導用 Spot Light に共通の固定レンダリング設定を適用します。
 */
void ADungeonRoomSensorBase::ConfigureGuidanceLightComponent(
	USpotLightComponent* spotLightComponent,
	const ELightUnits intensityUnits,
	const float intensity,
	const FLinearColor& color,
	const float attenuationRadius,
	const float innerConeAngle,
	const float outerConeAngle) noexcept
{
	if (!IsValid(spotLightComponent))
		return;

	const float safeOuterConeAngle = FMath::Clamp(outerConeAngle, 1.f, 80.f);
	const float safeInnerConeAngle = FMath::Clamp(innerConeAngle, 0.f, safeOuterConeAngle);
	const bool useInverseSquaredFalloff = intensityUnits == ELightUnits::Candelas || intensityUnits == ELightUnits::Lumens;
	const ELightUnits supportedIntensityUnits = useInverseSquaredFalloff ? intensityUnits : ELightUnits::Unitless;
	spotLightComponent->SetMobility(EComponentMobility::Movable);
	spotLightComponent->SetUseInverseSquaredFalloff(useInverseSquaredFalloff);
	spotLightComponent->SetIntensityUnits(supportedIntensityUnits);
	spotLightComponent->SetIntensity(FMath::Max(0.f, intensity));
	spotLightComponent->SetLightColor(color);
	spotLightComponent->SetLightFalloffExponent(2.f);
	spotLightComponent->SetAttenuationRadius(FMath::Max(0.f, attenuationRadius));
	spotLightComponent->SetInnerConeAngle(safeInnerConeAngle);
	spotLightComponent->SetOuterConeAngle(safeOuterConeAngle);
	spotLightComponent->SetCastShadows(false);

	if (UDungeonSpotLightComponent* dungeonSpotLightComponent = Cast<UDungeonSpotLightComponent>(spotLightComponent))
		dungeonSpotLightComponent->SetDefaultIntensity(FMath::Max(0.f, intensity));
}

/*
 * Rebuilds guidance Spot Lights after all generated door and stair targets have been collected.
 * 生成されたドアと階段のターゲットをすべて収集した後、誘導用 Spot Light を再構築します。
 */
void ADungeonRoomSensorBase::RebuildGuidanceLightComponents()
{
	DestroyGuidanceLightComponents();
	if (!bEnableGuidanceLights || !RoomSize.IsValid || !IsValid(Bounding) || MaxGuidanceLightCount <= 0)
		return;

	TArray<const FDungeonGuidanceTarget*> orderedTargets;
	orderedTargets.Reserve(GuidanceTargets.Num());
	for (const EDungeonGuidanceTargetType type : { EDungeonGuidanceTargetType::Door, EDungeonGuidanceTargetType::Stair })
	{
		for (const FDungeonGuidanceTarget& target : GuidanceTargets)
		{
			const bool enabled = target.Type == EDungeonGuidanceTargetType::Door ? bEnableDoorGuidanceLights : bEnableStairGuidanceLights;
			if (target.Type == type && enabled)
				orderedTargets.Add(&target);
		}
	}

	const int32 lightCount = CalculateGuidanceLightCount(GuidanceTargets, bEnableDoorGuidanceLights, bEnableStairGuidanceLights, MaxGuidanceLightCount);
	GuidanceLightComponents.Reserve(lightCount);
	for (int32 index = 0; index < lightCount; ++index)
	{
		const FDungeonGuidanceTarget& target = *orderedTargets[index];
		const FName componentName = MakeUniqueObjectName(this, UDungeonSpotLightComponent::StaticClass(), FName(*FString::Printf(TEXT("GuidanceLight_%d"), index)));
		USpotLightComponent* spotLightComponent = NewObject<UDungeonSpotLightComponent>(this, componentName);
		if (!IsValid(spotLightComponent))
			continue;

		const FVector lightLocation = CalculateGuidanceLightLocation(
			RoomSize,
			target,
			GuidanceLightHorizontalOffset,
			mVerticalGridSize,
			DoorGuidanceLightHeightOffsetScale,
			StairGuidanceLightHeightBelowCeiling);
		const float intensityMultiplier = target.Type == EDungeonGuidanceTargetType::Door ? DoorGuidanceLightIntensityMultiplier : StairGuidanceLightIntensityMultiplier;
		spotLightComponent->SetupAttachment(Bounding);
		spotLightComponent->SetWorldLocationAndRotation(lightLocation, CalculateGuidanceLightRotation(lightLocation, target, GuidanceLightTargetHeightAboveFloor));
		ConfigureGuidanceLightComponent(
			spotLightComponent,
			GuidanceLightIntensityUnits,
			GuidanceLightIntensity * FMath::Max(0.f, intensityMultiplier),
			GuidanceLightColor,
			GuidanceLightAttenuationRadius,
			GuidanceLightInnerConeAngle,
			GuidanceLightOuterConeAngle);
		AddInstanceComponent(spotLightComponent);
		spotLightComponent->RegisterComponent();
		GuidanceLightComponents.Add(spotLightComponent);
	}
}

bool ADungeonRoomSensorBase::InvokePrepare(
	const std::shared_ptr<dungeon::Random>& random,
	const int32 identifier,
	const FVector& center,
	const FVector& extents,
	const float horizontalGridSize,
	const float verticalGridSize,
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
	RebuildBaseFillLightComponents();

	mHorizontalGridSize = horizontalGridSize;
	mVerticalGridSize = verticalGridSize;
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
		RebuildGuidanceLightComponents();

		{
			if (Item == EDungeonRoomItem::Key)
			{
				DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Key spawn requested."), *GetName());
				RequestSpawnActorInRoomImpl(SpawnKeyActor, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			}
			if (Item == EDungeonRoomItem::UniqueKey)
			{
				DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Unique key spawn requested."), *GetName());
				RequestSpawnActorInRoomImpl(SpawnUniqueKeyActor, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
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

/*
 * Shifts cached generated-room world-space values after the generated dungeon moves.
 * 生成済みダンジョンの移動後に、キャッシュされた生成部屋のワールド空間値を移動します。
 */
void ADungeonRoomSensorBase::ShiftGeneratedRoomInfoWorldOffset(const FVector& delta) noexcept
{
	RoomSize = RoomSize.ShiftBy(delta);
	for (FDungeonGuidanceTarget& target : GuidanceTargets)
	{
		target.Location += delta;
		target.FloorHeight += delta.Z;
	}
}

void ADungeonRoomSensorBase::AddDungeonDoor(ADungeonDoorBase* dungeonDoorBase, const bool addGuidanceTarget)
{
	DungeonDoors.Add(dungeonDoorBase);
	if (addGuidanceTarget && IsValid(dungeonDoorBase))
	{
		const FVector location = dungeonDoorBase->GetActorLocation();
		AddGuidanceTarget(EDungeonGuidanceTargetType::Door, location, dungeonDoorBase->GetActorForwardVector(), location.Z);
	}
}

bool ADungeonRoomSensorBase::HasLockedDoor() const
{
	for (const auto& door : DungeonDoors)
	{
		if (IsValid(door) && door->IsAnyKeyLockedDoor())
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
		DUNGEON_GENERATOR_VERBOSE(TEXT("ADungeonRoomSensorBase(%s) Actor(%s) spawn requested."), *GetName(), *spawnActorPath.GetAssetName());
		RequestSpawnActorInRoomImpl(spawnActorPath, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	}
}

/**
 * Queues one configured room Actor while consuming synchronized placement randomness immediately.
 * 同期配置乱数を即時に消費し、設定された部屋Actorを1件生成予約します。
 */
void ADungeonRoomSensorBase::RequestSpawnActorInRoomImpl(const FSoftObjectPath& spawnActorPath, const ESpawnActorCollisionHandlingMethod spawnActorCollisionHandlingMethod)
{
	if (HasAuthority() == false)
		return;
	if (spawnActorPath.IsValid() == false)
		return;

	check(
		spawnActorCollisionHandlingMethod == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn ||
		spawnActorCollisionHandlingMethod == ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding
	);

	FTransform transform;
	if (!RandomTransform(transform, 100.f, false))
		return;

	const FSoftObjectPath path(spawnActorPath.ToString() + "_C");
	const TSoftClassPtr<AActor> softClassPointer(path);
	UClass* actorClass = softClassPointer.LoadSynchronous();
	TWeakObjectPtr<ADungeonRoomSensorBase> weakThis(this);
	RequestDeferredSpawnActorFromClassImpl(
		actorClass,
		transform,
		spawnActorCollisionHandlingMethod,
		nullptr,
		[weakThis](AActor* spawnedActor)
		{
			if (ADungeonRoomSensorBase* roomSensor = weakThis.Get(); IsValid(roomSensor) && IsValid(spawnedActor))
			{
				if (APawn* pawn = GetValid(Cast<APawn>(spawnedActor)))
				{
					if (!IsValid(pawn->GetController()))
						pawn->SpawnDefaultController();
				}

				roomSensor->DungeonRoomSpawnedActors.Add(spawnedActor);
			}
		});
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
		RegisterSpawnedChildActor(actor);
	}

	return actor;
}

void ADungeonRoomSensorBase::RequestDeferredSpawnActorFromClass(
	TSubclassOf<class AActor> actorClass,
	const FTransform transform,
	const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride,
	APawn* instigatorActor,
	const FDungeonRoomSensorDeferredActorSpawnedSignature& onSpawned)
{
	RequestDeferredSpawnActorFromClassImpl(
		actorClass,
		transform,
		spawnCollisionHandlingOverride,
		instigatorActor,
		[onSpawned](AActor* spawnedActor) mutable
		{
			onSpawned.ExecuteIfBound(spawnedActor);
		});
}

/**
 * Queues one child Actor through the owning generator and safely ignores callbacks after this sensor is destroyed.
 * 所有Generatorへ子Actorを1件生成予約し、このSensor破棄後のコールバックを安全に無視します。
 */
void ADungeonRoomSensorBase::RequestDeferredSpawnActorFromClassImpl(
	TSubclassOf<AActor> actorClass,
	const FTransform& transform,
	const ESpawnActorCollisionHandlingMethod spawnCollisionHandlingOverride,
	APawn* instigatorActor,
	TFunction<void(AActor*)> onSpawned)
{
	if (!IsValid(actorClass) || !IsValid(DungeonGeneratorOwner))
	{
		DUNGEON_GENERATOR_WARNING(TEXT("ADungeonRoomSensorBase(%s) could not queue a deferred Actor because its class or Dungeon Generator owner is invalid."), *GetName());
		if (onSpawned)
			onSpawned(nullptr);
		return;
	}

	FActorSpawnParameters actorSpawnParameters;
	actorSpawnParameters.Owner = this;
	actorSpawnParameters.Instigator = instigatorActor;
	actorSpawnParameters.SpawnCollisionHandlingOverride = spawnCollisionHandlingOverride;

	TWeakObjectPtr<ADungeonRoomSensorBase> weakThis(this);
	DungeonGeneratorOwner->DeferredSpawnActorWithFolderPath(
		actorClass,
		TEXT("Actors"),
		transform,
		actorSpawnParameters,
		[weakThis, onSpawned = MoveTemp(onSpawned)](AActor* spawnedActor) mutable
		{
			ADungeonRoomSensorBase* roomSensor = weakThis.Get();
			if (!IsValid(roomSensor))
				return;

			roomSensor->RegisterSpawnedChildActor(spawnedActor);
			if (onSpawned)
				onSpawned(spawnedActor);
		},
		this);
}

/**
 * Registers one spawned Actor as a tagged child of this Room Sensor.
 * 生成済みActorをこのRoom Sensorのタグ付き子Actorとして登録します。
 */
void ADungeonRoomSensorBase::RegisterSpawnedChildActor(AActor* actor)
{
	if (!IsValid(actor))
		return;

#if WITH_EDITOR
	actor->SetFolderPath(FName(dungeon::GetBaseDirectoryName() + TEXT("/Actors")));
#endif

	actor->Tags.AddUnique(GetDungeonGeneratorTag());
	Children.AddUnique(actor);
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
