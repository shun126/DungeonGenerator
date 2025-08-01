/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonComponentActivatorComponent.h"

#include "Components/SpotLightComponent.h"

#include "MainLevel/DungeonMainLevelScriptActor.h"
#include <Engine/Level.h>
#include <AIController.h>
#include <BrainComponent.h>
#include <Components/PrimitiveComponent.h>

static constexpr double DisplacementOfInitialLocation = 100;
static constexpr double MovementDetectionDistance = 1;

UDungeonComponentActivatorComponent::UDungeonComponentActivatorComponent(const FObjectInitializer& objectInitializer)
	: Super(objectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDungeonComponentActivatorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* ownerActor = GetOwner())
	{
		if (const ULevel* level = GetComponentLevel())
		{
			if (ADungeonMainLevelScriptActor* levelScript = Cast<ADungeonMainLevelScriptActor>(level->GetLevelScriptActor()))
			{
				mDungeonLevelScriptActor = levelScript;

				// 動かないならTick不要
				const USceneComponent* rootSceneComponent = ownerActor->GetRootComponent();
				if (rootSceneComponent && rootSceneComponent->Mobility != EComponentMobility::Movable)
					SetComponentTickEnabled(false);

				// 初回起動のため少しずらした座標を記録
				mLastLocation = ownerActor->GetActorLocation() + FVector(0, DisplacementOfInitialLocation, 0);
			}
		}
		// ADungeonMainLevelScriptActorではないならTick不要
		if (mDungeonLevelScriptActor.Get() == nullptr)
		{
			SetComponentTickEnabled(false);
		}

		TickImplement(ownerActor->GetActorLocation());
	}
}

void UDungeonComponentActivatorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	mDungeonLevelScriptActor.Reset();
}

void UDungeonComponentActivatorComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (const AActor* ownerActor = GetOwner())
	{
		// 移動した？
		const FVector& currentLocation = ownerActor->GetActorLocation();
		if (FVector::DistSquared(mLastLocation, currentLocation) >= MovementDetectionDistance * MovementDetectionDistance)
		{
			TickImplement(currentLocation);
			mLastLocation = currentLocation;
		}
	}
}

void UDungeonComponentActivatorComponent::TickImplement(const FVector& location)
{
	// ADungeonMainLevelScriptActorではないならTick不要
	if (const ADungeonMainLevelScriptActor* levelScript = mDungeonLevelScriptActor.Get())
	{
#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
		if (levelScript->IsEnableLoadControl())
		{
#endif
			// 新しい座標のUDungeonPartitionを検索
			UDungeonPartition* currentDungeonPartition = levelScript->Find(location);

			// 古い座標のUDungeonPartitionを検索
			UDungeonPartition* lastDungeonPartition = mLastDungeonPartition.Get();

			// UDungeonPartitionを移動した？
			if (lastDungeonPartition != currentDungeonPartition)
			{
				// 古いUDungeonPartitionから退出
				if (IsValid(lastDungeonPartition))
				{
					lastDungeonPartition->UnregisterActivatorComponent(this);
				}

				// 新しいUDungeonPartitionに登録
				if (IsValid(currentDungeonPartition))
				{
					currentDungeonPartition->RegisterActivatorComponent(this);
				}

				mLastDungeonPartition = currentDungeonPartition;
			}
#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
		}
#endif
	}
}

void UDungeonComponentActivatorComponent::CallPartitionActivate()
{
	OnPartitionActivate();

	if (EnableComponentActivationControl)
		LoadComponentActivation(EDungeonComponentActivateReason::Partition);

	if (EnableCollisionEnableControl)
		LoadCollisionEnable(EDungeonComponentActivateReason::Partition);

	if (EnableComponentVisibilityControl)
		LoadVisibility(EDungeonComponentActivateReason::Partition);

	if (EnableOwnerActorTickControl)
		LoadActorTickEnable(EDungeonComponentActivateReason::Partition);

	if (EnableOwnerActorAiControl)
		LoadAiLogic(EDungeonComponentActivateReason::Partition, TEXT("DungeonActivatorComponent"));
}

void UDungeonComponentActivatorComponent::CallPartitionInactivate()
{
	AActor* owner = GetOwner();
	if (IsValid(owner))
	{
		if (EnableComponentActivationControl)
			SaveAndDisableComponentActivation(EDungeonComponentActivateReason::Partition, owner);

		if (EnableCollisionEnableControl)
			SaveAndDisableCollisionEnable(EDungeonComponentActivateReason::Partition, owner);

		if (EnableComponentVisibilityControl)
			SaveAndDisableVisibility(EDungeonComponentActivateReason::Partition, owner);

		if (EnableOwnerActorTickControl)
			SaveAndDisableActorTickEnable(EDungeonComponentActivateReason::Partition, owner);

		if (EnableOwnerActorAiControl)
		{
			if (APawn* ownerPawn = Cast<APawn>(GetOwner()))
				SaveAndStopAiLogic(EDungeonComponentActivateReason::Partition, TEXT("DungeonActivatorComponent"), ownerPawn);
		}
	}

	OnPartitionInactivate();
}

