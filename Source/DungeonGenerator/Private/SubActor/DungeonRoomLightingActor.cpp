/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonRoomLightingActor.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "SubActor/DungeonPointLightComponent.h"
#include "SubActor/DungeonSpotLightComponent.h"
#include <Components/PointLightComponent.h>
#include <Components/SceneComponent.h>
#include <Components/SpotLightComponent.h>
#include <Net/UnrealNetwork.h>

ADungeonRoomLightingActor::ADungeonRoomLightingActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
	SetCanBeDamaged(false);
	bReplicates = true;
	SetReplicateMovement(false);
	bNetLoadOnClient = false;
	NetDormancy = DORM_Initial;
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRoot"));
	check(IsValid(SceneRoot));
	RootComponent = SceneRoot;

	ComponentActivator = initializer.CreateDefaultSubobject<UDungeonComponentActivatorComponent>(this, TEXT("ComponentActivator"));
	check(IsValid(ComponentActivator));
	ComponentActivator->SetEnableOwnerActorTickControl(false);
	ComponentActivator->SetEnableOwnerActorAiControl(false);
	ComponentActivator->SetEnableComponentActivationControl(false);
	ComponentActivator->SetEnableComponentVisibilityControl(true);
	ComponentActivator->SetEnableCollisionEnableControl(false);
}

void ADungeonRoomLightingActor::InitializeFromRoomSensor(ADungeonRoomSensorBase* roomSensor)
{
	if (!HasAuthority() || !IsValid(roomSensor))
		return;

	LightingState.Identifier = roomSensor->Identifier;
	++LightingState.LayoutRevision;
	LightingState.RoomBounds = roomSensor->RoomSize;
	LightingState.VerticalGridSize = roomSensor->mVerticalGridSize;
	LightingState.bEnableBaseFillLights = roomSensor->bEnableBaseFillLights;
	LightingState.BaseFillLightIntensityUnits = roomSensor->BaseFillLightIntensityUnits;
	LightingState.BaseFillLightIntensity = FMath::Max(0.f, roomSensor->BaseFillLightIntensity);
	LightingState.BaseFillLightColor = roomSensor->BaseFillLightColor;
	LightingState.BaseFillLightCellSize = roomSensor->BaseFillLightCellSize;
	LightingState.MaxBaseFillLightCountX = roomSensor->MaxBaseFillLightCountX;
	LightingState.MaxBaseFillLightCountY = roomSensor->MaxBaseFillLightCountY;
	LightingState.BaseFillLightHeightFromFloorRatio = roomSensor->BaseFillLightHeightFromFloorRatio;
	LightingState.BaseFillLightAttenuationScale = roomSensor->BaseFillLightAttenuationScale;
	LightingState.BaseFillLightMinAttenuationRadius = roomSensor->BaseFillLightMinAttenuationRadius;
	LightingState.BaseFillLightMaxAttenuationRadius = roomSensor->BaseFillLightMaxAttenuationRadius;
	LightingState.bEnableGuidanceLights = roomSensor->bEnableGuidanceLights;
	LightingState.bEnableDoorGuidanceLights = roomSensor->bEnableDoorGuidanceLights;
	LightingState.bEnableStairGuidanceLights = roomSensor->bEnableStairGuidanceLights;
	LightingState.GuidanceLightIntensityUnits = roomSensor->GuidanceLightIntensityUnits;
	LightingState.GuidanceLightIntensity = FMath::Max(0.f, roomSensor->GuidanceLightIntensity);
	LightingState.GuidanceLightColor = roomSensor->GuidanceLightColor;
	LightingState.GuidanceLightAttenuationRadius = roomSensor->GuidanceLightAttenuationRadius;
	LightingState.GuidanceLightInnerConeAngle = roomSensor->GuidanceLightInnerConeAngle;
	LightingState.GuidanceLightOuterConeAngle = roomSensor->GuidanceLightOuterConeAngle;
	LightingState.GuidanceLightHorizontalOffset = roomSensor->GuidanceLightHorizontalOffset;
	LightingState.DoorGuidanceLightHeightOffsetScale = roomSensor->DoorGuidanceLightHeightOffsetScale;
	LightingState.StairGuidanceLightHeightBelowCeiling = roomSensor->StairGuidanceLightHeightBelowCeiling;
	LightingState.GuidanceLightTargetHeightAboveFloor = roomSensor->GuidanceLightTargetHeightAboveFloor;
	LightingState.DoorGuidanceLightIntensityMultiplier = roomSensor->DoorGuidanceLightIntensityMultiplier;
	LightingState.StairGuidanceLightIntensityMultiplier = roomSensor->StairGuidanceLightIntensityMultiplier;
	LightingState.MaxGuidanceLightCount = roomSensor->MaxGuidanceLightCount;
	LightingState.GuidanceTargets = roomSensor->GuidanceTargets;

	ComponentActivator->SetFixedPartitionRegistrationWorldLocation(LightingState.RoomBounds.GetCenter());
	RebuildLightComponents();
	WakeForReplication();
}

