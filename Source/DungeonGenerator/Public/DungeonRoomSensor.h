/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomSensor.generated.h"

// forward declaration
class UBoxComponent;

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
		UBoxComponent* Bounding = nullptr;

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
