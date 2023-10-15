/**
パーリンノイズに関するソースファイル

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "PerlinNoise.h"
#include "Random.h"
#include <cmath>

namespace dungeon
{
	PerlinNoise::PerlinNoise(const std::shared_ptr<Random>& random)
	{
		SetSeed(random);
	}

	void PerlinNoise::SetSeed(const std::shared_ptr<Random>& random)
	{
		for (std::size_t i = 0; i < 256; ++i)
		{
			mHash[i] = static_cast<std::uint8_t>(i);
		}
		for (std::size_t i = 256 - 2; i >= 1; --i)
		{
			const std::size_t j = random->Get(256);
			const uint8_t t = mHash[i];
			mHash[i] = mHash[j];
			mHash[j] = t;
		}
		for (std::size_t i = 0; i < 256; ++i)
		{
			mHash[256 + i] = mHash[i];
		}
	}

	constexpr float PerlinNoise::GetFade(const float t) const noexcept
	{
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	constexpr float PerlinNoise::GetLerp(const float t, const float a, const float b) const noexcept
	{
		return a + t * (b - a);
	}

	constexpr float PerlinNoise::MakeGrad(const std::uint8_t hash, const float x, const float y, const float z) const noexcept
	{
		// Source: http://riven8192.blogspot.com/2010/08/calculate-perlinnoise-twice-as-fast.html
		if (z == 0.f)
		{
			switch (hash & 0x3)
			{
			case 0x0: return  x + y;
			case 0x1: return -x + y;
			case 0x2: return  x - y;
			case 0x3: return -x - y;
			default: return 0; // never happens
			}
		}
		else
		{
			switch (hash & 0xF)
			{
			case 0x0: return  x + y;
			case 0x1: return -x + y;
			case 0x2: return  x - y;
			case 0x3: return -x - y;
			case 0x4: return  x + z;
			case 0x5: return -x + z;
			case 0x6: return  x - z;
			case 0x7: return -x - z;
			case 0x8: return  y + z;
			case 0x9: return -y + z;
			case 0xA: return  y - z;
			case 0xB: return -y - z;
			case 0xC: return  y + x;
			case 0xD: return -y + z;
			case 0xE: return  y - x;
			case 0xF: return -y - z;
			default: return 0; // never happens
			}
		}
	}

	constexpr float PerlinNoise::GetGrad(const std::uint8_t hash, const float x, const float y, const float z) const noexcept
	{
		return MakeGrad(hash, x, y, z);
	}
	
	float PerlinNoise::SetNoise(float x, float y, float z) const noexcept
	{
		const std::size_t xInt = static_cast<std::size_t>(std::floor(x)) & 255;
		const std::size_t yInt = static_cast<std::size_t>(std::floor(y)) & 255;
		const std::size_t zInt = static_cast<std::size_t>(std::floor(z)) & 255;
		x -= std::floor(x);
		y -= std::floor(y);
		z -= std::floor(z);
		const float u = GetFade(x);
		const float v = GetFade(y);
		const float w = GetFade(z);
		const std::size_t a0 = mHash[xInt] + yInt;
		const std::size_t a1 = mHash[a0] + zInt;
		const std::size_t a2 = mHash[a0 + 1] + zInt;
		const std::size_t b0 = mHash[xInt + 1] + yInt;
		const std::size_t b1 = mHash[b0] + zInt;
		const std::size_t b2 = mHash[b0 + 1] + zInt;

		return GetLerp(w,
			GetLerp(v,
				GetLerp(u, GetGrad(mHash[a1], x, y, z), GetGrad(mHash[b1], x - 1, y, z)),
				GetLerp(u, GetGrad(mHash[a2], x, y - 1, z), GetGrad(mHash[b2], x - 1, y - 1, z))),
			GetLerp(v,
				GetLerp(u, GetGrad(mHash[a1 + 1], x, y, z - 1), GetGrad(mHash[b1 + 1], x - 1, y, z - 1)),
				GetLerp(u, GetGrad(mHash[a2 + 1], x, y - 1, z - 1), GetGrad(mHash[b2 + 1], x - 1, y - 1, z - 1)))
		);
	}

	float PerlinNoise::SetOctaveNoise(const std::size_t octaves, float x, float y) const noexcept
	{
		float noiseValue = 0.f;
		float amp = 1.f;
		for (std::size_t i = 0; i < octaves; ++i)
		{
			noiseValue += SetNoise(x, y) * amp;
			x *= 2.f;
			y *= 2.f;
			amp *= 0.5f;
		}
		return noiseValue;
	}

	float PerlinNoise::SetOctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept
	{
		float noiseValue = 0.f;
		float amp = 1.f;
		for (std::size_t i = 0; i < octaves; ++i)
		{
			noiseValue += SetNoise(x, y, z) * amp;
			x *= 2.f;
			y *= 2.f;
			z *= 2.f;
			amp *= 0.5f;
		}
		return noiseValue;
	}
}
