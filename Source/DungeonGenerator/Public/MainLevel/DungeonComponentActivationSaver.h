/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <functional>
#include <utility>

/**
A class that assists in saving and restoring the active status of a component owned by an actor
アクターが所有するコンポーネントのアクティブ状態の保存と復元を支援するクラス。
*/
template<typename T>
class DungeonComponentActivationSaver final
{
public:
	DungeonComponentActivationSaver();
	~DungeonComponentActivationSaver() = default;

	/**
	Records the activity of components owned by the actor
	@param[in]	actor
	@param[in]	function
	*/
	void Stash(const AActor* actor, const std::function<std::pair<bool, T>(UActorComponent*)>& function);

	/**
	Restore the activity of recorded components
	@param[in]	function
	*/
	void Pop(const std::function<void(UActorComponent*, const T)>& function);

	/**
	No records?
	@return		Returns true if nothing is recorded.
	*/
	bool IsEmpty() const;

private:
	TMap<TWeakObjectPtr<UActorComponent>, T> mActivations;
};

template<typename T>
inline DungeonComponentActivationSaver<T>::DungeonComponentActivationSaver()
{
}

template<typename T>
inline void DungeonComponentActivationSaver<T>::Stash(const AActor* actor, const std::function<std::pair<bool, T>(UActorComponent*)>& function)
{
	mActivations.Reset();

	if (IsValid(actor))
	{
		/*
		Note: GetComponents can also retrieve components below the child level,
		so there is no need to recurse.
		*/
		for (UActorComponent* component : actor->GetComponents())
		{
			if (!IsValid(component))
				continue;

			const std::pair<bool, T> result = function(component);
			if (result.first)
			{
				mActivations.Add(component, result.second);
			}
		}
	}
}

template<typename T>
inline void DungeonComponentActivationSaver<T>::Pop(const std::function<void(UActorComponent*, const T)>& function)
{
	for (const auto& activation : mActivations)
	{
		UActorComponent* component = activation.Key.Get();
		if (!IsValid(component))
			continue;

		function(component, activation.Value);
	}

	mActivations.Reset();
}

template<typename T>
inline bool DungeonComponentActivationSaver<T>::IsEmpty() const
{
	return mActivations.Num() <= 0;
}
