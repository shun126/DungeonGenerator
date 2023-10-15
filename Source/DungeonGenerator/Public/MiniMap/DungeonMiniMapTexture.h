/*
Minimap Texture

\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <memory>
#include "DungeonMiniMapTexture.generated.h"

class CDungeonGeneratorCore;
class UTexture2D;

/**
Minimap Textures class
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
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UTexture2D* GetTexture() const noexcept;

	// Conversion from Voxel space to texture space
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		const FVector2D& GetGeneratedScaleVector() const noexcept;

	// Conversion from Voxel space to texture space
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		float GetGeneratedScale() const noexcept;

protected:
	// overrides
	virtual void BeginDestroy() override;

private:
	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTextureWithSize(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureWidth, const uint8 currentFloor);

	/**
	Generates a minimap texture
	*/
	bool GenerateMiniMapTextureWithScale(const std::shared_ptr<CDungeonGeneratorCore>& dungeonGeneratorCore, const uint32_t textureScale, const uint8 currentFloor);

	/**
	Destroy minimap textures
	*/
	void DestroyMiniMapTexture();

protected:
	// Generated textures
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		UTexture2D* Texture;
	// TObjectPtr not used for UE4 compatibility

	// Transformation scale from Voxel space to texture space
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		FVector2D GeneratedScaleVector;

	// Transformation scale from Voxel space to texture space
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		float GeneratedScale = 1.f;

	friend class ADungeonGenerateActor;
	friend class UDungeonMiniMapTextureLayer;
};

inline UDungeonMiniMapTexture::UDungeonMiniMapTexture(const FObjectInitializer& initializer)
	: Super(initializer)
{
}

inline void UDungeonMiniMapTexture::BeginDestroy()
{
	Super::BeginDestroy();
	DestroyMiniMapTexture();
}

inline UTexture2D* UDungeonMiniMapTexture::GetTexture() const noexcept
{
	return Texture;
}

inline const FVector2D& UDungeonMiniMapTexture::GetGeneratedScaleVector() const noexcept
{
	return GeneratedScaleVector;
}

inline float UDungeonMiniMapTexture::GetGeneratedScale() const noexcept
{
	return GeneratedScale;
}
