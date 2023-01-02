/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomSensor.generated.h"

class UBoxComponent;

/*
Room Bounding Box Sensor Actor Class
*/
UCLASS(BlueprintType, Blueprintable)
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
	*/
	void Initialize(const int32 identifier, const FVector& extents, const EDungeonRoomParts parts, const EDungeonRoomItem item, const uint8 branchId);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
		void OnInitialize();
	virtual void OnInitialize_Implementation();

	UFUNCTION(BlueprintCallable)
		void Reset();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
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
	*/
	UFUNCTION(BlueprintCallable)
		int32 IdealNumberOfActor() const;

	/*
	Get a random position in the bounding
	*/
	UFUNCTION(BlueprintCallable)
		bool RandomPoint(FVector& result, const float offsetHeight = 0.f) const;

	/*
	Get random transforms in bounding
	*/
	UFUNCTION(BlueprintCallable)
		bool RandomTransform(FTransform& result, const float offsetHeight = 0.f) const;

	/*
	Get floor height position
	*/
	UFUNCTION(BlueprintCallable)
		bool GetFloorHeightPosition(FVector& result, FVector startPosition, const float offsetHeight = 0.f) const;

private:
	 bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;

private:
	// Room Bounding Box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
		UBoxComponent* Bounding = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		int32 Identifier = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		uint8 BranchId = 0;
};