void ADungeonRoomLightingActor::SetBaseFillLightIntensity(const float intensity)
{
	if (!HasAuthority())
		return;
	LightingState.BaseFillLightIntensity = FMath::Max(0.f, intensity);
	ApplyIntensities();
	WakeForReplication();
}

void ADungeonRoomLightingActor::SetGuidanceLightIntensity(const float intensity)
{
	if (!HasAuthority())
		return;
	LightingState.GuidanceLightIntensity = FMath::Max(0.f, intensity);
	ApplyIntensities();
	WakeForReplication();
}

float ADungeonRoomLightingActor::GetBaseFillLightIntensity() const noexcept
{
	return LightingState.BaseFillLightIntensity;
}

float ADungeonRoomLightingActor::GetGuidanceLightIntensity() const noexcept
{
	return LightingState.GuidanceLightIntensity;
}

void ADungeonRoomLightingActor::ShiftWorldOffset(const FVector& delta)
{
	if (!HasAuthority() || delta.IsNearlyZero())
		return;
	LightingState.RoomBounds = LightingState.RoomBounds.ShiftBy(delta);
	for (FDungeonGuidanceTarget& target : LightingState.GuidanceTargets)
		target.Location += delta;
	++LightingState.LayoutRevision;
	WakeForReplication();
}

void ADungeonRoomLightingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADungeonRoomLightingActor, LightingState);
}

void ADungeonRoomLightingActor::OnRep_LightingState()
{
	if (AppliedLayoutRevision != LightingState.LayoutRevision)
		RebuildLightComponents();
	else
		ApplyIntensities();
}

void ADungeonRoomLightingActor::DestroyLightComponents()
{
	for (UPointLightComponent* component : BaseFillLightComponents)
		if (IsValid(component))
			component->DestroyComponent();
	BaseFillLightComponents.Reset();

	for (USpotLightComponent* component : GuidanceLightComponents)
		if (IsValid(component))
			component->DestroyComponent();
	GuidanceLightComponents.Reset();
	GuidanceLightTargetTypes.Reset();
}

