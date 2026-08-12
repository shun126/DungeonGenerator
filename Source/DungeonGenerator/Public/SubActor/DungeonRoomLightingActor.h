/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include "DungeonRoomLightingActor.generated.h"


class UPointLightComponent;
class USceneComponent;
class USpotLightComponent;
class UDungeonComponentActivatorComponent;

/**
 * Complete replicated visual state used to construct one room's generated lights.
 * 1つの部屋の生成ライトを構築するために使用する、完全な同期対象の表示状態です。
 */
USTRUCT()
struct DUNGEONGENERATOR_API FDungeonRoomLightingState
{
	GENERATED_BODY()

	/**
	 * Stable generated-room identifier.
	 * 生成部屋の安定した識別子です。
	 */
	UPROPERTY()
	int32 Identifier = 0;

	/**
	 * Revision changed only when component layout must be rebuilt.
	 * Component配置の再構築が必要な場合だけ更新されるRevisionです。
	 */
	UPROPERTY()
	uint32 LayoutRevision = 0;

	/**
	 * World-space room bounds used for light placement.
	 * ライト配置に使用するワールド空間の部屋Boundsです。
	 */
	UPROPERTY()
	FBox RoomBounds = FBox(ForceInit);

	/**
	 * Vertical dungeon-grid size used by guidance placement.
	 * Guidance配置に使用する垂直ダンジョングリッドサイズです。
	 */
	UPROPERTY()
	float VerticalGridSize = 0.f;

	/** Enables base fill-light generation. Base Fill Light生成を有効にします。 */
	UPROPERTY()
	bool bEnableBaseFillLights = false;

	/** Intensity units for base fill lights. Base Fill Lightの明るさ単位です。 */
	UPROPERTY()
	ELightUnits BaseFillLightIntensityUnits = ELightUnits::Unitless;

	/** Current base fill-light intensity. 現在のBase Fill Lightの明るさです。 */
	UPROPERTY()
	float BaseFillLightIntensity = 0.f;

	/** Base fill-light color. Base Fill Lightの色です。 */
	UPROPERTY()
	FLinearColor BaseFillLightColor = FLinearColor::White;

	/** Target cell size for base fill-light placement. Base Fill Light配置の目標セルサイズです。 */
	UPROPERTY()
	float BaseFillLightCellSize = 1200.f;

	/** Maximum base fill-light count on X. X方向のBase Fill Light最大数です。 */
	UPROPERTY()
	int32 MaxBaseFillLightCountX = 3;

	/** Maximum base fill-light count on Y. Y方向のBase Fill Light最大数です。 */
	UPROPERTY()
	int32 MaxBaseFillLightCountY = 3;

	/** Height ratio measured from the room floor. 部屋の床から測る高さの比率です。 */
	UPROPERTY()
	float BaseFillLightHeightFromFloorRatio = 0.6f;

	/** Scale applied to calculated base fill attenuation. Base Fill Lightの自動減衰半径に適用する倍率です。 */
	UPROPERTY()
	float BaseFillLightAttenuationScale = 1.05f;

	/** Minimum base fill attenuation radius. Base Fill Lightの最小減衰半径です。 */
	UPROPERTY()
	float BaseFillLightMinAttenuationRadius = 100.f;

	/** Maximum base fill attenuation radius. Base Fill Lightの最大減衰半径です。 */
	UPROPERTY()
	float BaseFillLightMaxAttenuationRadius = 2000.f;

	/** Enables guidance-light generation. Guidance Light生成を有効にします。 */
	UPROPERTY()
	bool bEnableGuidanceLights = true;

	/** Enables door guidance lights. ドア用Guidance Lightを有効にします。 */
	UPROPERTY()
	bool bEnableDoorGuidanceLights = true;

	/** Enables stair guidance lights. 階段用Guidance Lightを有効にします。 */
	UPROPERTY()
	bool bEnableStairGuidanceLights = false;

	/** Intensity units for guidance lights. Guidance Lightの明るさ単位です。 */
	UPROPERTY()
	ELightUnits GuidanceLightIntensityUnits = ELightUnits::Candelas;

	/** Current guidance-light base intensity. 現在のGuidance Lightの基準明るさです。 */
	UPROPERTY()
	float GuidanceLightIntensity = 0.f;

	/** Guidance-light color. Guidance Lightの色です。 */
	UPROPERTY()
	FLinearColor GuidanceLightColor = FLinearColor::White;

	/** Guidance-light attenuation radius. Guidance Lightの減衰半径です。 */
	UPROPERTY()
	float GuidanceLightAttenuationRadius = 1200.f;

