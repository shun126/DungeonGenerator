/*
階層毎に生成されたミニマップテクスチャクラス
*/

#include "DungeonMiniMapTextureLayer.h"
#include "DungeonMiniMapTexture.h"
#include "DungeonGenerator.h"
#include "Core/Generator.h"

UDungeonMiniMapTextureLayer::UDungeonMiniMapTextureLayer(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

bool UDungeonMiniMapTextureLayer::GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const float gridSize)
{
	if (dungeonGenerator == nullptr)
		return false;

	mGenerator = dungeonGenerator->GetGenerator();
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
			if (miniMapTexture->GenerateMiniMapTexture(dungeonGenerator, textureWidth, height) == false)
				miniMapTexture = nullptr;
		}

		DungeonMiniMapTextures.Add(miniMapTexture);
	}

	mGridSize = gridSize;

	return true;
}

UDungeonMiniMapTexture* UDungeonMiniMapTextureLayer::GetByFloor(const uint8 floor) const noexcept
{
	return DungeonMiniMapTextures.IsEmpty() ? nullptr : DungeonMiniMapTextures[floor];
}

UDungeonMiniMapTexture* UDungeonMiniMapTextureLayer::GetByHeight(const double height) const noexcept
{
	const uint8_t voxelHeight = static_cast<uint8_t>(height / mGridSize);
	return GetByFloor(mGenerator->FindFloor(voxelHeight));
}

int32 UDungeonMiniMapTextureLayer::GetFloorHeight() const noexcept
{
	return DungeonMiniMapTextures.Num();
}

FVector2D UDungeonMiniMapTextureLayer::ToRelative(const FVector& location) const
{
	if (DungeonMiniMapTextures.IsEmpty())
		FVector2D::ZeroVector;

	const float ratio = 1.f / mGridSize * DungeonMiniMapTextures[0]->GetGeneratedScale();
	return FVector2D(location.X * ratio, location.Y * ratio);
}

FVector2D UDungeonMiniMapTextureLayer::ToRelativeWithScale(const FVector& location, const double scale) const
{
	return ToRelative(location) * scale;
}

FVector2D UDungeonMiniMapTextureLayer::ToRelativeAndFloor(uint8& floor, const FVector& location) const
{
	const uint8_t voxelHeight = static_cast<uint8_t>(location.Z / mGridSize);
	floor = mGenerator->FindFloor(voxelHeight);

	return ToRelative(location);
}
