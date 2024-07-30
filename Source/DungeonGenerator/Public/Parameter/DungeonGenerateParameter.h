/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGridSize.h"
#include "DungeonAisleMeshSetDatabase.h"
#include "DungeonRoomMeshSetDatabase.h"
#include "Parameter/DungeonDoorActorParts.h"
#include "SubActor/DungeonRoomSensorBase.h"
#include <CoreMinimal.h>
#include <functional>
#include <memory>
#include "DungeonGenerateParameter.generated.h"

// forward declaration

/*
Frequency of generation
生成頻度
*/
UENUM()
enum class EFrequencyOfGeneration : uint8
{
	Normaly,
	Sometime,
	Occasionally,
	Rarely,
	AlmostNever,
	Never,
};

/**
Dungeon generation parameter
ダンジョン生成パラメータ
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonGenerateParameter() = default;

	int32 GetRandomSeed() const;
	int32 GetGeneratedRandomSeed() const;

	int32 GetNumberOfCandidateRooms() const;

	const FInt32Interval& GetRoomWidth() const noexcept;
	const FInt32Interval& GetRoomDepth() const noexcept;
	const FInt32Interval& GetRoomHeight() const noexcept;

	int32 GetHorizontalRoomMargin() const noexcept;
	int32 GetVerticalRoomMargin() const noexcept;

	FDungeonGridSize GetGridSize() const;

	bool IsMergeRooms() const noexcept;
	bool IsMovePlayerStartToStartingPoint() const noexcept;
	
	bool EnableMissionGraph() const noexcept;
	uint8 GetAisleComplexity() const noexcept;
	bool IsAisleComplexity() const noexcept;

	EFrequencyOfGeneration GetFrequencyOfTorchlightGeneration() const noexcept;


	UClass* GetRoomSensorClass() const;

	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const FIntVector& location) const;
	
	// Converts from a grid coordinate system to a world coordinate system
	FVector ToWorld(const uint32_t x, const uint32_t y, const uint32_t z) const;

	// 	Converts from a grid coordinate system to a world coordinate system
	FIntVector ToGrid(const FVector& location) const;

public:
#if WITH_EDITOR

	/*
	Output parameters in JSON format
	パラメータをJSON形式で出力します
	*/
	UFUNCTION(Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void Dump();
#endif

private:
	void SetRandomSeed(const int32 generateRandomSeed);
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

#if WITH_EDITOR
	void DumpToJson() const;
	FString GetJsonDefaultDirectory() const;
#endif

private:
	const UDungeonRoomMeshSetDatabase* GetDungeonRoomPartsDatabase() const noexcept;
	const UDungeonAisleMeshSetDatabase* GetDungeonAislePartsDatabase() const noexcept;

	const FDungeonMeshPartsWithDirection* SelectFloorParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectWallParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshPartsWithDirection* SelectRoofParts(const UDungeonMeshSetDatabase* dungeonPartsDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonMeshParts* SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonRandomActorParts* SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonDoorActorParts* SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;

	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const;

private:
	void EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	void EachRoomFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachRoomWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachRoomRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	void EachAisleFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;
	void EachAisleWallParts(std::function<void(const FDungeonMeshParts&)> func) const;
	void EachAisleRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const;

	int32 GetGeneratedDungeonCRC32() const noexcept;
	void SetGeneratedDungeonCRC32(const int32 generatedDungeonCRC32) noexcept;
	
public:
	// overrides
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

protected:
	/*
	Seed of random number (if 0, auto-generated)

	乱数のシード（0なら自動生成）
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 RandomSeed = 0;

	/*
	Seed of random numbers used for the last generation

	最後の生成に使用された乱数のシード
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator")
	int32 GeneratedRandomSeed = 0;

	/*
	CRC32 of the last dungeon generated

	最後に生成されたダンジョンのCRC32
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator")
	int32 GeneratedDungeonCRC32 = 0;

	/*
	Candidate number of rooms to be generated
	This is the initial number of rooms to be generated, not the final number of rooms to be generated.

	生成される部屋数の候補
	これは最終的な部屋の数ではなく最初に生成される部屋の数です。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "1"))
	uint8 NumberOfCandidateRooms = 20;

	/*
	Room Width

	部屋の幅
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomWidth = { 3, 8 };

	/*
	Room depth

	部屋の奥行き
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomDepth = { 3, 8 };

	/*
	Room height

	部屋の高さ
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomHeight = { 2, 4 };

	/*
	Horizontal room-to-room margins
	MergeRooms must be unchecked to enable Room Horizontal Margin

	水平方向の部屋と部屋の空白
	Room Horizontal Marginを有効にするにはMergeRoomsのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "1", EditCondition = "!MergeRooms"), DisplayName = "Horizontal Room Margin")
	int32 RoomMargin = 2;

	/*
	Vertical room-to-room margins
	MergeRooms must be unchecked to enable Room Vertical Margin

	垂直方向の部屋と部屋の空白
	Room Vertical Marginを有効にするにはMergeRoomsのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "!MergeRooms"), DisplayName = "Vertical Room Margin")
	int32 VerticalRoomMargin = 0;

	/*
	Horizontal room-to-room coupling

	有効にすると部屋と部屋を結合します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
	bool MergeRooms = false;

	/*
	Generates a flat dungeon

	平面的なダンジョンを生成します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (EditCondition = "!MergeRooms"))
	bool Flat = false;

	/*
	Move PlayerStart to the starting point.

	有効にするとPlayerStartをスタート部屋に移動します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
	bool MovePlayerStartToStartingPoint = true;

	/*
	Enable MissionGraph to generate missions with keys.
	MergeRooms must be unchecked and AisleComplexity must be 0 for UseMissionGraph to be enabled.

	MissionGraphを有効にして、鍵を使ったミッション情報を生成します
	UseMissionGraphを有効にするには、MergeRoomsのチェックを外しAisleComplexityを0にする必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (EditCondition = "!MergeRooms"))
	bool UseMissionGraph = false;

	/*
	Aisle complexity (0 being the minimum aisle)
	MergeRooms and UseMissionGraph must be unchecked to enable AisleComplexity

	通路の複雑さ（０が最低限の通路）
	AisleComplexityを有効にするには、MergeRoomsとUseMissionGraphのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "10", EditCondition = "!MergeRooms && !UseMissionGraph"))
	uint8 AisleComplexity = 5;

	/*
	Horizontal voxel size

	水平方向のボクセルサイズ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|GridSize", meta = (ClampMin = 1), DisplayName = "Horizontal Size")
	float GridSize = 400.f;

	/*
	Vertical voxel size

	垂直方向のボクセルサイズ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|GridSize", meta = (ClampMin = 1), DisplayName = "Vertical Size")
	float VerticalGridSize = 400.f;

	/*
	Room mesh parts database

	部屋のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts", BlueprintReadWrite)
	TObjectPtr<UDungeonRoomMeshSetDatabase> DungeonRoomPartsDatabase;

	/*
	Aisle mesh parts database

	通路のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts", BlueprintReadWrite)
	TObjectPtr<UDungeonAisleMeshSetDatabase> DungeonAislePartsDatabase;

	/*
	How to generate parts of pillar

	柱のパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Pillar", BlueprintReadWrite)
	EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Pillar Parts

	柱のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Pillar", BlueprintReadWrite)
	TArray<FDungeonMeshParts> PillarParts;

	/*
	How to generate parts for torch

	燭台のパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Torch", BlueprintReadWrite)
	EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Frequency of torchlight generation

	燭台の生成頻度
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Torch", BlueprintReadWrite)
	EFrequencyOfGeneration FrequencyOfTorchlightGeneration = EFrequencyOfGeneration::Rarely;

	/*
	Torch (pillar lighting) parts

	燭台のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Torch", BlueprintReadWrite)
	TArray<FDungeonRandomActorParts> TorchParts;

	/*
	How to generate door parts

	ドアのパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Door", BlueprintReadWrite)
	EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/*
	Door Parts

	ドアのメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|Parts|Door", BlueprintReadWrite)
	TArray<FDungeonDoorActorParts> DoorParts;


	/*
	Specify the DungeonRoomSensorBase class,
	which is a box sensor that covers the room and controls doors and enemy spawn.

	DungeonRoomSensorBaseクラスを指定して下さい。
	DungeonRoomSensorBaseは部屋を覆う箱センサーで、ドアや敵のスポーンを制御します。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomSensor", BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensorBase"))
	TObjectPtr<UClass> DungeonRoomSensorClass;

	/*
	PluginVersion

	プラグインバージョン
	*/
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 PluginVersion;

	friend class ADungeonGenerateActor;
	friend class CDungeonGeneratorCore;
};

inline int32 UDungeonGenerateParameter::GetRandomSeed() const
{
	return RandomSeed;
}

inline int32 UDungeonGenerateParameter::GetGeneratedRandomSeed() const
{
	return GeneratedRandomSeed;
}

inline int32 UDungeonGenerateParameter::GetNumberOfCandidateRooms() const
{
	return NumberOfCandidateRooms;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomWidth() const noexcept
{
	return RoomWidth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomDepth() const noexcept
{
	return RoomDepth;
}

inline const FInt32Interval& UDungeonGenerateParameter::GetRoomHeight() const noexcept
{
	return RoomHeight;
}

inline int32 UDungeonGenerateParameter::GetHorizontalRoomMargin() const noexcept
{
	return RoomMargin;
}

inline int32 UDungeonGenerateParameter::GetVerticalRoomMargin() const noexcept
{
	return VerticalRoomMargin;
}

inline FDungeonGridSize UDungeonGenerateParameter::GetGridSize() const
{
	return FDungeonGridSize(GridSize, VerticalGridSize);
}

inline bool UDungeonGenerateParameter::IsMergeRooms() const noexcept
{
	return MergeRooms;
}

inline bool UDungeonGenerateParameter::IsMovePlayerStartToStartingPoint() const noexcept
{
	return MovePlayerStartToStartingPoint;
}

inline bool UDungeonGenerateParameter::EnableMissionGraph() const noexcept
{
	return UseMissionGraph;
}

inline uint8 UDungeonGenerateParameter::GetAisleComplexity() const noexcept
{
	return UseMissionGraph == false ? AisleComplexity : 0;
}

inline bool UDungeonGenerateParameter::IsAisleComplexity() const noexcept
{
	return UseMissionGraph == false && AisleComplexity > 0;
}

inline EFrequencyOfGeneration UDungeonGenerateParameter::GetFrequencyOfTorchlightGeneration() const noexcept
{
	return FrequencyOfTorchlightGeneration;
}

inline const UDungeonRoomMeshSetDatabase* UDungeonGenerateParameter::GetDungeonRoomPartsDatabase() const noexcept
{
	return DungeonRoomPartsDatabase;
}

inline const UDungeonAisleMeshSetDatabase* UDungeonGenerateParameter::GetDungeonAislePartsDatabase() const noexcept
{
	return DungeonAislePartsDatabase;
}

inline UClass* UDungeonGenerateParameter::GetRoomSensorClass() const
{
	return DungeonRoomSensorClass;
}

inline void UDungeonGenerateParameter::EachFloorParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachRoomFloorParts(func);
	EachAisleFloorParts(func);
}

inline void UDungeonGenerateParameter::EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const
{
	EachRoomWallParts(func);
	EachAisleWallParts(func);
}

inline void UDungeonGenerateParameter::EachRoofParts(std::function<void(const FDungeonMeshPartsWithDirection&)> func) const
{
	EachRoomRoofParts(func);
	EachAisleRoofParts(func);
}
