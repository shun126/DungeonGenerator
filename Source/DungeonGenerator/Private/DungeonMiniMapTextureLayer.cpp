/*
Minimap texture classes generated per layer

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonMiniMapTextureLayer.h"
#include "DungeonMiniMapTexture.h"
#include "DungeonGeneratorCore.h"
#include "Core/Generator.h"

FVector2D UDungeonMiniMapTextureLayer::GetTextureSize() const noexcept
{
	if (DungeonMiniMapTextures.Num() == 0)
		return FVector2D::ZeroVector;

	const UDungeonMiniMapTexture* dungeonMiniMapTexture = DungeonMiniMapTextures[0];
	if (!IsValid(dungeonMiniMapTexture))
		return FVector2D::ZeroVector;

	const UTexture2D* texture = dungeonMiniMapTexture->GetTexture();
	if (!IsValid(dungeonMiniMapTexture))
		return FVector2D::ZeroVector;

	return FVector2D(texture->GetSizeX(), texture->GetSizeY());
}

bool UDungeonMiniMapTextureLayer::GenerateMiniMapTextureWithSize(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureWidth, const float gridSize)
{
	if (dungeonGeneratorCore == nullptr)
		return false;

	mGenerator = dungeonGeneratorCore->GetGenerator();
	if (mGenerator == nullptr)
		return false;

	const std::vector<int32_t>& floorHeight = mGenerator->GetFloorHeight();
	DungeonMiniMapTextures.Reset(floorHeight.size());

	for (uint8_t i = 0; i < floorHeight.size(); ++i)
	{
		UDungeonMiniMapTexture* miniMapTexture = NewObject<UDungeonMiniMapTexture>(this);
		if (IsValid(miniMapTexture))
		{
			const int32_t height = floorHeight[i];
			if (miniMapTexture->GenerateMiniMapTextureWithSize(dungeonGeneratorCore, textureWidth, height) == false)
				miniMapTexture = nullptr;
		}
		DungeonMiniMapTextures.Add(miniMapTexture);
	}

	mGridSize = gridSize;

	return true;
}

bool UDungeonMiniMapTextureLayer::GenerateMiniMapTextureWithScale(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t dotScale, const float gridSize)
{
	if (dungeonGeneratorCore == nullptr)
		return false;

	mGenerator = dungeonGeneratorCore->GetGenerator();
	if (mGenerator == nullptr)
		return false;

	const std::vector<int32_t>& floorHeight = mGenerator->GetFloorHeight();
	DungeonMiniMapTextures.Reset(floorHeight.size());

	for (uint8_t i = 0; i < floorHeight.size(); ++i)
	{
		UDungeonMiniMapTexture* miniMapTexture = NewObject<UDungeonMiniMapTexture>(this);
		if (IsValid(miniMapTexture))
		{
			const int32_t height = floorHeight[i];
			if (miniMapTexture->GenerateMiniMapTextureWithScale(dungeonGeneratorCore, dotScale, height) == false)
				miniMapTexture = nullptr;
		}
		DungeonMiniMapTextures.Add(miniMapTexture);
	}

	mGridSize = gridSize;

	return true;
}

UDungeonMiniMapTexture* UDungeonMiniMapTextureLayer::GetByFloor(const uint8 floor) const noexcept
{
	if (floor < DungeonMiniMapTextures.Num())
		return DungeonMiniMapTextures[floor];
	else
		return nullptr;
}

UDungeonMiniMapTexture* UDungeonMiniMapTextureLayer::GetByHeight(const float height) const noexcept
{
	const uint8_t voxelHeight = static_cast<uint8_t>(height / mGridSize);
	return GetByFloor(mGenerator->FindFloor(voxelHeight));
}

FVector2D UDungeonMiniMapTextureLayer::ToRelative(const FVector& location) const
{
	if (DungeonMiniMapTextures.Num() <= 0)
	{
		return FVector2D::ZeroVector;
	}
	else
	{
		const float ratio = 1.f / mGridSize * DungeonMiniMapTextures[0]->GetGeneratedScale();
		return FVector2D(location.X, location.Y) * ratio;
	}
}

FVector2D UDungeonMiniMapTextureLayer::ToRelativeAndFloor(uint8& floor, const FVector& location) const
{
	const uint8_t voxelHeight = static_cast<uint8_t>(location.Z / mGridSize);
	floor = mGenerator->FindFloor(voxelHeight);

	return ToRelative(location);
}

FVector2D UDungeonMiniMapTextureLayer::ToNormalize(const FVector& location) const
{
	const FVector2D& textureSize = GetTextureSize();
	if (textureSize.IsNearlyZero())
		return FVector2D::ZeroVector;

	return ToRelative(location) / textureSize;
}