void ADungeonRoomLightingActor::RebuildLightComponents()
{
	DestroyLightComponents();
	if (!LightingState.RoomBounds.IsValid)
		return;
	AppliedLayoutRevision = LightingState.LayoutRevision;

	ComponentActivator->SetFixedPartitionRegistrationWorldLocation(LightingState.RoomBounds.GetCenter());
	const FVector roomSize = LightingState.RoomBounds.GetSize();
	if (LightingState.bEnableBaseFillLights)
	{
		const FIntPoint counts = ADungeonRoomSensorBase::CalculateBaseFillLightCounts(
			roomSize, LightingState.BaseFillLightCellSize, LightingState.MaxBaseFillLightCountX, LightingState.MaxBaseFillLightCountY);
		const float cellWidth = roomSize.X / static_cast<float>(counts.X);
		const float cellDepth = roomSize.Y / static_cast<float>(counts.Y);
		const float height = ADungeonRoomSensorBase::CalculateBaseFillLightHeightAboveFloor(roomSize.Z, LightingState.BaseFillLightHeightFromFloorRatio);
		const float radius = ADungeonRoomSensorBase::CalculateBaseFillLightAttenuationRadius(
			cellWidth, cellDepth, height, LightingState.BaseFillLightAttenuationScale,
			LightingState.BaseFillLightMinAttenuationRadius, LightingState.BaseFillLightMaxAttenuationRadius);
		for (int32 y = 0; y < counts.Y; ++y)
		{
			for (int32 x = 0; x < counts.X; ++x)
			{
				UPointLightComponent* component = NewObject<UDungeonPointLightComponent>(this, MakeUniqueObjectName(this, UDungeonPointLightComponent::StaticClass(), TEXT("BaseFillLight")));
				component->SetupAttachment(SceneRoot);
				component->SetWorldLocation(FVector(
					LightingState.RoomBounds.Min.X + cellWidth * (static_cast<float>(x) + 0.5f),
					LightingState.RoomBounds.Min.Y + cellDepth * (static_cast<float>(y) + 0.5f),
					LightingState.RoomBounds.Min.Z + height));
				ADungeonRoomSensorBase::ConfigureBaseFillLightComponent(component, LightingState.BaseFillLightIntensityUnits, LightingState.BaseFillLightIntensity, LightingState.BaseFillLightColor, radius);
				component->RegisterComponent();
				BaseFillLightComponents.Add(component);
			}
		}
	}

	if (LightingState.bEnableGuidanceLights && LightingState.MaxGuidanceLightCount > 0)
	{
		TArray<const FDungeonGuidanceTarget*> targets;
		for (const EDungeonGuidanceTargetType type : { EDungeonGuidanceTargetType::Door, EDungeonGuidanceTargetType::Stair })
			for (const FDungeonGuidanceTarget& target : LightingState.GuidanceTargets)
				if (target.Type == type && (type == EDungeonGuidanceTargetType::Door ? LightingState.bEnableDoorGuidanceLights : LightingState.bEnableStairGuidanceLights))
					targets.Add(&target);
		const int32 count = FMath::Min(targets.Num(), FMath::Max(0, LightingState.MaxGuidanceLightCount));
		for (int32 index = 0; index < count; ++index)
		{
			const FDungeonGuidanceTarget& target = *targets[index];
			USpotLightComponent* component = NewObject<UDungeonSpotLightComponent>(this, MakeUniqueObjectName(this, UDungeonSpotLightComponent::StaticClass(), TEXT("GuidanceLight")));
			component->SetupAttachment(SceneRoot);
			const FVector location = ADungeonRoomSensorBase::CalculateGuidanceLightLocation(
				LightingState.RoomBounds, target, LightingState.GuidanceLightHorizontalOffset, LightingState.VerticalGridSize,
				LightingState.DoorGuidanceLightHeightOffsetScale, LightingState.StairGuidanceLightHeightBelowCeiling);
			component->SetWorldLocationAndRotation(location, ADungeonRoomSensorBase::CalculateGuidanceLightRotation(location, target, LightingState.GuidanceLightTargetHeightAboveFloor));
			const float multiplier = target.Type == EDungeonGuidanceTargetType::Door ? LightingState.DoorGuidanceLightIntensityMultiplier : LightingState.StairGuidanceLightIntensityMultiplier;
			ADungeonRoomSensorBase::ConfigureGuidanceLightComponent(component, LightingState.GuidanceLightIntensityUnits,
				LightingState.GuidanceLightIntensity * FMath::Max(0.f, multiplier), LightingState.GuidanceLightColor,
				LightingState.GuidanceLightAttenuationRadius, LightingState.GuidanceLightInnerConeAngle, LightingState.GuidanceLightOuterConeAngle);
			component->RegisterComponent();
			GuidanceLightComponents.Add(component);
			GuidanceLightTargetTypes.Add(target.Type);
		}
	}
}

void ADungeonRoomLightingActor::ApplyIntensities()
{
	for (UPointLightComponent* component : BaseFillLightComponents)
		if (IsValid(component))
		{
			if (UDungeonPointLightComponent* dungeonComponent = Cast<UDungeonPointLightComponent>(component))
				dungeonComponent->SetDefaultIntensity(LightingState.BaseFillLightIntensity);
			else
				component->SetIntensity(LightingState.BaseFillLightIntensity);
		}

	for (int32 index = 0; index < GuidanceLightComponents.Num(); ++index)
	{
		if (!IsValid(GuidanceLightComponents[index]))
			continue;
		float multiplier = 1.f;
		if (GuidanceLightTargetTypes.IsValidIndex(index))
			multiplier = GuidanceLightTargetTypes[index] == EDungeonGuidanceTargetType::Door ? LightingState.DoorGuidanceLightIntensityMultiplier : LightingState.StairGuidanceLightIntensityMultiplier;
		const float intensity = LightingState.GuidanceLightIntensity * FMath::Max(0.f, multiplier);
		if (UDungeonSpotLightComponent* dungeonComponent = Cast<UDungeonSpotLightComponent>(GuidanceLightComponents[index]))
			dungeonComponent->SetDefaultIntensity(intensity);
		else
			GuidanceLightComponents[index]->SetIntensity(intensity);
	}
}

void ADungeonRoomLightingActor::WakeForReplication()
{
	FlushNetDormancy();
	ForceNetUpdate();
	SetNetDormancy(DORM_DormantAll);
}
