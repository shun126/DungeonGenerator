/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "MainLevel/DungeonComponentActivatorComponent.h"
#include "MainLevel/DungeonMainLevelScriptActor.h"
#include "Core/Debug/BuildInformation.h"
#include "Core/Debug/Debug.h"
#include <AIController.h>
#include <BrainComponent.h>
#include <Components/PrimitiveComponent.h>
#include <Engine/Level.h>
#include <Engine/World.h>

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

	if (const auto* ownerActor = GetOwner())
	{
		/*
		 * サブレベル上のアクターがUDungeonComponentActivatorComponentを所有している事があるので
		 * パーシスタントレベルから直接ADungeonMainLevelScriptActorを取得するようにしている
		 */
		if (const auto* world = GetWorld())
		{
			if (IsValid(world->PersistentLevel))
			{
				if (auto* levelScript = Cast<ADungeonMainLevelScriptActor>(world->PersistentLevel->GetLevelScriptActor()))
				{
					mDungeonLevelScriptActor = levelScript;

					// 動かないならTick不要
					const auto* rootSceneComponent = ownerActor->GetRootComponent();
					if (rootSceneComponent && rootSceneComponent->Mobility != EComponentMobility::Movable)
					{
						DUNGEON_GENERATOR_VERBOSE(TEXT("The tick is stopped because the Mobility of actor ‘%s’ is Static."), *ownerActor->GetName());
						SetComponentTickEnabled(false);
					}

					// 初回起動のため少しずらした座標を記録
					mLastLocation = ownerActor->GetActorLocation() + FVector(0, DisplacementOfInitialLocation, 0);



					// 制御対象のポイントライト派生クラスを回収
					for (auto* component : ownerActor->GetComponents())
					{
						// ポイントライト派生クラスか？
						auto* pointLightComponent = Cast<UPointLightComponent>(component);
						if (IsValid(pointLightComponent))
						{
							// 影を落とすライトのみ対象、BeginPlay以降にCastShadowsを変更しても制御対象には含まれない
							if (pointLightComponent->CastShadows)
							{
								// 静的ライト以外なら制御対象として登録
								if (pointLightComponent->Mobility != EComponentMobility::Type::Static)
									mPointLightComponents.emplace_back(pointLightComponent);
							}
						}
					}
					mPointLightComponents.shrink_to_fit();



				}
			}
		}

		// ADungeonMainLevelScriptActorではないならTick不要
		if (mDungeonLevelScriptActor.Get() == nullptr)
		{
			DUNGEON_GENERATOR_VERBOSE(TEXT("Stopping tick because persistent script for actor ‘%s’ not found."), *ownerActor->GetName());
			SetComponentTickEnabled(false);
		}

		TickImplement(ownerActor->GetActorLocation());
	}
}

void UDungeonComponentActivatorComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	Super::EndPlay(endPlayReason);

	mDungeonLevelScriptActor.Reset();

#if WITH_EDITOR
	if (const auto* ownerActor = GetOwner())
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("Component control finished for actor '%s'"), *ownerActor->GetName());
	}
#endif
}

void UDungeonComponentActivatorComponent::TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
	Super::TickComponent(deltaTime, tickType, thisTickFunction);

	if (const auto* ownerActor = GetOwner())
	{
		// 移動した？
		const auto& currentLocation = ownerActor->GetActorLocation();
		if (FVector::DistSquared(mLastLocation, currentLocation) >= MovementDetectionDistance * MovementDetectionDistance)
		{
			TickImplement(currentLocation);
			mLastLocation = currentLocation;
		}
	}

#if JENKINS_FOR_DEVELOP & UE_BUILD_DEBUG & 0
	for (const auto& weakPointLightComponent : mPointLightComponents)
	{
		const auto pointLightComponent = weakPointLightComponent.Get();
		if (IsValid(pointLightComponent))
		{
			if (pointLightComponent->CastShadows)
			{
				DrawDebugString(
					pointLightComponent->GetWorld(),
					pointLightComponent->GetComponentLocation(),
					TEXT("ON"),
					nullptr,
					FColor::White,
					0,
					true,
					1.f
				);
			}
		}
	}
#endif
}

void UDungeonComponentActivatorComponent::TickImplement(const FVector& location)
{
	// ADungeonMainLevelScriptActorではないならTick不要
	if (const auto* levelScript = mDungeonLevelScriptActor.Get())
	{
#if WITH_EDITOR && (UE_BUILD_SHIPPING == 0)
		if (levelScript->IsEnableLoadControl())
		{
#endif
			// 新しい座標のUDungeonPartitionを検索
			auto* currentDungeonPartition = levelScript->Find(location);

			// 古い座標のUDungeonPartitionを検索
			auto* lastDungeonPartition = mLastDungeonPartition.Get();

			// UDungeonPartitionを移動した？
			if (lastDungeonPartition != currentDungeonPartition)
			{
#if WITH_EDITOR
				if (GetOwner())
				{
					DUNGEON_GENERATOR_VERBOSE(TEXT("Actor '%s' has been registered for partitioning"), *GetOwner()->GetName());
				}
#endif

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
#if WITH_EDITOR
	if (GetOwner())
	{
		DUNGEON_GENERATOR_VERBOSE(TEXT("Activate actor '%s"), *GetOwner()->GetName());
	}
#endif

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
	auto* owner = GetOwner();
	if (IsValid(owner))
	{
#if WITH_EDITOR
		DUNGEON_GENERATOR_VERBOSE(TEXT("Inactivate actor '%s"), *owner->GetName());
#endif

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

// Actor
void UDungeonComponentActivatorComponent::SaveAndDisableActorTickEnable(const EDungeonComponentActivateReason activateReason)
{
	if (auto* owner = GetValid(GetOwner()))
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
	auto* owner = GetOwner();
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

				if (auto* primitiveComponent = Cast<UPrimitiveComponent>(component))
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
				if (auto* primitiveComponent = Cast<UPrimitiveComponent>(component))
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

// AI
void UDungeonComponentActivatorComponent::SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason)
{
	if (auto* owner = Cast<APawn>(GetOwner()))
	{
		if (IsValid(owner))
			SaveAndStopAiLogic(activateReason, reason, owner);
	}
}

void UDungeonComponentActivatorComponent::SaveAndStopAiLogic(const EDungeonComponentActivateReason activateReason, const FString& reason, const APawn* owner)
{
	if (mLogicEnabled.all())
	{
		if (auto* controller = owner->GetController<AAIController>())
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
		if (auto* owner = Cast<APawn>(GetOwner()))
		{
			if (auto* controller = owner->GetController<AAIController>())
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
