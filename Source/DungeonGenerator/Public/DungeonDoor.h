/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomProps.h"
#include <GameFramework/Actor.h>
#include "DungeonDoor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonDoor : public AActor
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit ADungeonDoor(const FObjectInitializer& initializer);

	/*
	destructor
	*/
	virtual ~ADungeonDoor() = default;

	/*
	Initialize
	*/
	void Initialize(const EDungeonRoomProps props);

	/*
	Get DungeonRoomProps
	*/
	EDungeonRoomProps GetRoomProps() const;

	/*
	Set DungeonRoomProps
	*/
	void SetRoomProps(const EDungeonRoomProps props);

	/**
	Function called during initialization after object creation
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, CallInEditor, Category = "DungeonGenerator")
		void OnInitialize(const EDungeonRoomProps props);
	virtual void OnInitialize_Implementation(const EDungeonRoomProps props);

	/**
	Function called before object destruction
	*/
	void Finalize();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, CallInEditor, Category = "DungeonGenerator")
		void OnFinalize();
	virtual void OnFinalize_Implementation();

	/**
	Reset
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		void Reset();

	/**
	Function called on reset
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DungeonGenerator")
		void OnReset();
	virtual void OnReset_Implementation();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		EDungeonRoomProps Props = EDungeonRoomProps::None;

	enum class State : uint8_t
	{
		Invalid,
		Initialized,
		Finalized,
	};
	State mState = State::Invalid;
};

inline ADungeonDoor::ADungeonDoor(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

inline void ADungeonDoor::OnInitialize_Implementation(const EDungeonRoomProps props)
{
}

inline void ADungeonDoor::OnFinalize_Implementation()
{
}

inline void ADungeonDoor::OnReset_Implementation()
{
}

inline void ADungeonDoor::Reset()
{
	OnReset();
}

inline EDungeonRoomProps ADungeonDoor::GetRoomProps() const
{
	return Props;
}

inline void ADungeonDoor::SetRoomProps(const EDungeonRoomProps props)
{
	Props = props;
}
