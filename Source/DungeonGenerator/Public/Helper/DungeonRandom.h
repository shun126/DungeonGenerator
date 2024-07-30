/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <memory>

namespace dungeon
{
	class Random;
}

/**
共通ランダムクラス
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
	@return	true or false
	*/
	bool GetBoolean();

	/**
	@return	-1 or 1
	*/
	IntegerType GetIntegerSign();

	/**
	Get a random number
	@return		Returns the range [type_min,type_max) if T is an integer,
				or [0,1] with equal probability if T is a real number.
	*/
	IntegerType GetInteger();

	/**
	Get a random number
	@param[in]	to	Upper value
	@return		Returns the range [0,to) if T is an integer,
				or [0,to] with equal probability if T is a real number.
	*/
	IntegerType GetInteger(const IntegerType to);

	/**
	Get a random number
	@param[in]	from	Lower value
	@param[in]	to		Upper value
	@return		Returns the range [from,to) if T is an integer,
				or [from,to] with equal probability if T is a real number.
	*/
	IntegerType GetInteger(const IntegerType from, const IntegerType to);

	/**
	@return	-1 or 1
	*/
	NumberType GetNumberSign();

	/**
	Get a random number
	@return		Returns the range [type_min,type_max) if T is an integer,
				or [0,1] with equal probability if T is a real number.
	*/
	NumberType GetNumber();

	/**
	Get a random number
	@param[in]	to	Upper value
	@return		Returns the range [0,to) if T is an integer,
				or [0,to] with equal probability if T is a real number.
	*/
	NumberType GetNumber(const NumberType to);

	/**
	Get a random number
	@param[in]	from	Lower value
	@param[in]	to		Upper value
	@return		Returns the range [from,to) if T is an integer,
				or [from,to] with equal probability if T is a real number.
	*/
	NumberType GetNumber(const NumberType from, const NumberType to);

private:
	std::shared_ptr<dungeon::Random> mRandom;
};
