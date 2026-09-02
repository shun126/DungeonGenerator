/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonSpotLightComponent.h"

UDungeonSpotLightComponent::UDungeonSpotLightComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDungeonSpotLightComponent::FadeToIntensity(const float TargetIntensity, const float FadeTime)
{
	LightFader.FadeToIntensity(TargetIntensity, FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonSpotLightComponent::TurnOn(const float FadeTime)
{
	LightFader.TurnOn(FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonSpotLightComponent::TurnOff(const float FadeTime)
{
	LightFader.TurnOff(FadeTime);
	EnableFadeTickIfNeeded();
}

void UDungeonSpotLightComponent::SetDefaultIntensity(const float InIntensity)
{
	LightFader.SetDefaultIntensity(InIntensity);
}

float UDungeonSpotLightComponent::GetDefaultIntensity() const
{
	return LightFader.GetDefaultIntensity();
}

void UDungeonSpotLightComponent::SetLightVolume(const float Volume)
{
	LightFader.SetVolume(Volume);
}

float UDungeonSpotLightComponent::GetLightVolume() const
{
	return LightFader.GetVolume();
}

bool UDungeonSpotLightComponent::IsLightFading() const
{
	return LightFader.IsFading();
}

void UDungeonSpotLightComponent::OnRegister()
{
	Super::OnRegister();
	LightFader.Initialize(this);
}

void UDungeonSpotLightComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	LightFader.Tick(DeltaTime);
	if (!LightFader.IsFading())
		SetComponentTickEnabled(false);
}

void UDungeonSpotLightComponent::EnableFadeTickIfNeeded()
{
	if (LightFader.IsFading())
		SetComponentTickEnabled(true);
}
