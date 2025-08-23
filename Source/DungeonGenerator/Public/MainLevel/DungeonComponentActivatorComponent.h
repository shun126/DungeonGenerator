/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonComponentActivationSaver.h"
#include <CoreMinimal.h>
#include <Components/PointLightComponent.h>
#include <Math/Box.h>
#include <bitset>
#include <vector>
#include "DungeonComponentActivatorComponent.generated.h"

class ADungeonMainLevelScriptActor;
class UDungeonPartition;
class UPointLightComponent;

UENUM(Blueprintable)
enum class EDungeonComponentActivateReason : uint8
{
	Partition,
	Demo,
	Custom
};
constexpr uint8_t DungeonComponentActivateReasonSize = 3;

/**
OnPartitionActivate will be called when the DungeonPartition belonging to
the player approaches the vicinity of the player.
OnPartitionInactivate is called when the DungeonPartition moves away.

The shadow control method for point light derived classes has been changed.
Only lights with CastShadow enabled at the time of BeginPlay will have shadow enable/disable control.

所属しているDungeonPartitionがプレイヤー周辺に近づいたらOnPartitionActivateが呼ばれます。
離れたらOnPartitionInactivateが呼ばれます。

ポイントライト派生クラスの影の制御方法が変更されました。
BeginPlay時点でCastShadowが有効のライトのみ、影の有効無効制御が行われます。
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
	bool IsEnableLightCastShadowControl() const noexcept;
	bool IsEnableCollisionEnableControl() const noexcept;

	void SetEnableOwnerActorTickControl(const bool enable = true) noexcept;
	void SetEnableOwnerActorAiControl(const bool enable = true) noexcept;
	void SetEnableComponentActivationControl(const bool enable = true) noexcept;
	void SetEnableComponentVisibilityControl(const bool enable = true) noexcept;
	void SetEnableLightCastShadowControl(const bool enable = true) noexcept;
	void SetEnableCollisionEnableControl(const bool enable = true) noexcept;

	// Actor
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason);
	void LoadActorTickEnable(const EDungeonComponentActivateReason activateReason);

	// Component
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SaveAndDisableComponentActivation(const EDungeonComponentActivateReason activateReason);
	void LoadComponentActivation(const EDungeonComponentActivateReason activateReason);

	// Collision
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SaveAndDisableCollisionEnable(const EDungeonComponentActivateReason activateReason);
	void LoadCollisionEnable(const EDungeonComponentActivateReason activateReason);

	// Visibility
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SaveAndDisableVisibility(const EDungeonComponentActivateReason activateReason);
	void LoadVisibility(const EDungeonComponentActivateReason activateReason);

	// AI
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason);
	void LoadAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason);

	// LightCastShadow
	void EachControlledLightCastShadow(const std::function<void(UPointLightComponent*)>& function) const;

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
	所属しているDungeonPartitionがプレイヤー周辺に近づいたら呼び出されます。
	The DungeonPartition will be called when it approaches the player's vicinity.
	*/
	UFUNCTION(BlueprintImplementableEvent)
	void OnPartitionActivate();

	/**
	所属しているDungeonPartitionがプレイヤー周辺から離れたら呼び出されます。
	It is called when the DungeonPartition to which it belongs leaves the player's vicinity.
	*/
	UFUNCTION(BlueprintImplementableEvent)
	void OnPartitionInactivate();

private:
	void TickImplement(const FVector& location);
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
	If enabled, controls the validity of the owner actor's Tick
	有効にするとオーナーアクターのTickの有効性を制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool EnableOwnerActorTickControl = true;

	/**
	If enabled, controls the effectiveness of the owner actor's AI
	有効にするとオーナーアクターのAIの有効性を制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool EnableOwnerActorAiControl = true;

	/**
	If enabled, controls the activation of the owner actor's components
	有効にするとオーナーアクターのコンポーネントのアクティブ性を制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool EnableComponentActivationControl = true;

	/**
	If enabled, controls the visibility of the owner actor's components
	有効にするとオーナーアクターのコンポーネントの表示を制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool EnableComponentVisibilityControl = true;

	/**
	If enabled, controls the Cast Shadow of the owner actor's point light and spotlight
	有効にするとオーナーアクターのポイントライトとスポットライトのCast Shadowを制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool EnableLightShadowControl = true;

	/**
	If enabled, controls the enable of the collision component of the owner actor
	有効にするとオーナーアクターのコリジョンコンポーネントの有効性を制御します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
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
	std::vector<TWeakObjectPtr<UPointLightComponent>> mPointLightComponents;

	TWeakObjectPtr<ADungeonMainLevelScriptActor> mDungeonLevelScriptActor;
	TWeakObjectPtr<UDungeonPartition> mLastDungeonPartition;
	FVector mLastLocation = FVector::ZeroVector;

	bool mTickSaver = false;

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

inline bool UDungeonComponentActivatorComponent::IsEnableLightCastShadowControl() const noexcept
{
	return EnableLightShadowControl;
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

inline void UDungeonComponentActivatorComponent::SetEnableLightCastShadowControl(const bool enable) noexcept
{
	EnableLightShadowControl = enable;
}

inline void UDungeonComponentActivatorComponent::SetEnableCollisionEnableControl(const bool enable) noexcept
{
	EnableCollisionEnableControl = enable;
}

inline void UDungeonComponentActivatorComponent::EachControlledLightCastShadow(const std::function<void(UPointLightComponent*)>& function) const
{
	for (const auto& pointLightComponent : mPointLightComponents)
	{
		if (auto* pointer = GetValid(pointLightComponent.Get()))
		{
			function(pointer);
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
