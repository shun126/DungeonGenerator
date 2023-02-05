/**
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomAsset.h"
#include "DungeonRoomSensor.h"
#include <functional>
#include "DungeonGenerateParameter.generated.h"

// 前方宣言
class UDungeonRoomAsset;

namespace dungeon
{
	class Direction;
	class Grid;
	class Random;
}

/**
配置方向
*/
UENUM(BlueprintType)
enum class EPlacementDirection : uint8
{
	North,
	East,
	South,
	West,
	FollowGridDirection,
	RandomDirection
};

/**
パーツ選択方法
*/
UENUM(BlueprintType)
enum class EDungeonPartsSelectionMethod : uint8
{
	Random,
	GridIndex,
	Direction,
};

/**
メッシュパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonParts
{
	GENERATED_BODY()

	// 相対的なトランスフォーム
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		FTransform RelativeTransform;

	FTransform CalculateWorldTransform(const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(const FVector& position, const dungeon::Direction& direction) const noexcept;

protected:
	/**
	パーツのトランスフォームを計算します
	\param[in]	random				dungeon::Random&
	\param[in]	transform			FTransform
	\param[in]	placementDirection	EPlacementDirection 
	\return		EPlacementDirectionに応じたトランスフォーム
	*/
	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform, const EPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator, const EPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw, const EPlacementDirection placementDirection) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction, const EPlacementDirection placementDirection) const noexcept;
};

/**
メッシュパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshParts : public FDungeonParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		UStaticMesh* StaticMesh = nullptr;
};

/**
方向指定付きメッシュパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshPartsWithDirection : public FDungeonMeshParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		EPlacementDirection PlacementDirection = EPlacementDirection::RandomDirection;

	/**
	パーツのトランスフォームを計算します
	\param[in]	random				dungeon::Random&
	\param[in]	transform			FTransform
	\param[in]	placementDirection	EPlacementDirection
	\return		EPlacementDirectionに応じたトランスフォーム
	*/
	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
};

/**
アクターパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorParts : public FDungeonParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "Actor"))
		UClass* ActorClass = nullptr;
};

/**
方向指定付きアクターパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorPartsWithDirection : public FDungeonActorParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		EPlacementDirection PlacementDirection = EPlacementDirection::RandomDirection;

	/**
	パーツのトランスフォームを計算します
	\param[in]	random				dungeon::Random&
	\param[in]	transform			FTransform
	\param[in]	placementDirection	EPlacementDirection
	\return		EPlacementDirectionに応じたトランスフォーム
	*/
	FTransform CalculateWorldTransform(dungeon::Random& random, const FTransform& transform) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const FRotator& rotator) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const float yaw) const noexcept;
	FTransform CalculateWorldTransform(dungeon::Random& random, const FVector& position, const dungeon::Direction& direction) const noexcept;
};

/**
アクターおよびブループリント構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonObjectParts : public FDungeonParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (AllowedClasses = "Actor, Blueprint"))
		FSoftObjectPath ActorPath;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		UClass* ActorClass = nullptr;
	UClass* GetActorClass();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
		EPlacementDirection PlacementDirection = EPlacementDirection::RandomDirection;
};

/**
確率付きアクターおよびブループリント構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRandomObjectParts : public FDungeonObjectParts
{
	GENERATED_BODY()

	// 生成頻度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin="0.", ClampMax = "1."))
		float Frequency = 1.f;
};

/**
ダンジョン生成パラメータ
*/
UCLASS(Blueprintable)
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject
{
	GENERATED_BODY()

public:
	/*
	コンストラクタ
	*/
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);

	/*
	デストラクタ
	*/
	virtual ~UDungeonGenerateParameter() = default;

	int32 GetRandomSeed() const;
	int32 GetGeneratedRandomSeed() const;
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

	int32 GetNumberOfCandidateRooms() const;

	const FInt32Interval& GetRoomWidth() const noexcept;
	const FInt32Interval& GetRoomDepth() const noexcept;
	const FInt32Interval& GetRoomHeight() const noexcept;

	int32 GetUnderfloorHeight() const;

	int32 GetRoomMargin() const;

	float GetGridSize() const;

	const FDungeonMeshPartsWithDirection* SelectFloorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectWallParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshPartsWithDirection* SelectRoomRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachRoomRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshPartsWithDirection* SelectAisleRoofParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachAisleRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	const FDungeonMeshParts* SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	void EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonRandomObjectParts* SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;
	//const FDungeonRandomObjectParts* SelectChandelierParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;

	const FDungeonObjectParts* SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random) const;

	const FDungeonActorPartsWithDirection& GetStartParts() const;

	const FDungeonActorPartsWithDirection& GetGoalParts() const;

	void EachDungeonRoomAsset(std::function<void(const UDungeonRoomAsset*)> func) const;

	UClass* GetRoomSensorClass() const;

	/*
	Converts from a grid coordinate system to a world coordinate system
	2D空間は（X軸:前 Y軸:右）
	3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
	*/
	FVector ToWorld(const FIntVector& location) const;
	
	/*
	Converts from a grid coordinate system to a world coordinate system
	2D空間は（X軸:前 Y軸:右）
	3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
	*/
	FVector ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const;

	/*
	Converts from a grid coordinate system to a world coordinate system
	2D空間は（X軸:前 Y軸:右）
	3D空間は（X軸:前 Y軸:右 Z軸:上）である事に注意
	*/
	FIntVector ToGrid(const FVector& location) const;

