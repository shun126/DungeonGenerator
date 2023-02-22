/**
パーリンノイズに関するヘッダーファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <array>
#include <cstdint>

namespace dungeon
{
	class Random;

	/*!
	パーリンノイズクラス
	*/
	class PerlinNoise final
	{
	public:
		explicit PerlinNoise(Random& random);
		~PerlinNoise() = default;

		// SEED値を設定する
		void SetSeed(Random& random);

		// オクターブ無しノイズを取得する
		float Noise(float x) const noexcept;
		float Noise(float x, float y) const noexcept;
		float Noise(float x, float y, float z) const noexcept;

		// オクターブ有りノイズを取得する
		float OctaveNoise(const std::size_t octaves, float x) const noexcept;
		float OctaveNoise(const std::size_t octaves, float x, float y) const noexcept;
		float OctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept;

	private:
		constexpr float GetFade(const float t) const noexcept;
		constexpr float GetLerp(const float t, const float a, const float b) const noexcept;
		constexpr float MakeGrad(const std::uint8_t hash, const float u, const float v) const noexcept;
		constexpr float MakeGrad(const std::uint8_t hash, const float x, const float y, const float z) const noexcept;
		constexpr float GetGrad(const std::uint8_t hash, const float x, const float y, const float z) const noexcept;
		float SetNoise(float x, float y = 0.f, float z = 0.f) const noexcept;
		float SetOctaveNoise(const std::size_t octaves, float x) const noexcept;
		float SetOctaveNoise(const std::size_t octaves, float x, float y) const noexcept;
		float SetOctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept;

	private:
		std::array<std::uint8_t, 512> mPixel;
	};

	inline float PerlinNoise::Noise(float x) const noexcept
	{
		return SetNoise(x);
	}

	inline float PerlinNoise::Noise(float x, float y) const noexcept
	{
		return SetNoise(x, y);
	}

	inline float PerlinNoise::Noise(float x, float y, float z) const noexcept
	{
		return SetNoise(x, y, z);
	}

	inline float PerlinNoise::OctaveNoise(const std::size_t octaves, float x) const noexcept
	{
		return SetOctaveNoise(octaves, x);
	}

	inline float PerlinNoise::OctaveNoise(const std::size_t octaves, float x, float y) const noexcept
	{
		return SetOctaveNoise(octaves, x, y);
	}

	inline float PerlinNoise::OctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept
	{
		return SetOctaveNoise(octaves, x, y, z);
	}
}
