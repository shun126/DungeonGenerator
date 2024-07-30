/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

/*
共通ランダムクラス
*/

#include "Helper/DungeonRandom.h"
#include "Core/Math/Random.h"

CDungeonRandom::CDungeonRandom(const std::shared_ptr<dungeon::Random>& random)
	: mRandom(random)
{
}

CDungeonRandom::CDungeonRandom(const CDungeonRandom& other) noexcept
	: mRandom(other.mRandom)
{
}

CDungeonRandom::CDungeonRandom(CDungeonRandom&& other) noexcept
	: mRandom(std::move(other.mRandom))
{
	other.mRandom.reset();
}

CDungeonRandom& CDungeonRandom::operator=(const CDungeonRandom& other) noexcept
{
	mRandom = other.mRandom;
	return *this;
}

CDungeonRandom& CDungeonRandom::operator=(CDungeonRandom&& other) noexcept
{
	mRandom = std::move(other.mRandom);
	other.mRandom.reset();
	return* this;
}

void CDungeonRandom::SetSeed(const uint32_t seed)
{
	mRandom->SetSeed(seed);
}

void CDungeonRandom::SetOwner(const std::shared_ptr<dungeon::Random>& random)
{
	mRandom = random;
}

void CDungeonRandom::ResetOwner()
{
	mRandom.reset();
}

bool CDungeonRandom::GetBoolean()
{
	return mRandom->Get<bool>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetIntegerSign()
{
	return mRandom->GetSign<int32_t>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger()
{
	return mRandom->Get<int32_t>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger(const IntegerType to)
{
	return mRandom->Get<int32_t>(to);
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger(const IntegerType from, const IntegerType to)
{
	return mRandom->Get<int32_t>(from, to);
}

CDungeonRandom::NumberType CDungeonRandom::GetNumberSign()
{
	return mRandom->GetSign<NumberType>();
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber()
{
	return mRandom->Get<NumberType>();
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber(const NumberType to)
{
	return mRandom->Get<NumberType>(to);
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber(const NumberType from, const NumberType to)
{
	return mRandom->Get<NumberType>(from, to);
}