	/** Guidance-light inner cone angle. Guidance Lightの内側コーン角度です。 */
	UPROPERTY()
	float GuidanceLightInnerConeAngle = 18.f;

	/** Guidance-light outer cone angle. Guidance Lightの外側コーン角度です。 */
	UPROPERTY()
	float GuidanceLightOuterConeAngle = 32.f;

	/** Horizontal offset from a guidance target. Guidance Targetからの水平オフセットです。 */
	UPROPERTY()
	float GuidanceLightHorizontalOffset = 300.f;

	/** Door-light height offset in vertical-grid units. 垂直グリッド単位のドアライト高さオフセットです。 */
	UPROPERTY()
	float DoorGuidanceLightHeightOffsetScale = 0.5f;

	/** Stair-light distance below the ceiling. 階段ライトの天井からの距離です。 */
	UPROPERTY()
	float StairGuidanceLightHeightBelowCeiling = 300.f;

	/** Illuminated target height above the floor. 床から測る照射目標の高さです。 */
	UPROPERTY()
	float GuidanceLightTargetHeightAboveFloor = 100.f;

	/** Door guidance-light intensity multiplier. ドア用Guidance Lightの明るさ倍率です。 */
	UPROPERTY()
	float DoorGuidanceLightIntensityMultiplier = 1.f;

	/** Stair guidance-light intensity multiplier. 階段用Guidance Lightの明るさ倍率です。 */
	UPROPERTY()
	float StairGuidanceLightIntensityMultiplier = 1.f;

	/** Maximum number of guidance lights in the room. 部屋内のGuidance Light最大数です。 */
	UPROPERTY()
	int32 MaxGuidanceLightCount = 8;

	/** Collected door and stair guidance targets. 収集されたドアおよび階段のGuidance Targetです。 */
	UPROPERTY()
	TArray<FDungeonGuidanceTarget> GuidanceTargets;
};

/**
 * Replicated visual proxy that locally constructs generated room lights on every machine.
 * 各端末で生成部屋ライトをローカル構築する、同期対応の表示専用Proxyです。
 */
UCLASS(NotPlaceable, Transient)
class DUNGEONGENERATOR_API ADungeonRoomLightingActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * Creates the replicated room-lighting proxy.
	 * 同期対応の部屋ライトProxyを生成します。
	 */
	explicit ADungeonRoomLightingActor(const FObjectInitializer& initializer);

	/**
	 * Copies the complete generated-light state from the authoritative Room Sensor.
	 * Authorityを持つRoom Sensorから生成ライトの完全な状態をコピーします。
	 */
	void InitializeFromRoomSensor(ADungeonRoomSensorBase* roomSensor);

	/**
	 * Changes the base fill-light intensity and synchronizes the current value.
	 * Base Fill Lightの明るさを変更し、現在値を同期します。
	 */
	void SetBaseFillLightIntensity(float intensity);

	/**
	 * Changes the guidance-light base intensity and synchronizes the current value.
	 * Guidance Lightの基準明るさを変更し、現在値を同期します。
	 */
	void SetGuidanceLightIntensity(float intensity);

	/** Returns the synchronized base fill-light intensity. 同期中のBase Fill Lightの明るさを返します。 */
	float GetBaseFillLightIntensity() const noexcept;

	/** Returns the synchronized guidance-light base intensity. 同期中のGuidance Lightの基準明るさを返します。 */
	float GetGuidanceLightIntensity() const noexcept;

	/**
	 * Shifts replicated world-space bounds and guidance targets with the generated dungeon.
	 * 生成ダンジョンとともに同期対象のワールド空間BoundsとGuidance Targetを移動します。
	 */
	void ShiftWorldOffset(const FVector& delta);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Applies replicated state by rebuilding or updating local light components. 同期された状態からローカルライトを再構築または更新します。 */
	UFUNCTION()
	void OnRep_LightingState();

private:
	void RebuildLightComponents();
	void DestroyLightComponents();
	void ApplyIntensities();
	void WakeForReplication();

protected:
	UPROPERTY(VisibleAnywhere, Category = "DungeonGenerator")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "DungeonGenerator")
	TObjectPtr<UDungeonComponentActivatorComponent> ComponentActivator;

	UPROPERTY(ReplicatedUsing = OnRep_LightingState)
	FDungeonRoomLightingState LightingState;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> BaseFillLightComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USpotLightComponent>> GuidanceLightComponents;

	UPROPERTY(Transient)
	TArray<EDungeonGuidanceTargetType> GuidanceLightTargetTypes;

private:
	uint32 AppliedLayoutRevision = 0;
};
