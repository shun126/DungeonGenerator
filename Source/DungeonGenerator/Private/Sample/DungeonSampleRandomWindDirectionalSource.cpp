/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Sample/DungeonSampleRandomWindDirectionalSource.h"
#include <Engine/World.h>
#include <Components/WindDirectionalSourceComponent.h>
#include <Math/UnrealMathUtility.h>
#include <cmath>
#if WITH_EDITOR
#include <Kismet/KismetSystemLibrary.h>
#endif

namespace
{
	/*
	 * Returns true when the value is close enough to zero for smooth wind interpolation.
	 * 滑らかな風の補間でゼロとみなせるほど値が小さい場合にtrueを返します。
	 */
	static bool IsZero(const float value)
	{
		return std::abs(value) <= 0.01f;
	}
}

ADungeonSampleRandomWindDirectionalSource::ADungeonSampleRandomWindDirectionalSource(const FObjectInitializer& ObjectInitializer)
{
	// Tick 呼び出し許可
	PrimaryActorTick.bCanEverTick = true;

	// 初期化
	const auto rotator = GetActorRotation();
	CurrentYaw = rotator.Yaw;
	CurrentPitch = rotator.Pitch;
}

/*
 * Returns the shortest signed angular difference in degrees from the current angle to the target angle.
 * 現在角度から目標角度までの最短の符号付き角度差（度）を返します。
 */
static float GetRotationDegreeAngle(const float target, const float current)
{
	auto deltaAngle = target - current;
	if (deltaAngle > 180.f)
	{
		deltaAngle -= 360.f;
	}
	else if (deltaAngle < -180.f)
	{
		deltaAngle += 360.f;
	}
	return deltaAngle;
}

/*
 * Selects the next random wind target values and resets the remaining time.
 * 次のランダムな風の目標値を選択し、残り時間をリセットします。
 */
void ADungeonSampleRandomWindDirectionalSource::Change()
{
	Remain = FMath::FRandRange(MinTime, MaxTime);

	TargetStrength = FMath::FRandRange(MinStrength, MaxStrength);
	TargetSpeed = FMath::FRandRange(MinSpeed, MaxSpeed);

	TargetYaw = FMath::FRandRange(MinYaw, MaxYaw);
	TargetPitch = FMath::FRandRange(MinPitch, MaxPitch);
}

void ADungeonSampleRandomWindDirectionalSource::BeginPlay()
{
	Super::BeginPlay();

	Change();

	CurrentStrength = TargetStrength;
	CurrentSpeed = TargetSpeed;
	CurrentYaw = TargetYaw;
	CurrentPitch = TargetPitch;
}

void ADungeonSampleRandomWindDirectionalSource::Tick(float DeltaSeconds)
{
	// 親クラスを呼び出す
	Super::Tick(DeltaSeconds);

	// 残り時間が切れた？
	Remain -= DeltaSeconds;
	if (Remain <= 0.f)
	{
		Change();
	}

	// 風の強さを調整
	{
		auto deltaStrength = TargetStrength - CurrentStrength;
		if (IsZero(deltaStrength) == false)
		{
			if (deltaStrength > StrengthVelocity)
			{
				deltaStrength = StrengthVelocity;
			}
			else if (deltaStrength < -StrengthVelocity)
			{
				deltaStrength = -StrengthVelocity;
			}
			CurrentStrength += deltaStrength * DeltaSeconds;
		}
	}

	// 風の速度を調整
	{
		auto deltaSpeed = TargetSpeed - CurrentSpeed;
		if (IsZero(deltaSpeed) == false)
		{
			if (deltaSpeed > SpeedVelocity)
			{
				deltaSpeed = SpeedVelocity;
			}
			else if (deltaSpeed < -SpeedVelocity)
			{
				deltaSpeed = -SpeedVelocity;
			}
			CurrentSpeed += deltaSpeed * DeltaSeconds;
		}
	}

	// コンポーネントに反映
	if (auto* component = GetValid<UWindDirectionalSourceComponent>(GetComponent()))
	{
		component->SetStrength(CurrentStrength);
		component->SetSpeed(CurrentSpeed);
	}

	// 風向を調整
	{
		auto deltaYaw = GetRotationDegreeAngle(TargetYaw, CurrentYaw);
		auto deltaPitch = TargetPitch - CurrentPitch;
		if (IsZero(deltaYaw) == false && IsZero(deltaPitch) == false)
		{
			// Yaw
			if (deltaYaw > YawAngularVelocity)
			{
				deltaYaw = YawAngularVelocity;
			}
			else if (deltaYaw < -YawAngularVelocity)
			{
				deltaYaw = -YawAngularVelocity;
			}
			CurrentYaw += deltaYaw * DeltaSeconds;

			// Pitch
			if (deltaPitch > PitchAngularVelocity)
			{
				deltaPitch = PitchAngularVelocity;
			}
			else if (deltaPitch < -PitchAngularVelocity)
			{
				deltaPitch = -PitchAngularVelocity;
			}
			CurrentPitch += deltaPitch * DeltaSeconds;

			// Apply rotation
			SetActorRotation(FRotator(0.f, CurrentYaw, CurrentPitch));
		}
	}

#if WITH_EDITOR
	if (ShowDebugInformation)
	{
		const auto& location = GetActorLocation();

		auto message = GetName();
		message += TEXT("\nStrength : ") + FString::SanitizeFloat(CurrentStrength);
		message += TEXT("\nSpeed : ") + FString::SanitizeFloat(CurrentSpeed);
		message += TEXT("\nYaw : ") + FString::SanitizeFloat(CurrentYaw);
		message += TEXT("\nPitch: ") + FString::SanitizeFloat(CurrentPitch);
		message += TEXT("\nRemain: ") + FString::SanitizeFloat(Remain);
		UKismetSystemLibrary::DrawDebugString(
			GetWorld(),
			location,
			message,
			nullptr,
			FLinearColor::White
		);

		const auto yawRadian = CurrentYaw * (3.14159265359 / 180.0);
		const auto pitchRadian = CurrentPitch * (3.14159265359 / 180.0);
		const auto yawSin = std::sin(yawRadian);
		const auto yawCos = std::cos(yawRadian);
		const auto pitchSin = std::sin(pitchRadian);
		//const double pitchCos = std::cos(pitchRadian);


		auto velocity = FVector(
			CurrentSpeed * yawCos,
			CurrentSpeed * yawSin,
			CurrentSpeed * pitchSin
		);
		velocity *= 100.0;
		const auto arrowSize = velocity.Size() / 4.0;

		UKismetSystemLibrary::DrawDebugArrow(
			GetWorld(),
			location,
			location + velocity,
			arrowSize,
			FLinearColor::White,
			0.f,
			3.f
		);
	}
#endif
}
