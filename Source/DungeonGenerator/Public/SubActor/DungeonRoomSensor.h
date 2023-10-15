/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Mission/DungeonRoomItem.h"
#include "Mission/DungeonRoomParts.h"
#include <GameFramework/Actor.h>
#include <memory>
#include "DungeonRoomSensor.generated.h"

// forward declaration
class UBoxComponent;

namespace dungeon
{
	class Random;
}

/*
Room Bounding Box Sensor Actor Class
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonRoomSensor : public AActor
{
	GENERATED_BODY()

public:
	/*
	constructor
	*/
	explicit ADungeonRoomSensor(const FObjectInitializer& initializer);

	/*
	destructor
	*/
	virtual ~ADungeonRoomSensor() = default;

	/*
	Initialize
	TODO: Too many arguments, so please deal with them by putting them together in a structure, etc.
	*/
	void Initialize(
		const std::shared_ptr<dungeon::Random>& random,
		const int32 identifier,
		const FVector& extents,
		const EDungeonRoomParts parts,
		const EDungeonRoomItem item,
		const uint8 branchId,
		const uint8 depthFromStart,
		const uint8 deepestDepthFromStart
	);

	/**
	Function called during initialization after object creation
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, CallInEditor, Category = "DungeonGenerator")
		void OnInitialize(const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 depthFromStart, const float depthFromStartRatio);
	virtual void OnInitialize_Implementation(const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 depthFromStart, const float depthFromStartRatio);

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

	/*
	Get Bounding
	*/
	UBoxComponent* GetBounding();
	
	/*
	Get Bounding
	*/
	const UBoxComponent* GetBounding() const;

	/*
	Get the ideal number of people in the bounding
	\param[in]		areaRequiredPerPerson (default : 500cm)
	\param[in]		maxNumberOfActor (default : 5 actors)
	\return			ideal number of people in the bounding
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 IdealNumberOfActor(const float areaRequiredPerPerson = 500.f, const int32 maxNumberOfActor = 5) const;

	/*
	Get a random position in the bounding
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		bool RandomPoint(FVector& result, const float offsetHeight = 0.f) const;

	/*
	Get random transforms in bounding
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		bool RandomTransform(FTransform& result, const float offsetHeight = 0.f) const;

	/*
	Get floor height position
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		bool GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight = 0.f) const;

	/**
	Calculate the depth ratio from the start
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
		float GetDepthRatioFromStart() const;

	/*
	Gets the tag of the interior of the room to be generated.
	For example, if the creator returns the tag `kitchen`, the interior with the kitchen tag will be selected.
	If multiple tags are set, actors containing all tags will be selected as spawn candidates.
	*/
	UFUNCTION(BlueprintImplementableEvent)
		TArray<FString> GetInquireInteriorTags() const;

	/*
	Get the DungeonGenerator tag name.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
		static const FName& GetDungeonGeneratorTag();

	/*
	Get the DungeonGenerator tag name.
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "DungeonGenerator")
		static const TArray<FName>& GetDungeonGeneratorTags();

	// overrides
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void Tick(float DeltaSeconds) override;
#endif

private:
	 bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;

private:
	// Room Bounding Box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		UBoxComponent* Bounding;
	// TObjectPtr not used for UE4 compatibility

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		int32 Identifier = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 BranchId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 DepthFromStart = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 DeepestDepthFromStart = 0;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		bool ShowDebugInfomation = false;
#endif

private:
	enum class State : uint8_t
	{
		Invalid,
		Initialized,
		Finalized,
	};
	State mState = State::Invalid;

	std::shared_ptr<dungeon::Random> mRandom;

	friend class CDungeonGeneratorCore;
};

inline void ADungeonRoomSensor::OnInitialize_Implementation(const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 depthFromStart, const float depthFromStartRatio)
{
}

inline void ADungeonRoomSensor::OnFinalize_Implementation()
{
}

inline void ADungeonRoomSensor::OnReset_Implementation()
{
}

inline void ADungeonRoomSensor::Reset()
{
	OnReset();
}

inline UBoxComponent* ADungeonRoomSensor::GetBounding()
{
	return Bounding;
}

inline const UBoxComponent* ADungeonRoomSensor::GetBounding() const
{
	return Bounding;
}
