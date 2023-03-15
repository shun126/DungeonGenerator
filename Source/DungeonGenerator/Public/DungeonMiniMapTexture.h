/*
テクスチャ

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonMiniMapTexture.generated.h"

class CDungeonGenerator;
class UTexture2D;

/**
Minimap Textures
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonMiniMapTexture : public UObject
{
	GENERATED_BODY()

public:
	/**
	constructor
	*/
	explicit UDungeonMiniMapTexture(const FObjectInitializer& initializer);

	/**
	destructor
	*/
	virtual ~UDungeonMiniMapTexture() = default;

	/*
	Generated textures
	*/
	UTexture2D* GetTexture() const noexcept;

	// Conversion from Voxel space to texture space
	const FVector2D& GetGeneratedScaleVector() const noexcept;

	// Conversion from Voxel space to texture space
	float GetGeneratedScale() const noexcept;

protected:
	// overrides
	virtual void BeginDestroy() override;

private:
	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentFloor);

	/**
	Discard minimap textures
	*/
	void DestroyMiniMapTexture();

protected:
	// Generated textures
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		UTexture2D* Texture = nullptr;
	// TObjectPtr<UTexture2D> not used for UE4 compatibility

	// Transformation scale from Voxel space to texture space
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		FVector2D GeneratedScaleVector;

	// Transformation scale from Voxel space to texture space
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		float GeneratedScale = 1.f;

	friend class ADungeonGenerateActor;
	friend class UDungeonMiniMapTextureLayer;
};
