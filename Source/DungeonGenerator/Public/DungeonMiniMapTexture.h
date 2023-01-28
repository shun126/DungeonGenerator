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

/**
ミニマップテクスチャクラス
*/
UCLASS(BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API UDungeonMiniMapTexture : public UObject
{
	GENERATED_BODY()

public:
	/**
	コンストラクタ
	*/
	explicit UDungeonMiniMapTexture(const FObjectInitializer& initializer);

	/**
	デストラクタ
	*/
	virtual ~UDungeonMiniMapTexture() = default;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelative(const FVector& location) const;

	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeWithScale(const FVector& location, const double scale) const;

protected:
	// overrides
	virtual void BeginDestroy() override;

private:
	/**
	ミニマップテクスチャを生成します
	*/
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentLevel, const uint8 lowerLevel);

	/**
	ミニマップテクスチャを破棄します
	*/
	void DestroyMiniMapTexture();

protected:
	//! 生成したテクスチャ
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		UTexture2D* Texture = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		FVector2D GeneratedScaleVector;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		float GeneratedScale = 1.f;

private:
	float mGridSize;

	friend class ADungeonGenerateActor;
};
