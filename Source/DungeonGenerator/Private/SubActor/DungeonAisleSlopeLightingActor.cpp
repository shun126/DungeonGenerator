/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonAisleSlopeLightingActor.h"
#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "SubActor/DungeonPointLightComponent.h"
#include <Components/PointLightComponent.h>
#include <Components/SceneComponent.h>
#include <Net/UnrealNetwork.h>

ADungeonAisleSlopeLightingActor::ADungeonAisleSlopeLightingActor(const FObjectInitializer& initializer)
	: Super(initializer)
{
	SetCanBeDamaged(false);
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = initializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneRoot"));
	check(IsValid(SceneRoot));
	RootComponent = SceneRoot;

	PointLightComponent = initializer.CreateDefaultSubobject<UDungeonPointLightComponent>(this, TEXT("PointLight"));
	check(IsValid(PointLightComponent));
	PointLightComponent->SetupAttachment(SceneRoot);

	ComponentActivator = initializer.CreateDefaultSubobject<UDungeonComponentActivatorComponent>(this, TEXT("ComponentActivator"));
	check(IsValid(ComponentActivator));
	ComponentActivator->SetEnableOwnerActorTickControl(false);
	ComponentActivator->SetEnableOwnerActorAiControl(false);
	ComponentActivator->SetEnableComponentActivationControl(false);
	ComponentActivator->SetEnableComponentVisibilityControl(true);
	ComponentActivator->SetEnableCollisionEnableControl(false);
}

void ADungeonAisleSlopeLightingActor::Initialize(const FDungeonAisleSlopeBaseLightSettings& settings, const FVector& partitionRegistrationWorldLocation)
{
	Settings = settings;
	ConfigurePointLight(PointLightComponent, Settings);
	ComponentActivator->SetFixedPartitionRegistrationWorldLocation(partitionRegistrationWorldLocation);
	ForceNetUpdate();
}

void ADungeonAisleSlopeLightingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADungeonAisleSlopeLightingActor, Settings);
}

void ADungeonAisleSlopeLightingActor::OnRep_Settings()
{
	ConfigurePointLight(PointLightComponent, Settings);
	ComponentActivator->SetFixedPartitionRegistrationWorldLocation(GetActorLocation());
}

UPointLightComponent* ADungeonAisleSlopeLightingActor::GetPointLightComponent() const noexcept
{
	return PointLightComponent;
}

UDungeonComponentActivatorComponent* ADungeonAisleSlopeLightingActor::GetComponentActivator() const noexcept
{
	return ComponentActivator;
}

FVector ADungeonAisleSlopeLightingActor::CalculateLightLocation(
	const FVector& slopeOrigin,
	const FVector& slopeDirection,
	const float horizontalGridSize,
	const float verticalGridSize,
	const float heightOffset) noexcept
{
	const FVector horizontalDirection = slopeDirection.GetSafeNormal2D();
	return slopeOrigin
		+ horizontalDirection * (FMath::Max(0.f, horizontalGridSize) * 0.5f)
		+ FVector::UpVector * (FMath::Max(0.f, verticalGridSize) * 0.5f + FMath::Max(0.f, heightOffset));
}

/*
 * Applies the fixed rendering behavior and user-adjustable visual settings.
 * 固定の描画動作とユーザー調整可能な表示設定を適用します。
 */
void ADungeonAisleSlopeLightingActor::ConfigurePointLight(UPointLightComponent* pointLightComponent, const FDungeonAisleSlopeBaseLightSettings& settings) noexcept
{
	if (!IsValid(pointLightComponent))
		return;

	const bool useInverseSquaredFalloff = settings.IntensityUnits == ELightUnits::Candelas || settings.IntensityUnits == ELightUnits::Lumens;
	pointLightComponent->SetMobility(EComponentMobility::Movable);
	pointLightComponent->SetUseInverseSquaredFalloff(useInverseSquaredFalloff);
	pointLightComponent->SetIntensityUnits(useInverseSquaredFalloff ? settings.IntensityUnits : ELightUnits::Unitless);
	pointLightComponent->SetLightFalloffExponent(2.f);
	pointLightComponent->SetIntensity(FMath::Max(0.f, settings.Intensity));
	pointLightComponent->SetLightColor(settings.Color);
	pointLightComponent->SetAttenuationRadius(FMath::Max(0.f, settings.AttenuationRadius));
	pointLightComponent->SetCastShadows(false);

	if (UDungeonPointLightComponent* dungeonPointLightComponent = Cast<UDungeonPointLightComponent>(pointLightComponent))
		dungeonPointLightComponent->SetDefaultIntensity(FMath::Max(0.f, settings.Intensity));
}

