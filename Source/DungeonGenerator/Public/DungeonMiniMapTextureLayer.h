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
ミニマップテクスチャクラス
*/
UCLASS(BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API UDungeonMiniMapTextureLayer : public UObject
{
	GENERATED_BODY()

public:
	/**
	コンストラクタ
	*/
	explicit UDungeonMiniMapTextureLayer(const FObjectInitializer& initializer);

	/**
	デストラクタ
	*/
	virtual ~UDungeonMiniMapTextureLayer() = default;

	/**
	総階層数を取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 GetFloorHeight() const noexcept;

	/**
	階層からUDungeonMiniMapTextureを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTexture* GetByFloor(const uint8 floor) const noexcept;

	/**
	高さからUDungeonMiniMapTextureを取得します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTexture* GetByHeight(const double height) const noexcept;

	/**
	ワールド座標からスクリーン座標に変換します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelative(const FVector& location) const;

	/**
	ワールド座標からスクリーン座標に変換します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeWithScale(const FVector& location, const double scale) const;

	/**
	ワールド座標からスクリーン座標と階層に変換します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		FVector2D ToRelativeAndFloor(uint8& floor, const FVector& location) const;

private:
	/**
	ミニマップテクスチャを生成します
	*/
	bool GenerateMiniMapTexture(const std::shared_ptr<const CDungeonGenerator>& dungeonGenerator, const uint32_t textureWidth, const float gridSize);

protected:
	// 階層毎のダンジョンミニマップテクスチャ
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<UDungeonMiniMapTexture*> DungeonMiniMapTextures;

private:
	// Generatorへの参照
	std::shared_ptr<const dungeon::Generator> mGenerator;

	// ワールド空間からVoxel空間への変換
	float mGridSize = 1.f;

	friend class ADungeonGenerateActor;
};
