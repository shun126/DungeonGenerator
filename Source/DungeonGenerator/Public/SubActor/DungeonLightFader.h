/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <CoreMinimal.h>
#include <UObject/WeakObjectPtr.h>

class ULightComponent;

/**
 * Shared intensity fade helper for Dungeon point and spot lights.
 * Dungeon の Point Light と Spot Light で共有する明るさフェード補助クラスです。
 */
struct DUNGEONGENERATOR_API FDungeonLightFader final
{
	/**
	 * Captures the light component and initializes fade state from its current intensity.
	 * ライトコンポーネントを保持し、現在の明るさからフェード状態を初期化します。
	 */
	void Initialize(ULightComponent* InLightComponent) noexcept;

	/**
	 * Starts fading from the current logical intensity to the requested target intensity.
	 * 現在の論理明るさから指定した目標明るさへのフェードを開始します。
	 */
	void FadeToIntensity(float TargetIntensity, float FadeTime) noexcept;

	/**
	 * Fades the light toward the stored default intensity.
	 * 保存済みの既定明るさへライトをフェードします。
	 */
	void TurnOn(float FadeTime) noexcept;

	/**
	 * Fades the light toward zero intensity.
	 * 明るさ0へライトをフェードします。
	 */
	void TurnOff(float FadeTime) noexcept;

	/**
	 * Sets the authored intensity used by TurnOn.
	 * TurnOn で使用する作成時の明るさを設定します。
	 */
	void SetDefaultIntensity(float InIntensity) noexcept;

	/**
	 * Returns the authored intensity used by TurnOn.
	 * TurnOn で使用する作成時の明るさを返します。
	 */
	float GetDefaultIntensity() const noexcept;

	/**
	 * Sets a master volume multiplier applied to the current intensity.
	 * 現在の明るさに適用するマスターボリューム倍率を設定します。
	 */
	void SetVolume(float InVolume) noexcept;

	/**
	 * Returns the master volume multiplier.
	 * マスターボリューム倍率を返します。
	 */
	float GetVolume() const noexcept;

	/**
	 * Advances the fade and applies the final intensity to the light.
	 * フェードを進め、最終明るさをライトへ反映します。
	 */
	void Tick(float DeltaTime) noexcept;

	/**
	 * Returns whether a fade is currently active.
	 * 現在フェード中か返します。
	 */
	bool IsFading() const noexcept;

	/**
	 * Returns the current logical intensity before volume is applied.
	 * ボリューム適用前の現在の論理明るさを返します。
	 */
	float GetCurrentIntensity() const noexcept;

	/**
	 * Returns the target logical intensity before volume is applied.
	 * ボリューム適用前の目標の論理明るさを返します。
	 */
	float GetTargetIntensity() const noexcept;

private:
	void Apply() const noexcept;
	float CalculateFinalIntensity() const noexcept;

	TWeakObjectPtr<ULightComponent> LightComponent;
	float DefaultIntensity = 0.f;
	float StartIntensity = 0.f;
	float CurrentIntensity = 0.f;
	float TargetIntensity = 0.f;
	float FadeElapsed = 0.f;
	float FadeDuration = 0.f;
	float Volume = 1.f;
	bool bFading = false;
};
