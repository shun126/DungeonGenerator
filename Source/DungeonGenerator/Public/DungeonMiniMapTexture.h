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

	// 生成したテクスチャ
	UTexture2D* GetTexture() const noexcept;

	// Voxel空間からテクスチャ空間への変換
	const FVector2D& GetGeneratedScaleVector() const noexcept;

	// Voxel空間からテクスチャ空間への変換
	float GetGeneratedScale() const noexcept;

protected:
	// overrides
	virtual void BeginDestroy() override;

private:
	/**
	ミニマップテクスチャを生成します
	*/
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const uint8 currentFloor);

	/**
	ミニマップテクスチャを破棄します
	*/
	void DestroyMiniMapTexture();

protected:
	// 生成したテクスチャ
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		UTexture2D* Texture = nullptr;

	// Voxel空間からテクスチャ空間への変換
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		FVector2D GeneratedScaleVector;

	// Voxel空間からテクスチャ空間への変換
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		float GeneratedScale = 1.f;

	friend class ADungeonGenerateActor;
	friend class UDungeonMiniMapTextureLayer;
};
