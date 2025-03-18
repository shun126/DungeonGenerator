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

bool CDungeonRandom::GetBoolean() const
{
	return mRandom->Get<bool>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetIntegerSign() const
{
	return mRandom->GetSign<int32_t>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger() const
{
	return mRandom->Get<int32_t>();
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger(const IntegerType to) const
{
	return mRandom->Get<int32_t>(to);
}

CDungeonRandom::IntegerType CDungeonRandom::GetInteger(const IntegerType from, const IntegerType to) const
{
	return mRandom->Get<int32_t>(from, to);
}

CDungeonRandom::NumberType CDungeonRandom::GetNumberSign() const
{
	return mRandom->GetSign<NumberType>();
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber() const
{
	return mRandom->Get<NumberType>();
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber(const NumberType to) const
{
	return mRandom->Get<NumberType>(to);
}

CDungeonRandom::NumberType CDungeonRandom::GetNumber(const NumberType from, const NumberType to) const
{
	return mRandom->Get<NumberType>(from, to);
}



void UDungeonRandom::SetOwner(const std::shared_ptr<dungeon::Random>& random)
{
	mRandom = random;
}

bool UDungeonRandom::GetBoolean() const
{
	return mRandom->Get<bool>();
}

int32 UDungeonRandom::GetIntegerSign() const
{
	return mRandom->GetSign<int32_t>();
}

int32 UDungeonRandom::GetInteger() const
{
	return mRandom->Get<int32_t>();
}

int32 UDungeonRandom::GetIntegerFrom(const int32 to) const
{
	return mRandom->Get<int32_t>(to);
}

int32 UDungeonRandom::GetIntegerInRangeFrom(const int32 from, const int32 to) const
{
	return mRandom->Get<int32_t>(from, to);
}

float UDungeonRandom::GetNumberSign() const
{
	return mRandom->GetSign<float>();
}

float UDungeonRandom::GetFloat() const
{
	return mRandom->Get<float>();
}

float UDungeonRandom::GetFloatFrom(const float to) const
{
	return mRandom->Get<float>(to);
}

float UDungeonRandom::GetFloatInRangeFrom(const float from, const float to) const
{
	return mRandom->Get<float>(from, to);
}
