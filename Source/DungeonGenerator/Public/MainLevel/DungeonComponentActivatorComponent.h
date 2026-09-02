/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonComponentActivationSaver.h"
#include <CoreMinimal.h>
#include <Components/PointLightComponent.h>
#include <GameFramework/Actor.h>
#include <Math/Box.h>
#include <bitset>
#include <vector>
#include "DungeonComponentActivatorComponent.generated.h"

class ADungeonMainLevelScriptActor;
class ADungeonGenerateBase;
class UDungeonPartition;
class UPointLightComponent;

/**
 * Facing axis used by the managed point and spot light visibility control.
 * 管理対象のポイントライトとスポットライトの表示制御で使用する正面軸です。
 */
UENUM(BlueprintType)
enum class EDungeonManagedLightFacingAxis : uint8
{
	PositiveX UMETA(DisplayName = "X+", ToolTip = "Use the owner's local positive X axis as the light-facing direction."),
	NegativeX UMETA(DisplayName = "X-", ToolTip = "Use the owner's local negative X axis as the light-facing direction."),
	PositiveY UMETA(DisplayName = "Y+", ToolTip = "Use the owner's local positive Y axis as the light-facing direction."),
	NegativeY UMETA(DisplayName = "Y-", ToolTip = "Use the owner's local negative Y axis as the light-facing direction."),
	PositiveZ UMETA(DisplayName = "Z+", ToolTip = "Use the owner's local positive Z axis as the light-facing direction."),
	NegativeZ UMETA(DisplayName = "Z-", ToolTip = "Use the owner's local negative Z axis as the light-facing direction.")
};

/**
 * Stores the authored and runtime-managed state of a point-light-derived component.
 * ポイントライト派生コンポーネントの作成時状態と実行時管理状態を保持します。
 */
struct FDungeonControlledPointAndSpotLight final
{
	/**
	 * Weak reference to the managed light component.
	 * 管理対象ライトコンポーネントへの弱参照です。
	 */
	TWeakObjectPtr<UPointLightComponent> Component;

	/**
	 * Visibility recorded when runtime control begins.
	 * 実行時制御開始時に記録した表示状態です。
	 */
	bool InitialVisibility = true;

	/**
	 * Cast-shadow state recorded when runtime control begins.
	 * 実行時制御開始時に記録した影生成状態です。
	 */
	bool InitialCastShadows = true;

	/**
	 * Last state selected by identifier and facing-angle control.
	 * Identifierと正面角度の制御で最後に選択された状態です。
	 */
	bool EnabledByManager = true;
};

/**
 * Enum definition for EDungeonComponentActivateReason.
 *
 * EDungeonComponentActivateReason の列挙型定義です。
 */
UENUM(Blueprintable)
enum class EDungeonComponentActivateReason : uint8
{
	Partition UMETA(DisplayName = "Partition", ToolTip = "Activation state changed by partition distance logic."),
	Demo UMETA(DisplayName = "Demo", ToolTip = "Activation state changed for demo or preview behavior."),
	Custom UMETA(DisplayName = "Custom", ToolTip = "Activation state changed by custom game logic.")
};
constexpr uint8_t DungeonComponentActivateReasonSize = 3;

/**
 * OnPartitionActivate will be called when the DungeonPartition belonging to
 * the player approaches the vicinity of the player.
 * OnPartitionInactivate is called when the DungeonPartition moves away.
 *
 * The shadow control method for point light derived classes has been changed.
 * Only lights with CastShadow enabled at the time of BeginPlay will have shadow enable/disable control.
 *
 * 所属しているDungeonPartitionがプレイヤー周辺に近づいたらOnPartitionActivateが呼ばれます。
 * 離れたらOnPartitionInactivateが呼ばれます。
 *
 * ポイントライト派生クラスの影の制御方法が変更されました。
 * BeginPlay時点でCastShadowが有効のライトのみ、影の有効無効制御が行われます。
 */
