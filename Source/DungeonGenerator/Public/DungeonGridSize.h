/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include "DungeonGridSize.generated.h"

/*
Dungeon grid size
ダンジョンのグリッドサイズ
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonGridSize
{
    GENERATED_BODY()

public:
    /*
    コンストラクタ
    constructor
    */
    FDungeonGridSize() = default;

    /*
    コンストラクタ
    constructor
    */
    FDungeonGridSize(const float horizontalSize, const float verticalSize) noexcept
        : HorizontalSize(horizontalSize)
        , VerticalSize(verticalSize)
    {
    }

    /*
    コピーコンストラクタ
    copy constructor
    */
    FDungeonGridSize(const FDungeonGridSize& other) noexcept
        : HorizontalSize(other.HorizontalSize)
        , VerticalSize(other.VerticalSize)
    {
    }

    /*
    ムーブコンストラクタ
    move constructor
    */
    FDungeonGridSize(FDungeonGridSize&& other) noexcept
        : HorizontalSize(other.HorizontalSize)
        , VerticalSize(other.VerticalSize)
    {
    }

    /*
    デストラクタ
    destructor
    */
    virtual ~FDungeonGridSize() = default;

    /*
    コピー代入
    copy assignment
    */
    FDungeonGridSize& operator=(const FDungeonGridSize& other) noexcept
    {
        HorizontalSize = other.HorizontalSize;
        VerticalSize = other.VerticalSize;
        return *this;
    }

    /*
    ムーブ代入
    move assignment
    */
    FDungeonGridSize& operator=(FDungeonGridSize&& other) noexcept
    {
        HorizontalSize = other.HorizontalSize;
        VerticalSize = other.VerticalSize;
        return *this;
    }

    /*
    一致か調べます
    equal
    */
    bool operator==(const FDungeonGridSize& other) const noexcept
    {
        return HorizontalSize == other.HorizontalSize && VerticalSize == other.VerticalSize;
    }

    /*
    不一致か調べます
    not equal
    */
    bool operator!=(const FDungeonGridSize& other) const noexcept
    {
        return HorizontalSize != other.HorizontalSize || VerticalSize != other.VerticalSize;
    }

    /*
    FVector2Dにキャストします
    Cast to FVector2D
    */
    FVector2D To2D() const noexcept
    {
        return FVector2D(HorizontalSize, VerticalSize);
    }

    /*
    FVectorにキャストします
    Cast to FVector
    */
    FVector To3D() const noexcept
    {
        return FVector(HorizontalSize, HorizontalSize, VerticalSize);
    }

public:
    // 水平サイズ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
    float HorizontalSize = 400.f;

    // 垂直サイズ
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
    float VerticalSize = 400.f;
};
