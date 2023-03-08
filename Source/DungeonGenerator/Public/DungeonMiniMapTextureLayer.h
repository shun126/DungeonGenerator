/*
階層毎に生成されたミニマップテクスチャクラス

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonMiniMapTextureLayer.generated.h"

class CDungeonGenerator;
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
		UDungeonMiniMapTexture* GetByHeight(const double height) const noexcept;

	/**
	Converts from world coordinates to screen coordinates
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelative(const FVector& location) const;

	/**
	Converts from world coordinates to screen coordinates
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeWithScale(const FVector& location, const double scale) const;

	/**
	Converts from world coordinates to screen coordinates and level
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeAndFloor(uint8& floor, const FVector& location) const;

private:
	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const float gridSize);

protected:
	// 階層毎のダンジョンミニマップテクスチャ
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<UDungeonMiniMapTexture*> DungeonMiniMapTextures;

private:
	// Reference to Generator
	std::shared_ptr<const dungeon::Generator> mGenerator;

	// Transformation scale from world space to Voxel space
	float mGridSize = 1.f;

	friend class ADungeonGenerateActor;
};
