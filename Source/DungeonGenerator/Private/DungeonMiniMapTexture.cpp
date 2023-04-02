/*
テクスチャ

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonMiniMapTexture.h"
#include "DungeonGenerator.h"

bool UDungeonMiniMapTexture::GenerateMiniMapTexture(const UDungeonGenerator* dungeonGenerator, const uint32_t textureWidth, const uint8 currentFloor)
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
