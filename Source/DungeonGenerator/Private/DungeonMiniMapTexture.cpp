/*
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonMiniMapTexture.h"
#include "DungeonGeneratorCore.h"

bool UDungeonMiniMapTexture::GenerateMiniMapTextureWithSize(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureWidth, const uint8 currentFloor)
{
	if (dungeonGeneratorCore == nullptr)
		return false;

	DestroyMiniMapTexture();

	uint32_t horizontalScale;
	Texture = dungeonGeneratorCore->GenerateMiniMapTextureWithSize(horizontalScale, textureWidth, currentFloor);
	if (IsValid(Texture) == false)
		return false;

	GeneratedScale = horizontalScale;
	GeneratedScaleVector.Set(GeneratedScale, GeneratedScale);

	return true;
}

bool UDungeonMiniMapTexture::GenerateMiniMapTextureWithScale(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t dotScale, const uint8 currentFloor)
{
	if (dungeonGeneratorCore == nullptr)
		return false;

	DestroyMiniMapTexture();

	uint32_t horizontalScale;
	Texture = dungeonGeneratorCore->GenerateMiniMapTextureWithScale(horizontalScale, dotScale, currentFloor);
	if (IsValid(Texture) == false)
		return false;

	GeneratedScale = horizontalScale;
	GeneratedScaleVector.Set(horizontalScale, horizontalScale);

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
