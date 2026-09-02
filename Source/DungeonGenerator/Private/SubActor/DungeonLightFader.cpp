/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "SubActor/DungeonLightFader.h"
#include <Components/LightComponent.h>

void FDungeonLightFader::Initialize(ULightComponent* InLightComponent) noexcept
{
	LightComponent = InLightComponent;
	const float initialIntensity = IsValid(InLightComponent) ? FMath::Max(0.f, InLightComponent->Intensity) : 0.f;
	DefaultIntensity = initialIntensity;
	StartIntensity = initialIntensity;
	CurrentIntensity = initialIntensity;
	TargetIntensity = initialIntensity;
	FadeElapsed = 0.f;
	FadeDuration = 0.f;
	bFading = false;
	Apply();
}

void FDungeonLightFader::FadeToIntensity(const float InTargetIntensity, const float FadeTime) noexcept
{
	ULightComponent* lightComponent = LightComponent.Get();
	if (!IsValid(lightComponent))
		return;

	const float newTargetIntensity = FMath::Max(0.f, InTargetIntensity);
	if (FMath::IsNearlyEqual(TargetIntensity, newTargetIntensity, KINDA_SMALL_NUMBER))
	{
		if (newTargetIntensity > KINDA_SMALL_NUMBER)
			lightComponent->SetVisibility(true);
		Apply();
		return;
	}

	TargetIntensity = newTargetIntensity;
	StartIntensity = CurrentIntensity;
	FadeElapsed = 0.f;
	FadeDuration = FMath::Max(0.f, FadeTime);

	if (TargetIntensity > CurrentIntensity || TargetIntensity > KINDA_SMALL_NUMBER)
		lightComponent->SetVisibility(true);

	if (FadeDuration <= KINDA_SMALL_NUMBER)
	{
		CurrentIntensity = TargetIntensity;
		bFading = false;
		Apply();
		return;
	}

	bFading = true;
	Apply();
}

void FDungeonLightFader::TurnOn(const float FadeTime) noexcept
{
	FadeToIntensity(DefaultIntensity, FadeTime);
}

void FDungeonLightFader::TurnOff(const float FadeTime) noexcept
{
	FadeToIntensity(0.f, FadeTime);
}

void FDungeonLightFader::SetDefaultIntensity(const float InIntensity) noexcept
{
	DefaultIntensity = FMath::Max(0.f, InIntensity);
	if (!bFading)
	{
		CurrentIntensity = DefaultIntensity;
		TargetIntensity = DefaultIntensity;
		StartIntensity = DefaultIntensity;
		Apply();
	}
}

float FDungeonLightFader::GetDefaultIntensity() const noexcept
{
	return DefaultIntensity;
}

void FDungeonLightFader::SetVolume(const float InVolume) noexcept
{
	Volume = FMath::Clamp(InVolume, 0.f, 1.f);
	Apply();
}

float FDungeonLightFader::GetVolume() const noexcept
{
	return Volume;
}

void FDungeonLightFader::Tick(const float DeltaTime) noexcept
{
	if (!bFading)
		return;

	if (!IsValid(LightComponent.Get()))
	{
		bFading = false;
		return;
	}

	FadeElapsed += FMath::Max(0.f, DeltaTime);
	const float Alpha = FadeDuration <= KINDA_SMALL_NUMBER ? 1.f : FMath::Clamp(FadeElapsed / FadeDuration, 0.f, 1.f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);
	CurrentIntensity = FMath::Lerp(StartIntensity, TargetIntensity, SmoothAlpha);
	if (Alpha >= 1.f)
	{
		CurrentIntensity = TargetIntensity;
		bFading = false;
	}

	Apply();
}

bool FDungeonLightFader::IsFading() const noexcept
{
	return bFading;
}

float FDungeonLightFader::GetCurrentIntensity() const noexcept
{
	return CurrentIntensity;
}

float FDungeonLightFader::GetTargetIntensity() const noexcept
{
	return TargetIntensity;
}

void FDungeonLightFader::Apply() const noexcept
{
	ULightComponent* lightComponent = LightComponent.Get();
	if (!IsValid(lightComponent))
		return;

	const float finalIntensity = CalculateFinalIntensity();
	lightComponent->SetIntensity(finalIntensity);
	if (!bFading && finalIntensity <= KINDA_SMALL_NUMBER)
		lightComponent->SetVisibility(false);
}

float FDungeonLightFader::CalculateFinalIntensity() const noexcept
{
	return FMath::Max(0.f, CurrentIntensity) * FMath::Clamp(Volume, 0.f, 1.f);
}
