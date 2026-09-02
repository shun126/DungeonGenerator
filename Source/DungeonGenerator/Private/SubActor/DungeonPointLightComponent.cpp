/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonPointLightComponent.h"

UDungeonPointLightComponent::UDungeonPointLightComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDungeonPointLightComponent::FadeToIntensity(const float TargetIntensity, const float FadeTime)
{
	LightFader.FadeToIntensity(TargetIntensity, FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonPointLightComponent::TurnOn(const float FadeTime)
{
	LightFader.TurnOn(FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonPointLightComponent::TurnOff(const float FadeTime)
{
	LightFader.TurnOff(FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonPointLightComponent::SetDefaultIntensity(const float InIntensity)
{
	LightFader.SetDefaultIntensity(InIntensity);
}

float UDungeonPointLightComponent::GetDefaultIntensity() const
{
	return LightFader.GetDefaultIntensity();
}

void UDungeonPointLightComponent::SetLightVolume(const float Volume)
{
	LightFader.SetVolume(Volume);
}

float UDungeonPointLightComponent::GetLightVolume() const
{
	return LightFader.GetVolume();
}

bool UDungeonPointLightComponent::IsLightFading() const
{
	return LightFader.IsFading();
}

void UDungeonPointLightComponent::OnRegister()
{
	Super::OnRegister();
	LightFader.Initialize(this);
}

void UDungeonPointLightComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	LightFader.Tick(DeltaTime);
	if (!LightFader.IsFading())
		SetComponentTickEnabled(false);
}

void UDungeonPointLightComponent::EnableFadeTickIfNeeded()
{
	if (LightFader.IsFading())
		SetComponentTickEnabled(true);
}
