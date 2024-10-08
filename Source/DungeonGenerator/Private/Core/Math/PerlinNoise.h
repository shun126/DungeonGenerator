/**
パーリンノイズに関するヘッダーファイル

@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <array>
#include <cstdint>
#include <memory>

namespace dungeon
{
	class Random;

	/*!
	パーリンノイズクラス
	*/
	class PerlinNoise final
	{
	public:
		explicit PerlinNoise(const std::shared_ptr<Random>& random);
		~PerlinNoise() = default;

		// SEED値を設定する
		void SetSeed(const std::shared_ptr<Random>& random);

		/*
		オクターブ無しノイズを取得する
		[-1.0 ~ 1.0]
		*/
		float Noise(float x, float y) const noexcept;
		float Noise(float x, float y, float z) const noexcept;

		/*
		オクターブ有りノイズを取得する
		[-1.0 ~ 1.0]
		*/
		float OctaveNoise(const std::size_t octaves, float x, float y) const noexcept;
		float OctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept;

	private:
		static constexpr float GetFade(const float t) noexcept;
		static constexpr float GetLerp(const float t, float a, const float b) noexcept;
		static constexpr float MakeGrad(const std::uint8_t hash, const float x, const float y, const float z) noexcept;
		static constexpr float GetGrad(const std::uint8_t hash, const float x, const float y, const float z) noexcept;
		float SetNoise(float x, float y, float z = 0.f) const noexcept;
		float SetOctaveNoise(const std::size_t octaves, float x, float y) const noexcept;
		float SetOctaveNoise(const std::size_t octaves, float x, float y, float z) const noexcept;

	private:
		std::array<std::uint8_t, 512> mHash;
	};

	inline float PerlinNoise::Noise(float x, float y) const noexcept
	{
		return SetNoise(x, y);
	}

	inline float PerlinNoise::Noise(float x, float y, float z) const noexcept
	{
		return SetNoise(x, y, z);
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