UCLASS(ClassGroup = "DungeonGenerator", meta = (BlueprintSpawnableComponent))
class DUNGEONGENERATOR_API UDungeonComponentActivatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	explicit UDungeonComponentActivatorComponent(const FObjectInitializer& objectInitializer);
	virtual ~UDungeonComponentActivatorComponent() override = default;

	bool IsEnableOwnerActorTickControl() const noexcept;
	bool IsEnableOwnerActorAiControl() const noexcept;
	bool IsEnableComponentActivationControl() const noexcept;
	bool IsEnableComponentVisibilityControl() const noexcept;
	bool IsEnableCollisionEnableControl() const noexcept;

	void SetEnableOwnerActorTickControl(const bool enable = true) noexcept;
	void SetEnableOwnerActorAiControl(const bool enable = true) noexcept;
	void SetEnableComponentActivationControl(const bool enable = true) noexcept;
	void SetEnableComponentVisibilityControl(const bool enable = true) noexcept;
	void SetEnableCollisionEnableControl(const bool enable = true) noexcept;
	void SetManagedLightFacingAxis(EDungeonManagedLightFacingAxis facingAxis) noexcept;
	FVector GetManagedLightFacingDirection(const AActor* ownerActor) const noexcept;

	/**
	 * このコンポーネントが所属する生成グリッドのIdentifierを設定します。
	 */
	void SetGridIdentifier(uint16 identifier) noexcept;

	/**
	 * Clears the generated-grid identifier associated with this component.
	 * このコンポーネントに関連付けられた生成グリッドのIdentifierを解除します。
	 */
	void ResetGridIdentifier() noexcept;

	/**
	 * このコンポーネントが有効な生成グリッドIdentifierを持つか返します。
	 */
	bool HasGridIdentifier() const noexcept;

	/**
	 * このコンポーネントに関連付けられた生成グリッドIdentifierを返します。
	 */
	uint16 GetGridIdentifier() const noexcept;

	/**
	 * Sets FixedPartitionRegistrationWorldLocation.
	 *
	 * パーティエーション登録に使用する固定ワールド座標を設定します。
	 */
	void SetFixedPartitionRegistrationWorldLocation(const FVector& worldLocation) noexcept;

	/**
	 * Saves the owner actor Tick state and disables Tick for the specified reason.
	 * 指定した理由でオーナーアクターのTick状態を保存し、Tickを無効にします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Saves the owner actor Tick state and disables Tick for the specified reason."))
	void SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason);

	/**
	 * Restores the owner actor Tick state saved for the specified reason.
	 * 指定した理由で保存したオーナーアクターのTick状態を復元します。
	 */
	void LoadActorTickEnable(const EDungeonComponentActivateReason activateReason);

	/**
	 * Saves component activation states and disables components for the specified reason.
	 * 指定した理由でコンポーネントのアクティブ状態を保存し、無効にします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Saves component activation states and disables components for the specified reason."))
	void SaveAndDisableComponentActivation(const EDungeonComponentActivateReason activateReason);

	/**
	 * Restores component activation states saved for the specified reason.
	 * 指定した理由で保存したコンポーネントのアクティブ状態を復元します。
	 */
	void LoadComponentActivation(const EDungeonComponentActivateReason activateReason);

	/**
	 * Saves collision states and disables collision for the specified reason.
	 * 指定した理由でコリジョン状態を保存し、コリジョンを無効にします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Saves collision states and disables collision for the specified reason."))
	void SaveAndDisableCollisionEnable(const EDungeonComponentActivateReason activateReason);

	/**
	 * Restores collision states saved for the specified reason.
	 * 指定した理由で保存したコリジョン状態を復元します。
	 */
	void LoadCollisionEnable(const EDungeonComponentActivateReason activateReason);

	/**
	 * Saves visibility states and hides components for the specified reason.
	 * 指定した理由で表示状態を保存し、コンポーネントを非表示にします。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Saves visibility states and hides components for the specified reason."))
	void SaveAndDisableVisibility(const EDungeonComponentActivateReason activateReason);

	/**
	 * Restores visibility states saved for the specified reason.
	 * 指定した理由で保存した表示状態を復元します。
	 */
	void LoadVisibility(const EDungeonComponentActivateReason activateReason);

	/**
	 * Saves AI logic state and stops AI logic for the specified reason.
	 * 指定した理由でAIロジック状態を保存し、AIロジックを停止します。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Saves AI logic state and stops AI logic for the specified reason."))
	void SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason);

	/**
	 * Restores AI logic state saved for the specified reason.
	 * 指定した理由で保存したAIロジック状態を復元します。
	 */
	void LoadAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason);

	/**
	 * Visits every point or spot light managed by this component.
	 * このコンポーネントが管理する全てのポイントライトまたはスポットライトを巡回します。
	 */
	void EachControlledPointAndSpotLight(const std::function<void(FDungeonControlledPointAndSpotLight&)>& function);

	/**
	 * Restores managed point and spot lights to their states recorded at BeginPlay.
	 * 管理対象のポイントライトとスポットライトをBeginPlay時に記録した状態へ復元します。
	 */
	void RestoreControlledPointAndSpotLightStates();

