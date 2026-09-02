/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include <memory>
#include "DungeonRandom.generated.h"

namespace dungeon
{
	class Random;
}

/**
 * Random numbers for dungeon generation
 * ダンジョン生成用乱数
 */
class DUNGEONGENERATOR_API CDungeonRandom
{
	using IntegerType = int32;
	using NumberType = float;

public:
	CDungeonRandom() = default;
	virtual ~CDungeonRandom() = default;

	explicit CDungeonRandom(const std::shared_ptr<dungeon::Random>& random);
	CDungeonRandom(const CDungeonRandom& other) noexcept;
	CDungeonRandom(CDungeonRandom&& other) noexcept;

	CDungeonRandom& operator=(const CDungeonRandom& other) noexcept;
	CDungeonRandom& operator=(CDungeonRandom&& other) noexcept;

	void SetSeed(const uint32_t seed);

	void SetOwner(const std::shared_ptr<dungeon::Random>& random);
	void ResetOwner();

	/**
	 * @return	true or false
	 * Boolean を返します。
	 */
	bool GetBoolean() const;

	/**
	 * @return	-1 or 1
	 * IntegerSign を返します。
	 */
	IntegerType GetIntegerSign() const;

	/**
	 * Get a random number
	 * @return		Returns the range [type_min,type_max) if T is an integer,
	 * or [0,1] with equal probability if T is a real number.
	 * Integer を返します。
	 */
	IntegerType GetInteger() const;

	/**
	 * Get a random number
	 * @param[in]	to	Upper value
	 * @return		Returns the range [0,to) if T is an integer,
	 * or [0,to] with equal probability if T is a real number.
	 * Integer を返します。
	 */
	IntegerType GetInteger(const IntegerType to) const;

	/**
	 * Get a random number
	 * @param[in]	from	Lower value
	 * @param[in]	to		Upper value
	 * @return		Returns the range [from,to) if T is an integer,
	 * or [from,to] with equal probability if T is a real number.
	 * Integer を返します。
	 */
	IntegerType GetInteger(const IntegerType from, const IntegerType to) const;

	/**
	 * @return	-1 or 1
	 * NumberSign を返します。
	 */
	NumberType GetNumberSign() const;

	/**
	 * Get a random number
	 * @return		Returns the range [type_min,type_max) if T is an integer,
	 * or [0,1] with equal probability if T is a real number.
	 * Number を返します。
	 */
	NumberType GetNumber() const;

	/**
	 * Get a random number
	 * @param[in]	to	Upper value
	 * @return		Returns the range [0,to) if T is an integer,
	 * or [0,to] with equal probability if T is a real number.
	 * Number を返します。
	 */
	NumberType GetNumber(const NumberType to) const;

	/**
	 * Get a random number
	 * @param[in]	from	Lower value
	 * @param[in]	to		Upper value
	 * @return		Returns the range [from,to) if T is an integer,
	 * or [from,to] with equal probability if T is a real number.
	 * Number を返します。
	 */
	NumberType GetNumber(const NumberType from, const NumberType to) const;

private:
	std::shared_ptr<dungeon::Random> mRandom;
};

/**
 * Random numbers for dungeon generation available from BluePrint
 * BluePrintから利用可能なダンジョン生成用乱数
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonRandom : public UObject
{
	GENERATED_BODY()

public:
	UDungeonRandom() = default;
	virtual ~UDungeonRandom() override = default;

	void SetOwner(const std::shared_ptr<dungeon::Random>& random);

	/**
	 * Returns Boolean.
	 * 同期されたランダムな真偽値を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Return a synchronized random Boolean value."))
	bool GetBoolean() const;

	/**
	 * Returns IntegerSign.
	 * -1または1の整数を同じ確率で返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Return either -1 or 1 as an integer with equal probability."))
	int32 GetIntegerSign() const;

	/**
	 * Get a random number
	 * @return		Returns the range [type_min,type_max) if T is an integer,
	 * or [0,1] with equal probability if T is a real number.
	 * Integer を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	int32 GetInteger() const;

	/**
	 * Get a random number
	 * @param[in]	to	Upper value
	 * @return		Returns the range [0,to) if T is an integer,
	 * or [0,to] with equal probability if T is a real number.
	 * IntegerFrom を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	int32 GetIntegerFrom(const int32 to) const;

	/**
	 * Get a random number
	 * @param[in]	from	Lower value
	 * @param[in]	to		Upper value
	 * @return		Returns the range [from,to) if T is an integer,
	 * or [from,to] with equal probability if T is a real number.
	 * IntegerInRangeFrom を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	int32 GetIntegerInRangeFrom(const int32 from, const int32 to) const;

	/**
	 * Returns NumberSign.
	 * -1.0または1.0の浮動小数値を同じ確率で返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Return either -1.0 or 1.0 as a float with equal probability."))
	float GetNumberSign() const;

	/**
	 * Get a random number
	 * @return		Returns the range [type_min,type_max) if T is an integer,
	 * or [0,1] with equal probability if T is a real number.
	 * Float を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	float GetFloat() const;

	/**
	 * Get a random number
	 * @param[in]	to	Upper value
	 * @return		Returns the range [0,to) if T is an integer,
	 * or [0,to] with equal probability if T is a real number.
	 * FloatFrom を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	float GetFloatFrom(const float to) const;

	/**
	 * Get a random number
	 * @param[in]	from	Lower value
	 * @param[in]	to		Upper value
	 * @return		Returns the range [from,to) if T is an integer,
	 * or [from,to] with equal probability if T is a real number.
	 * FloatInRangeFrom を返します。
	 */
	UFUNCTION(BlueprintPure, Category = "DungeonGenerator", meta = (ToolTip = "Get a random number"))
	float GetFloatInRangeFrom(const float from, const float to) const;

private:
	std::shared_ptr<dungeon::Random> mRandom;
};
