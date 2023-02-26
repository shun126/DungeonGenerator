/*
テクスチャ

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
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

bool UDungeonMiniMapTexture::GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentFloor)
{
	if (dungeonGenerator == nullptr)
		return false;

	DestroyMiniMapTexture();

	uint32_t horizontalScale;
	Texture = dungeonGenerator->GenerateMiniMapTexture(horizontalScale, textureWidth, currentFloor);
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

UTexture2D* UDungeonMiniMapTexture::GetTexture() const noexcept
{
	return Texture;
}

const FVector2D& UDungeonMiniMapTexture::GetGeneratedScaleVector() const noexcept
{
	return GeneratedScaleVector;
}

float UDungeonMiniMapTexture::GetGeneratedScale() const noexcept
{
	return GeneratedScale;
}