#if 0
	// コンポーネントを更新します
	template<typename T>
	void EachComponent(const std::function<void(T&)>& function) const;
#endif

	// overrides
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
	virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

protected:
	/**
	 * 所属しているDungeonPartitionがプレイヤー周辺に近づいたら呼び出されます。
	 * The DungeonPartition will be called when it approaches the player's vicinity.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (ToolTip = "Called when this component's dungeon partition enters the active range around a player."))
	void OnPartitionActivate();

	/**
	 * 所属しているDungeonPartitionがプレイヤー周辺から離れたら呼び出されます。
	 * It is called when the DungeonPartition to which it belongs leaves the player's vicinity.
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (ToolTip = "Called when this component's dungeon partition leaves the active range around all players."))
	void OnPartitionInactivate();

private:
	/**
	 * Shifts the fixed world location used for partition registration by the specified world offset.
	 * パーティション登録に使用する固定ワールド座標を、指定したワールドオフセット分だけ移動します。
	 */
	void ShiftFixedPartitionRegistrationWorldLocation(const FVector& worldOffset) noexcept;

	void RefreshPartitionRegistration(ADungeonMainLevelScriptActor* dungeonMainLevelScriptActor);
	void TickImplement(const FVector& location);
	FVector GetPartitionRegistrationWorldLocation(const AActor* ownerActor) const noexcept;
	bool HasFixedPartitionRegistrationWorldLocation() const noexcept;
	void CallPartitionActivate();
	void CallPartitionInactivate();

	// Actor
	void SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason, AActor* owner);

	// Component
	void SaveAndDisableComponentActivation(const EDungeonComponentActivateReason activateReason, const AActor* owner);

	// Collision
	void SaveAndDisableCollisionEnable(const EDungeonComponentActivateReason activateReason, const AActor* owner);

	// Visibility
	void SaveAndDisableVisibility(const EDungeonComponentActivateReason activateReason, const AActor* owner);

	// AI
	void SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason, const APawn* owner);

