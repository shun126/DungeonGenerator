/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include "DungeonVerifiableActor.generated.h"

/**
Verifiable Actor Class
検証可能なアクタークラス
*/
UCLASS(Abstract, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonVerifiableActor : public AActor
{
	GENERATED_BODY()

protected:
	/**
	constructor
	コンストラクタ
	*/
	explicit ADungeonVerifiableActor(const FObjectInitializer& initializer);

	/**
	destructor
	デストラクタ
	*/
	virtual ~ADungeonVerifiableActor() override = default;

public:
	/**
	Calculate CRC32
	CRC32を計算します
	*/
	virtual uint32_t GenerateCrc32(uint32_t crc = 0xffffffffU) const noexcept;

public:
	/**
	Calculates the CRC32 of the actor
	アクターのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const AActor* actor, uint32_t crc = 0xffffffffU) noexcept;

	/**
	Calculates the CRC32 of the FBox
	FBoxのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const FBox& box, uint32_t crc = 0xffffffffU) noexcept;

	/**
	Calculate CRC32 of FRotator
	FRotatorのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const FRotator& rotator, uint32_t crc = 0xffffffffU) noexcept;

	/**
	Calculate CRC32 of FQuat
	FQuatのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const FQuat& rotator, uint32_t crc = 0xffffffffU) noexcept;

	/**
	Calculate CRC32 of FTransform
	FTransformのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const FTransform& transform, uint32_t crc = 0xffffffffU) noexcept;

	/**
	Calculate CRC32 of FVector
	FVectorのCRC32を計算します
	*/
	static uint32_t GenerateCrc32(const FVector& vector, uint32_t crc = 0xffffffffU) noexcept;
};
