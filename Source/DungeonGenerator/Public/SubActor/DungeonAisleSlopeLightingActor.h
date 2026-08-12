/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include "Parameter/DungeonLayoutTypes.h"
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "DungeonAisleSlopeLightingActor.generated.h"

class UDungeonComponentActivatorComponent;
class UPointLightComponent;
class USceneComponent;

/*
 * Owns the partition-controlled base light generated for one aisle slope.
 * 1つの通路スロープ用に生成される、Partition制御対応ベースライトを所有します。
 */
UCLASS(NotPlaceable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonAisleSlopeLightingActor : public AActor
{
	GENERATED_BODY()

public:
	/*
	 * Creates the light and partition-control components.
	 * ライトとPartition制御コンポーネントを作成します。
	 */
	explicit ADungeonAisleSlopeLightingActor(const FObjectInitializer& initializer);

	/*
	 * Applies resolved settings and registers this actor at the generated slope location.
	 * 解決済み設定を適用し、このActorを生成スロープ位置へ登録します。
	 */
	void Initialize(const FDungeonAisleSlopeBaseLightSettings& settings, const FVector& partitionRegistrationWorldLocation);

	/*
	 * Returns the generated base-light component.
	 * 生成されたベースライトコンポーネントを返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|AisleSlopeBaseLight", meta = (ToolTip = "Returns the shadow-free Point Light generated for this aisle slope."))
	UPointLightComponent* GetPointLightComponent() const noexcept;

	/*
	 * Returns the component that controls visibility by dungeon partition distance.
	 * ダンジョンPartition距離に応じて表示を制御するコンポーネントを返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator|AisleSlopeBaseLight", meta = (ToolTip = "Returns the component that controls this light's visibility by dungeon partition distance."))
	UDungeonComponentActivatorComponent* GetComponentActivator() const noexcept;

	/*
	 * Calculates the world location above the midpoint of an aisle slope.
	 * 通路スロープ中央面の上方にあるワールド位置を計算します。
	 */
	static FVector CalculateLightLocation(const FVector& slopeOrigin, const FVector& slopeDirection, float horizontalGridSize, float verticalGridSize, float heightOffset) noexcept;

	/** Returns the properties replicated by this generated visual actor. この生成表示Actorが同期するプロパティを返します。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	/** Applies the resolved settings received from the server. サーバーから受信した解決済み設定を適用します。 */
	UFUNCTION()
	void OnRep_Settings();
	/*
	 * Root used to keep the generated light at the slope midpoint.
	 * 生成ライトをスロープ中央に保持するためのRootです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|AisleSlopeBaseLight", meta = (ToolTip = "Root component positioned above the midpoint of the generated aisle slope."))
	TObjectPtr<USceneComponent> SceneRoot;

	/*
	 * Shadow-free Point Light that provides minimum visibility on the aisle slope.
	 * 通路スロープの最低限の視認性を確保する、影なしのPoint Lightです。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|AisleSlopeBaseLight", meta = (ToolTip = "Shadow-free Point Light that provides minimum visibility on this generated aisle slope."))
	TObjectPtr<UPointLightComponent> PointLightComponent;

	/*
	 * Controls the generated light visibility using dungeon partitions.
	 * ダンジョンPartitionを使用して生成ライトの表示を制御します。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|AisleSlopeBaseLight", meta = (ToolTip = "Controls the generated Point Light visibility by dungeon partition distance."))
	TObjectPtr<UDungeonComponentActivatorComponent> ComponentActivator;

	/**
	 * Resolved aisle-slope settings synchronized for existing and late-joining clients.
	 * 接続中および途中参加クライアントへ同期する、解決済みの通路スロープ設定です。
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Settings)
	FDungeonAisleSlopeBaseLightSettings Settings;

private:
	static void ConfigurePointLight(UPointLightComponent* pointLightComponent, const FDungeonAisleSlopeBaseLightSettings& settings) noexcept;
};
