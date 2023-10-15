#pragma once
#include <CoreMinimal.h>
#include "DungeonMiniMap.generated.h"

/*
A mini-map of the dungeon generated in the editor
It contains the mini-map textures of each floor and the information
about the conversion from world coordinates to texture coordinates.
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonMiniMap : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonMiniMap(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonMiniMap() = default;

	void AddTexture(UTexture2D* texture);

	float GetGridSize() const noexcept;
	void SetGridSize(const float gridSize);

	float GetWorldToTextureScale() const noexcept;
	void SetWorldToTextureScale(const float worldToTextureScale);

	void Add(const int32 elevation);

	// Conversion from World space to texture space
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D TransformWorldToTexture(const FVector worldLocation) const noexcept;

	// Conversion from World space to texture layer
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 TransformWorldToLayer(const float worldElevation) const noexcept;

protected:
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		TArray<UTexture2D*> Textures;
	// TObjectPtr not used for UE4 compatibility

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		TArray<int32> Elevations;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		float WorldToVoxelScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		float WorldToTextureScale = 1.f;
};

inline UDungeonMiniMap::UDungeonMiniMap(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

inline void UDungeonMiniMap::AddTexture(UTexture2D* texture)
{
	Textures.Add(texture);
}

inline float UDungeonMiniMap::GetGridSize() const noexcept
{
	return 1.f / WorldToVoxelScale;
}

inline void UDungeonMiniMap::SetGridSize(const float gridSize)
{
	WorldToVoxelScale = 1.f / gridSize;
}

inline float UDungeonMiniMap::GetWorldToTextureScale() const noexcept
{
	return WorldToTextureScale;
}

inline void UDungeonMiniMap::SetWorldToTextureScale(const float worldToTextureScale)
{
	WorldToTextureScale = worldToTextureScale;
}

inline void UDungeonMiniMap::Add(const int32 elevation)
{
	Elevations.Add(elevation);
}

inline FVector2D UDungeonMiniMap::TransformWorldToTexture(const FVector worldLocation) const noexcept
{
	return FVector2D(worldLocation.X, worldLocation.Y) * WorldToTextureScale;
}