protected:
	/**
	 * If enabled, controls the validity of the owner actor's Tick
	 * 有効にするとオーナーアクターのTickの有効性を制御します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "If enabled, controls the validity of the owner actor's Tick"))
	bool EnableOwnerActorTickControl = true;

	/**
	 * If enabled, controls the effectiveness of the owner actor's AI
	 * 有効にするとオーナーアクターのAIの有効性を制御します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "If enabled, controls the effectiveness of the owner actor's AI"))
	bool EnableOwnerActorAiControl = true;

	/**
	 * If enabled, controls the activation of the owner actor's components
	 * 有効にするとオーナーアクターのコンポーネントのアクティブ性を制御します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "If enabled, controls the activation of the owner actor's components"))
	bool EnableComponentActivationControl = true;

	/**
	 * If enabled, controls the visibility of the owner actor's components
	 * 有効にするとオーナーアクターのコンポーネントの表示を制御します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "If enabled, controls the visibility of the owner actor's components"))
	bool EnableComponentVisibilityControl = true;

	/**
	 * Owner-local axis treated as the front direction for managed light angle culling.
	 * 管理対象ライトの角度カリングで正面として扱う、オーナー Actor のローカル軸です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ToolTip = "Owner-local axis used as the front direction for managed point and spot light angle culling. Use Y+ for torches, Z- for chandeliers, and Z+ for most other actors."))
	EDungeonManagedLightFacingAxis ManagedLightFacingAxis = EDungeonManagedLightFacingAxis::PositiveZ;

	/**
	 * If enabled, controls the enable of the collision component of the owner actor
	 * 有効にするとオーナーアクターのコリジョンコンポーネントの有効性を制御します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "If enabled, controls the enable of the collision component of the owner actor"))
	bool EnableCollisionEnableControl = true;

private:
	// Actor
	std::bitset<DungeonComponentActivateReasonSize> mIsTickEnabled = ~0;
	std::bitset<DungeonComponentActivateReasonSize> mComponentActivation = ~0;
	std::bitset<DungeonComponentActivateReasonSize> mComponentVisibility = ~0;
	std::bitset<DungeonComponentActivateReasonSize> mComponentCollisionEnabled = ~0;
	std::bitset<DungeonComponentActivateReasonSize> mLogicEnabled = ~0;

	DungeonComponentActivationSaver<bool> mComponentActivationSaver;
	DungeonComponentActivationSaver<bool> mComponentVisibilitySaver;
	DungeonComponentActivationSaver<ECollisionEnabled::Type> mComponentCollisionEnabledSaver;
	/**
	 * Point and spot lights whose authored CastShadow state was enabled at BeginPlay.
	 * BeginPlay時にCastShadowが有効だった制御対象のポイントライトとスポットライトです。
	 */
	std::vector<FDungeonControlledPointAndSpotLight> mControlledPointAndSpotLights;

	TWeakObjectPtr<ADungeonMainLevelScriptActor> mDungeonLevelScriptActor;
	TWeakObjectPtr<UDungeonPartition> mLastDungeonPartition;
	FVector mFixedPartitionRegistrationWorldLocation = FVector::ZeroVector;
	FVector mLastLocation = FVector::ZeroVector;

	/**
	 * Identifier of the generated grid that owns this component.
	 * このコンポーネントが所属する生成グリッドのIdentifierです。
	 */
	uint16 mGridIdentifier = 0;

	bool bHasFixedPartitionRegistrationWorldLocation = false;

	/**
	 * Whether mGridIdentifier currently contains a valid value.
	 * mGridIdentifierが現在有効な値を保持しているかを表します。
	 */
	bool bHasGridIdentifier = false;
	bool mTickSaver = false;

	friend class ADungeonMainLevelScriptActor;
	friend class ADungeonGenerateBase;
	friend class UDungeonPartition;
};

inline bool UDungeonComponentActivatorComponent::IsEnableOwnerActorTickControl() const noexcept
{
	return EnableOwnerActorTickControl;
}

inline bool UDungeonComponentActivatorComponent::IsEnableOwnerActorAiControl() const noexcept
{
	return EnableOwnerActorAiControl;
}

inline bool UDungeonComponentActivatorComponent::IsEnableComponentActivationControl() const noexcept
{
	return EnableComponentActivationControl;
}

inline bool UDungeonComponentActivatorComponent::IsEnableComponentVisibilityControl() const noexcept
{
	return EnableComponentVisibilityControl;
}

inline bool UDungeonComponentActivatorComponent::IsEnableCollisionEnableControl() const noexcept
{
	return EnableCollisionEnableControl;
}

