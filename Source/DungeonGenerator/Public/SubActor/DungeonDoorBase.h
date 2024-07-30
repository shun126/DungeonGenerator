/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonActorBase.h"
#include "Helper/DungeonRandom.h"
#include "Mission/DungeonRoomProps.h"
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "DungeonDoorBase.generated.h"

// forward declaration

namespace dungeon
{
	class Random;
}

/*
Dungeon Door Actor
DungeonDoorBase is an actor intended to be replicated.
Please be very careful with server-client synchronization.

ダンジョンドアアクター
DungeonDoorBaseはレプリケーションする前提のアクターです。
サーバーとクライアントの同期に十分注意して下さい。
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonDoorBase : public ADungeonActorBase
{
	GENERATED_BODY()

public:
	explicit ADungeonDoorBase(const FObjectInitializer& initializer);
	virtual ~ADungeonDoorBase() = default;

	/*
	Get DungeonRoomProps
	DungeonRoomPropsを取得します
	*/
	EDungeonRoomProps GetRoomProps() const;

	/*
	Set DungeonRoomProps
	DungeonRoomPropsを設定します
	*/
	void SetRoomProps(const EDungeonRoomProps props);

	/*
	Is locked door?
	鍵付きドアか？
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
	bool IsLockedDoor() const;

	/**
	Function called during initialization after object creation
	オブジェクト生成後に呼び出される初期化用関数
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnInitialize(const EDungeonRoomProps props);

	/*
	Finalize function called before object destruction
	オブジェクト破棄前に呼び出される終了用関数
	*/
	UFUNCTION(BlueprintImplementableEvent, Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void OnFinalize(const bool finish);

	// overrides
	virtual uint32_t GenerateCrc32(uint32_t crc = 0xffffffffU) const noexcept override;

protected:
	/*
	Functions for initialization after object creation
	オブジェクト生成後の初期化用関数
	*/
	virtual void OnNativeInitialize(const EDungeonRoomProps props);

	/**
	Function called before object destruction
	オブジェクト破棄前の終了用関数
	*/
	virtual void OnNativeFinalize();

	/*
	Get random numbers common to dungeon generation systems
	ダンジョン生成システム共通の乱数を取得します
	*/
	CDungeonRandom& GetRandom() noexcept;

private:
	void InvokeInitialize(const std::shared_ptr<dungeon::Random>& random, const EDungeonRoomProps props);
	void InvokeFinalize(const bool finish);

private:
	/*
	Types of props attached to the door
	ドアに付属する小道具の種類
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
	EDungeonRoomProps Props = EDungeonRoomProps::None;

private:
	CDungeonRandom mRandom;

	enum class State : uint8_t
	{
		Invalid,
		Initialized,
		Finalized,
	};
	State mState = State::Invalid;

	friend class CDungeonGeneratorCore;
};

inline EDungeonRoomProps ADungeonDoorBase::GetRoomProps() const
{
	return Props;
}

inline void ADungeonDoorBase::SetRoomProps(const EDungeonRoomProps props)
{
	Props = props;
}

inline bool ADungeonDoorBase::IsLockedDoor() const
{
	return Props != EDungeonRoomProps::None;
}

inline void ADungeonDoorBase::OnNativeInitialize(const EDungeonRoomProps props)
{
}

inline void ADungeonDoorBase::OnNativeFinalize()
{
}

inline CDungeonRandom& ADungeonDoorBase::GetRandom() noexcept
{
	return mRandom;
}
