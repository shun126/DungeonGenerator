/*
Minimap texture classes generated per layer

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonMiniMapTextureLayer.generated.h"

class ADungeonGenerateActor;
class CDungeonGeneratorCore;
class UDungeonMiniMapTexture;
class UTexture2D;

namespace dungeon
{
	class Generator;
}

/**
Minimap Texture Layer
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonMiniMapTextureLayer : public UObject
{
	GENERATED_BODY()

public:
	/**
	constructor
	*/
	explicit UDungeonMiniMapTextureLayer(const FObjectInitializer& initializer);

	/**
	destructor
	*/
	virtual ~UDungeonMiniMapTextureLayer() = default;

	/**
	Get UDungeonMiniMapTexture from height
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D GetTextureSize() const noexcept;

	/**
	Get the total number of levels
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 GetFloorHeight() const noexcept;

	/**
	Get UDungeonMiniMapTexture from level
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTexture* GetByFloor(const uint8 floor) const noexcept;

	/**
	Get UDungeonMiniMapTexture from height
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTexture* GetByHeight(const float height) const noexcept;

	/**
	Converts from world coordinates to screen coordinates
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelative(const FVector& location) const;

	/**
	Converts from world coordinates to screen coordinates
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeWithScale(const FVector& location, const float scale) const;

	/**
	Converts from world coordinates to screen coordinates and level
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeAndFloor(uint8& floor, const FVector& location) const;

	/**
	Converts from world coordinates to normalized coordinates (0 to 1)
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToNormalize(const FVector& location) const;

private:
	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTextureWithSize(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureWidth, const float gridSize);

	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTextureWithScale(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureScale, const float gridSize);

private:
	// Dungeon minimap textures for each level
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowPrivateAccess = "true"))
		TArray<UDungeonMiniMapTexture*> DungeonMiniMapTextures;
	// TObjectPtr<UDungeonMiniMapTexture> not used for UE4 compatibility

	// Reference to Generator
	std::shared_ptr<const dungeon::Generator> mGenerator;

	// Transformation scale from world space to Voxel space
	float mGridSize = 1.f;

	friend class ADungeonGenerateActor;
	friend class FDungeonGenerateEditorModule;
};

inline UDungeonMiniMapTextureLayer::UDungeonMiniMapTextureLayer(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

inline int32 UDungeonMiniMapTextureLayer::GetFloorHeight() const noexcept
{
	return DungeonMiniMapTextures.Num();
}

inline FVector2D UDungeonMiniMapTextureLayer::ToRelativeWithScale(const FVector& location, const float scale) const
{
	return ToRelative(location) * scale;
}