inline void UDungeonComponentActivatorComponent::SetEnableOwnerActorTickControl(const bool enable) noexcept
{
	EnableOwnerActorTickControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetEnableOwnerActorAiControl(const bool enable) noexcept
{
	EnableOwnerActorAiControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetEnableComponentActivationControl(const bool enable) noexcept
{
	EnableComponentActivationControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetEnableComponentVisibilityControl(const bool enable) noexcept
{
	EnableComponentVisibilityControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetEnableCollisionEnableControl(const bool enable) noexcept
{
	EnableCollisionEnableControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetManagedLightFacingAxis(const EDungeonManagedLightFacingAxis facingAxis) noexcept
{
	ManagedLightFacingAxis = facingAxis;
}

inline FVector UDungeonComponentActivatorComponent::GetManagedLightFacingDirection(const AActor* ownerActor) const noexcept
{
	if (!IsValid(ownerActor))
		return FVector::UpVector;

	switch (ManagedLightFacingAxis)
	{
	case EDungeonManagedLightFacingAxis::PositiveX:
		return ownerActor->GetActorForwardVector();
	case EDungeonManagedLightFacingAxis::NegativeX:
		return -ownerActor->GetActorForwardVector();
	case EDungeonManagedLightFacingAxis::PositiveY:
		return ownerActor->GetActorRightVector();
	case EDungeonManagedLightFacingAxis::NegativeY:
		return -ownerActor->GetActorRightVector();
	case EDungeonManagedLightFacingAxis::PositiveZ:
		return ownerActor->GetActorUpVector();
	case EDungeonManagedLightFacingAxis::NegativeZ:
		return -ownerActor->GetActorUpVector();
	default:
		return ownerActor->GetActorUpVector();
	}
}

inline void UDungeonComponentActivatorComponent::SetGridIdentifier(const uint16 identifier) noexcept
{
	mGridIdentifier = identifier;
	bHasGridIdentifier = true;
}

inline void UDungeonComponentActivatorComponent::ResetGridIdentifier() noexcept
{
	mGridIdentifier = 0;
	bHasGridIdentifier = false;
}

inline bool UDungeonComponentActivatorComponent::HasGridIdentifier() const noexcept
{
	return bHasGridIdentifier;
}

inline uint16 UDungeonComponentActivatorComponent::GetGridIdentifier() const noexcept
{
	return mGridIdentifier;
}

inline void UDungeonComponentActivatorComponent::SetFixedPartitionRegistrationWorldLocation(const FVector& worldLocation) noexcept
{
	bHasFixedPartitionRegistrationWorldLocation = true;
	mFixedPartitionRegistrationWorldLocation = worldLocation;
}

inline void UDungeonComponentActivatorComponent::ShiftFixedPartitionRegistrationWorldLocation(const FVector& worldOffset) noexcept
{
	if (bHasFixedPartitionRegistrationWorldLocation)
		mFixedPartitionRegistrationWorldLocation += worldOffset;
}

inline void UDungeonComponentActivatorComponent::EachControlledPointAndSpotLight(const std::function<void(FDungeonControlledPointAndSpotLight&)>& function)
{
	for (FDungeonControlledPointAndSpotLight& controlledLight : mControlledPointAndSpotLights)
	{
		if (IsValid(controlledLight.Component.Get()))
		{
			function(controlledLight);
		}
	}
}

#if 0
template<typename T>
void UDungeonComponentActivatorComponent::EachComponent(const std::function<void(T&)>& function) const
{
	static_assert(std::is_base_of_v<UActorComponent, T> , "Specify the class from which UActorComponent is derived");
	if (const auto* ownerActor = GetOwner())
	{
		for (auto* component : ownerActor->GetComponents())
		{
			if (auto* targetComponent = GetValid(Cast<T>(component)))
				function(*targetComponent);
		}
	}
}
#endif

