/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomItem.h"
#include "DungeonRoomParts.h"
#include "DungeonRoomSensor.generated.h"

// �O���錾
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
	TODO: ��������������̂ŁA�\���̂ł܂Ƃ߂铙�̑Ώ����ĉ�����
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

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, CallInEditor, Category = "DungeonGenerator")
		void OnInitialize();
	virtual void OnInitialize_Implementation();

	/*!
	Finalize
	*/
	void Finalize();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, CallInEditor, Category = "DungeonGenerator")
		void OnFinalize();
	virtual void OnFinalize_Implementation();

	/*!
	Reset
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		void Reset();

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
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 IdealNumberOfActor() const;

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

	/*!
	�X�^�[�g����̐[���̔䗦���v�Z����
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

private:
	 bool FindFloorHeightPosition(FVector& result, const FVector& startPosition, const FVector& endPosition, const float offsetHeight) const;

private:
	// Room Bounding Box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		UBoxComponent* Bounding = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		int32 Identifier = -1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		EDungeonRoomParts Parts = EDungeonRoomParts::Any;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		EDungeonRoomItem Item = EDungeonRoomItem::Empty;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 BranchId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 DepthFromStart = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		uint8 DeepestDepthFromStart = 0;

private:
	enum class State : uint8_t
	{
		Invalid,
		Initialized,
		Finalized,
	};
	State mState = State::Invalid;
};
