/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include "SubActor/DungeonLightFader.h"
#include <CoreMinimal.h>
#include <Components/SpotLightComponent.h>
#include "DungeonSpotLightComponent.generated.h"

/**
 * Dungeon spot light component with reusable fade and volume control.
 * 再利用可能なフェードとボリューム制御を持つ Dungeon 用 Spot Light コンポーネントです。
 */
UCLASS(ClassGroup = "DungeonGenerator", meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonSpotLightComponent : public USpotLightComponent
{
	GENERATED_BODY()

public:
	/**
	 * Creates a Dungeon spot light with fade ticking disabled until needed.
	 * 必要になるまでフェード Tick を無効にした Dungeon Spot Light を作成します。
	 */
	explicit UDungeonSpotLightComponent(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonSpotLightComponent() override = default;

	/**
	 * Starts fading from the current logical intensity to the requested target intensity.
	 * 現在の論理明るさから指定した目標明るさへのフェードを開始します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Light", meta = (ToolTip = "Fades this light from its current intensity to the target intensity. Use 0 seconds to apply immediately."))
	void FadeToIntensity(float TargetIntensity, float FadeTime);

	/**
	 * Fades this light to its stored default intensity.
	 * 保存済みの既定明るさへこのライトをフェードします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Light", meta = (ToolTip = "Fades this light on to its default intensity. Use 0 seconds to apply immediately."))
	void TurnOn(float FadeTime);

	/**
	 * Fades this light to zero and hides it when the final intensity is nearly zero.
	 * 明るさ0へフェードし、最終明るさがほぼ0になったら非表示にします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Light", meta = (ToolTip = "Fades this light off and hides it once the final intensity is nearly zero. Use 0 seconds to apply immediately."))
	void TurnOff(float FadeTime);

	/**
	 * Sets the authored intensity used by TurnOn.
	 * TurnOn で使用する作成時の明るさを設定します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Light", meta = (ToolTip = "Sets the default intensity used when this light fades on."))
	void SetDefaultIntensity(float InIntensity);

	/**
	 * Returns the authored intensity used by TurnOn.
	 * TurnOn で使用する作成時の明るさを返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|Light", meta = (ToolTip = "Returns the default intensity used when this light fades on."))
	float GetDefaultIntensity() const;

	/**
	 * Sets a 0-1 multiplier applied after fade intensity.
	 * フェード明るさの後に適用する 0-1 の倍率を設定します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator|Light", meta = (ToolTip = "Sets a 0-1 light volume multiplier applied after fade intensity. Use this for global or room-level light volume."))
	void SetLightVolume(float Volume);

	/**
	 * Returns the 0-1 light volume multiplier.
	 * 0-1 のライトボリューム倍率を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|Light", meta = (ToolTip = "Returns the 0-1 light volume multiplier applied after fade intensity."))
	float GetLightVolume() const;

	/**
	 * Returns whether this light is currently fading.
	 * このライトが現在フェード中か返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|Light", meta = (ToolTip = "Returns whether this light is currently fading."))
	bool IsLightFading() const;

	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnableFadeTickIfNeeded();

	/**
	 * Shared helper that owns the current fade state for this light.
	 * このライトの現在のフェード状態を保持する共有補助クラスです。
	 */
	FDungeonLightFader LightFader;
};
