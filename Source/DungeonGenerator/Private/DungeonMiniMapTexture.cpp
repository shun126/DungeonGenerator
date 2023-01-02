/*
テクスチャ

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "DungeonMiniMapTexture.h"
#include "DungeonGenerator.h"

UDungeonMiniMapTexture::UDungeonMiniMapTexture(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

void UDungeonMiniMapTexture::BeginDestroy()
{
	Super::BeginDestroy();
	DestroyMiniMapTexture();
}

bool UDungeonMiniMapTexture::GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentLevel, const uint8 lowerLevel)
{
	if (dungeonGenerator == nullptr)
		return false;

	DestroyMiniMapTexture();

	uint32_t horizontalScale;
	Texture = dungeonGenerator->GenerateMiniMapTexture(horizontalScale, textureWidth, currentLevel, lowerLevel);
	if (IsValid(Texture) == false)
		return false;

	Texture->AddToRoot();

	GeneratedScale = horizontalScale;
	GeneratedScaleVector.Set(GeneratedScale, GeneratedScale);

	return true;
}

void UDungeonMiniMapTexture::DestroyMiniMapTexture()
{
	if (IsValid(Texture))
	{
		Texture->RemoveFromRoot();
		Texture->ConditionalBeginDestroy();
		Texture = nullptr;
	}
}

FVector2D UDungeonMiniMapTexture::ToRelative(const FVector& location) const
{
	const float ratio = 1.f / mGridSize * GeneratedScale;
	return FVector2D(location.X * ratio, location.Y * ratio);
}

FVector2D UDungeonMiniMapTexture::ToRelativeWithScale(const FVector& location, const double scale) const
{
	return ToRelative(location) * scale;
}
