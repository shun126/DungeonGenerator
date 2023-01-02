/*
テクスチャ

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonMiniMapTexture.generated.h"

class CDungeonGenerator;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API UDungeonMiniMapTexture : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonMiniMapTexture(const FObjectInitializer& initializer);
	virtual ~UDungeonMiniMapTexture() = default;

	UFUNCTION(BlueprintCallable)
		FVector2D ToRelative(const FVector& location) const;

	UFUNCTION(BlueprintCallable)
		FVector2D ToRelativeWithScale(const FVector& location, const double scale) const;

protected:
	// overrides
	virtual void BeginDestroy() override;

private:
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentLevel, const uint8 lowerLevel);
	void DestroyMiniMapTexture();

protected:
	UPROPERTY(BlueprintReadOnly)
		UTexture2D* Texture = nullptr;

	UPROPERTY(BlueprintReadOnly)
		FVector2D GeneratedScaleVector;

	UPROPERTY(BlueprintReadOnly)
		float GeneratedScale = 1.f;

private:
	float mGridSize;

	friend class ADungeonGenerateActor;
};