void UDungeonComponentActivatorComponent::CallCastShadowActivate()
{
	if (EnableLightShadowControl)
		LoadCastShadow(EDungeonComponentActivateReason::Partition);
}

void UDungeonComponentActivatorComponent::CallCastShadowInactivate()
{
	if (const AActor* owner = GetValid(GetOwner()))
	{
		if (EnableLightShadowControl)
			SaveAndDisableCastShadow(EDungeonComponentActivateReason::Partition, owner);
	}
}

// Actor
void UDungeonComponentActivatorComponent::SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason)
{
	if (AActor* owner = GetValid(GetOwner()))
		SaveAndDisableActorTickEnable(activateReason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason, AActor* owner)
{
	const bool previousEnabled = mIsTickEnabled.all();
	mIsTickEnabled.reset(static_cast<size_t>(activateReason));
	const bool currentEnabled = mIsTickEnabled.all();
	if (previousEnabled != currentEnabled)
	{
		mTickSaver = owner->IsActorTickEnabled();
		owner->SetActorTickEnabled(false);
	}
}

void UDungeonComponentActivatorComponent::LoadActorTickEnable(const EDungeonComponentActivateReason activateReason)
{
	const bool previousEnabled = mIsTickEnabled.all();
	mIsTickEnabled.set(static_cast<size_t>(activateReason));
	const bool currentEnabled = mIsTickEnabled.all();
	if (previousEnabled != currentEnabled)
	{
		if (AActor* owner = GetOwner())
		{
			owner->SetActorTickEnabled(mTickSaver);
		}
	}
}

// Component
void UDungeonComponentActivatorComponent::SaveAndDisableComponentActivation(const EDungeonComponentActivateReason activateReason)
{
	AActor* owner = GetOwner();
	if (IsValid(owner))
		SaveAndDisableComponentActivation(activateReason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndDisableComponentActivation(const EDungeonComponentActivateReason activateReason, const AActor* owner)
{
	const bool previousEnabled = mComponentActivation.all();
	mComponentActivation.reset(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentActivation.all();
	if (previousEnabled != currentEnabled)
	{
		// Deactivate after preserving component activation
		mComponentActivationSaver.Stash(owner, [](UActorComponent* component)
			{
				std::pair<bool, bool> result;
				result.second = component->IsActive();
				result.first = result.second
					// && Cast<UDungeonComponentActivatorComponent>(component) == nullptr
					// && Cast<UGameplayTasksComponent>(component) == nullptr
					;
				if (result.first)
				{
					component->Deactivate();
				}

				return result;
			}
		);
	}
}

void UDungeonComponentActivatorComponent::LoadComponentActivation(const EDungeonComponentActivateReason activateReason)
{
	const bool previousEnabled = mComponentActivation.all();
	mComponentActivation.set(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentActivation.all();
	if (previousEnabled != currentEnabled)
	{
		// Restore component activation
		mComponentActivationSaver.Pop([](UActorComponent* component, const bool activation)
			{
				component->SetActive(activation);
			}
		);
	}
}

// Collision
void UDungeonComponentActivatorComponent::SaveAndDisableCollisionEnable(const EDungeonComponentActivateReason activateReason)
{
	AActor* owner = GetOwner();
	if (IsValid(owner))
		SaveAndDisableCollisionEnable(activateReason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndDisableCollisionEnable(const EDungeonComponentActivateReason activateReason, const AActor* owner)
{
	const bool previousEnabled = mComponentCollisionEnabled.all();
	mComponentCollisionEnabled.reset(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentCollisionEnabled.all();
	if (previousEnabled != currentEnabled)
	{
		mComponentCollisionEnabledSaver.Stash(owner, [](UActorComponent* component)
			{
				std::pair<bool, ECollisionEnabled::Type> result;

				if (UPrimitiveComponent* primitiveComponent = Cast<UPrimitiveComponent>(component))
				{
					result.first = true;
					result.second = primitiveComponent->GetCollisionEnabled();
					primitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
				else
				{
					result.first = false;
				}

				return result;
			}
		);
	}
}

void UDungeonComponentActivatorComponent::LoadCollisionEnable(const EDungeonComponentActivateReason activateReason)
{
	const bool previousEnabled = mComponentCollisionEnabled.all();
	mComponentCollisionEnabled.set(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentCollisionEnabled.all();
	if (previousEnabled != currentEnabled)
	{
		mComponentCollisionEnabledSaver.Pop([](UActorComponent* component, const ECollisionEnabled::Type activation)
			{
				if (UPrimitiveComponent* primitiveComponent = Cast<UPrimitiveComponent>(component))
				{
					primitiveComponent->SetCollisionEnabled(activation);
				}
			}
		);
	}
}

// Visibility
void UDungeonComponentActivatorComponent::SaveAndDisableVisibility(const EDungeonComponentActivateReason activateReason)
{
	AActor* owner = GetOwner();
	if (IsValid(owner))
		SaveAndDisableVisibility(activateReason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndDisableVisibility(const EDungeonComponentActivateReason activateReason, const AActor* owner)
{
	const bool previousEnabled = mComponentVisibility.all();
	mComponentVisibility.reset(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentVisibility.all();
	if (previousEnabled != currentEnabled)
	{
		mComponentVisibilitySaver.Stash(owner, [](UActorComponent* component)
			{
				std::pair<bool, bool> result;

				auto* sceneComponent = Cast<USceneComponent>(component);
				result.first = result.second = IsValid(sceneComponent) && sceneComponent->IsVisible();
				if (result.first)
				{
					sceneComponent->SetVisibility(false);
				}
				else
				{
					result.first = false;
				}

				return result;
			}
		);
	}
}

void UDungeonComponentActivatorComponent::LoadVisibility(const EDungeonComponentActivateReason activateReason)
{
	const bool previousEnabled = mComponentVisibility.all();
	mComponentVisibility.set(static_cast<size_t>(activateReason));
	const bool currentEnabled = mComponentVisibility.all();
	if (previousEnabled != currentEnabled)
	{
		mComponentVisibilitySaver.Pop([](UActorComponent* component, const bool activation)
			{
				if (auto* sceneComponent = Cast<USceneComponent>(component))
				{
					sceneComponent->SetVisibility(activation);
				}
			}
		);
	}
}

// CastShadow
void UDungeonComponentActivatorComponent::SaveAndDisableCastShadow(const EDungeonComponentActivateReason activateReason)
{
	AActor* owner = GetOwner();
	if (IsValid(owner))
		SaveAndDisableCastShadow(activateReason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndDisableCastShadow(const EDungeonComponentActivateReason activateReason, const AActor* owner)
{
	const bool previousEnabled = mLightCastShadow.all();
	mLightCastShadow.reset(static_cast<size_t>(activateReason));
	const bool currentEnabled = mLightCastShadow.all();
	if (previousEnabled != currentEnabled)
	{
		mLightCastShadowSaver.Stash(owner, [](UActorComponent* component)
			{
				std::pair<bool, bool> result;

				auto* pointLightComponent = Cast<UPointLightComponent>(component);
				result.first = result.second =
					IsValid(pointLightComponent) &&
					pointLightComponent->Mobility != EComponentMobility::Type::Static &&
					pointLightComponent->CastShadows != 0;
				if (result.first)
				{
					pointLightComponent->SetCastShadows(false);
				}
				else
				{
					result.first = false;
				}

				return result;
			}
		);
	}
}

void UDungeonComponentActivatorComponent::LoadCastShadow(const EDungeonComponentActivateReason activateReason)
{
	const bool previousEnabled = mLightCastShadow.all();
	mLightCastShadow.set(static_cast<size_t>(activateReason));
	const bool currentEnabled = mLightCastShadow.all();
	if (previousEnabled != currentEnabled)
	{
		mLightCastShadowSaver.Pop([](UActorComponent* component, const bool castShadow)
			{
				if (auto* pointLightComponent = Cast<UPointLightComponent>(component))
				{
					pointLightComponent->SetCastShadows(castShadow);
				}
			}
		);
	}
}

// AI
void UDungeonComponentActivatorComponent::SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason)
{
	if (APawn* owner = Cast<APawn>(GetOwner()))
		if (IsValid(owner))
			SaveAndStopAiLogic(activateReason, reason, owner);
}

void UDungeonComponentActivatorComponent::SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason, const APawn* owner)
{
	if (mLogicEnabled.all())
	{
		if (AAIController* controller = owner->GetController<AAIController>())
		{
			UBrainComponent* brain = controller->BrainComponent;
			if (IsValid(brain) == true && brain->IsPaused() == false)
			{
				brain->PauseLogic(reason);

				controller->StopMovement();
				controller->ClearFocus(EAIFocusPriority::Move);
			}
		}
	}

	mLogicEnabled.reset(static_cast<size_t>(activateReason));
}

void UDungeonComponentActivatorComponent::LoadAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason)
{
	if (mLogicEnabled.set(static_cast<size_t>(activateReason)).all())
	{
		if (APawn* owner = Cast<APawn>(GetOwner()))
		{
			if (AAIController* controller = owner->GetController<AAIController>())
			{
				UBrainComponent* brain = controller->BrainComponent;
				if (IsValid(brain) && brain->IsPaused())
				{
					brain->ResumeLogic(reason);
				}
			}
		}
	}
}