#if WITH_EDITOR
	void DumpToJson() const;
#endif

private:
	template<typename T = FDungeonMeshParts>
	T* SelectDungeonMeshParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	FDungeonObjectParts* SelectDungeonObjectParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonObjectParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	FDungeonRandomObjectParts* SelectDungeonRandomObjectParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<FDungeonRandomObjectParts>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const;

	template<typename T = FDungeonMeshParts>
	void EachMeshParts(const TArray<T>& parts, std::function<void(const T&)> func) const;

#if WITH_EDITOR
	FString GetJsonDefaultDirectory() const;
#endif

protected:
	//! 乱数の種 (0なら自動生成)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
		int32 RandomSeed = 0;

	//! 乱数の種 (0なら自動生成)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		int32 GeneratedRandomSeed = 0;

	/**
	生成する部屋の数の候補
	部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
		int32 NumberOfCandidateRooms = 8;

	//! 部屋の幅
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomWidth = { 4, 8 };

	//! 部屋の奥行き
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomDepth = { 4, 8 };

	//! 部屋の高さ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomHeight = { 2, 3 };

	//! 床下の高さ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 UnderfloorHeight = 0;

	//! 部屋と部屋の余白
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 RoomMargin = 1;

	//! 部屋と部屋の結合
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0"))
		bool MergeRooms = false;

	//! グリッドサイズ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
		float GridSize = 100.f;

	//!	床のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		EDungeonPartsSelectionMethod FloorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 床のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Floor", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> FloorParts;

	//!	階段やスロープのパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
		EDungeonPartsSelectionMethod SloopPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 階段やスロープのパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Sloop", BlueprintReadWrite)
		TArray<FDungeonMeshParts> SlopeParts;

	//!	壁のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		EDungeonPartsSelectionMethod WallPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 壁のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Wall", BlueprintReadWrite)
		TArray<FDungeonMeshParts> WallParts;

	//!	部屋のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomRoof", BlueprintReadWrite)
		EDungeonPartsSelectionMethod RoomRoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 部屋の屋根のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomRoof", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> RoomRoofParts;

	//!	部屋のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|AisleRoof", BlueprintReadWrite)
		EDungeonPartsSelectionMethod AisleRoofPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 通路の屋根のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|AisleRoof", BlueprintReadWrite)
		TArray<FDungeonMeshPartsWithDirection> AisleRoofParts;

	//!	柱のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Pillar", BlueprintReadWrite)
		EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! 柱のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Pillar", BlueprintReadWrite)
		TArray<FDungeonMeshParts> PillarParts;

	//!	たいまつ（柱の照明）のパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Torch", BlueprintReadWrite)
		EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	//! たいまつ（柱の照明）のパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Torch", BlueprintReadWrite)
		TArray<FDungeonRandomObjectParts> TorchParts;

/*
	シャンデリア（天井の照明）のパーツ
	UPROPERTY(EditAnywhere, Category="DungeonGenerator", BlueprintReadWrite)
		TArray<FDungeonRandomObjectParts> ChandelierParts;
*/

	//!	ドアのパーツの生成方法
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Door", BlueprintReadWrite)
		EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	// ドアのパーツ
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Door", BlueprintReadWrite)
		TArray<FDungeonObjectParts> DoorParts;

	// スタート地点
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		FDungeonActorPartsWithDirection StartParts;

	// ゴール位置
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
		FDungeonActorPartsWithDirection GoalParts;

	// 部屋のセンサークラス
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensor"))
		UClass* DungeonRoomSensorClass = ADungeonRoomSensor::StaticClass();

	//! サブレベル配置
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator")
		TArray<UDungeonRoomAsset*> DungeonRoomAssets;

	friend class CDungeonGenerator;
};

// aka: SelectDungeonObjectParts, SelectDungeonRandomObjectParts
template <typename T>
inline T* UDungeonGenerateParameter::SelectDungeonMeshParts(const size_t gridIndex, const dungeon::Grid& grid, dungeon::Random& random, const TArray<T>& parts, const EDungeonPartsSelectionMethod partsSelectionMethod) const
{
	const int32 size = parts.Num();
	if (size <= 0)
		return nullptr;

	int32 index = 0;

	switch (partsSelectionMethod)
	{
	case EDungeonPartsSelectionMethod::GridIndex:
		index = 0 % size;
		break;

	case EDungeonPartsSelectionMethod::Direction:
		index = grid.GetDirection().Get() % size;
		break;

	case EDungeonPartsSelectionMethod::Random:
	default:
		index = random.Get<uint32_t>(size);
		break;
	}

	return const_cast<T*>(&parts[index]);
}

template <typename T>
inline void UDungeonGenerateParameter::EachMeshParts(const TArray<T>& parts, std::function<void(const T&)> func) const
{
	for (const auto& part : parts)
	{
		func(part);
	}
}
