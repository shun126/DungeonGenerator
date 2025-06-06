/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGridSize.h"
#include "DungeonMeshSetDatabase.h"
#include "Parameter/DungeonDoorActorParts.h"
#include <CoreMinimal.h>
#include <functional>
#include <memory>
#include "DungeonGenerateParameter.generated.h"

// forward declaration

/**
Frequency of generation
生成頻度
*/
UENUM()
enum class EFrequencyOfGeneration : uint8
{
	Normally,
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
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonGenerateParameter : public UObject
{
	GENERATED_BODY()

public:
	explicit UDungeonGenerateParameter(const FObjectInitializer& ObjectInitializer);
	virtual ~UDungeonGenerateParameter() override = default;

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
	
	bool IsUseMissionGraph() const noexcept;
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

	/**
	 * ランダムなダンジョンのパラメータを生成します
	 * @param sourceParameter	コピー元パラメータ（メッシュ等の設定をコピーします）
	 * @return DungeonGenerateParameter
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	static UDungeonGenerateParameter* GenerateRandomParameter(const UDungeonGenerateParameter* sourceParameter) noexcept;

	/**
	 * ランダムなダンジョンのパラメータにリセットします
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void SetRandomParameter() noexcept;

public:
#if WITH_EDITOR

	/**
	Output parameters in JSON format
	パラメータをJSON形式で出力します
	*/
	UFUNCTION(Category = "DungeonGenerator", meta = (CallInEditor = "true"))
	void Dump() const;
#endif

private:
	void SetRandomSeed(const int32 generateRandomSeed);
	void SetGeneratedRandomSeed(const int32 generatedRandomSeed);

#if WITH_EDITOR
	void DumpToJson() const;
	FString GetJsonDefaultDirectory() const;
#endif

	static const FDungeonMeshSet* SelectParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);

	const FDungeonMeshParts* SelectSlopeParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;

	const UDungeonMeshSetDatabase* GetDungeonRoomMeshPartsDatabase() const noexcept;
	const UDungeonMeshSetDatabase* GetDungeonAisleMeshPartsDatabase() const noexcept;
	static const FDungeonMeshPartsWithDirection* SelectFloorParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);
	static const FDungeonMeshParts* SelectCatwalkParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);
	static const FDungeonMeshParts* SelectWallPartsByGrid(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);
	static const FDungeonMeshParts* SelectWallPartsByFace(const FDungeonMeshSet* dungeonMeshSet, const FIntVector& gridLocation, const dungeon::Direction& direction);
	static const FDungeonMeshPartsWithDirection* SelectRoofParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);
	static const FDungeonMeshParts* SelectSlopeParts(const UDungeonMeshSetDatabase* dungeonMeshSetDatabase, const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random);

	const FDungeonMeshParts* SelectPillarParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonRandomActorParts* SelectTorchParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;
	const FDungeonDoorActorParts* SelectDoorParts(const size_t gridIndex, const dungeon::Grid& grid, const std::shared_ptr<dungeon::Random>& random) const;

	void EachFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
void EachPillarParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	void EachRoomFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachRoomWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoomRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachRoomSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachRoomCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	void EachAisleFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachAisleWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachAisleRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const;
	void EachAisleSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const;
	void EachAisleCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const;

	int32 GetGeneratedDungeonCRC32() const noexcept;
	void SetGeneratedDungeonCRC32(const int32 generatedDungeonCRC32) noexcept;
	
public:
	// overrides
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override;

protected:
	/**
	Seed of random number (if 0, auto-generated)

	乱数のシード（0なら自動生成）
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 RandomSeed = 0;

	/**
	Seed of random numbers used for the last generation

	最後の生成に使用された乱数のシード
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator")
	int32 GeneratedRandomSeed = 0;

	/**
	CRC32 of the last dungeon generated

	最後に生成されたダンジョンのCRC32
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Transient, Category = "DungeonGenerator")
	int32 GeneratedDungeonCRC32 = 0;

	/**
	Room Width

	部屋の幅
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomWidth = { 3, 8 };

	/**
	Room depth

	部屋の奥行き
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomDepth = { 3, 8 };

	/**
	Room height

	部屋の高さ
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", meta = (UIMin = 1, ClampMin = 1))
	FInt32Interval RoomHeight = { 2, 4 };

	/**
	Horizontal room-to-room margins
	MergeRooms must be unchecked to enable Room Horizontal Margin

	水平方向の部屋と部屋の空白
	Room Horizontal Marginを有効にするにはMergeRoomsのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "1", EditCondition = "!MergeRooms"), DisplayName = "Horizontal Room Margin")
	uint8 RoomMargin = 2;

	/**
	Vertical room-to-room margins
	MergeRooms must be unchecked to enable Room Vertical Margin

	垂直方向の部屋と部屋の空白
	Room Vertical Marginを有効にするにはMergeRoomsのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (ClampMin = "0", EditCondition = "!MergeRooms && !Flat "), DisplayName = "Vertical Room Margin")
	uint8 VerticalRoomMargin = 0;

	/**
	Candidate number of rooms to be generated
	This is the initial number of rooms to be generated, not the final number of rooms to be generated.

	生成される部屋数の候補
	これは最終的な部屋の数ではなく最初に生成される部屋の数です。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "3", ClampMax = "100"))
	uint8 NumberOfCandidateRooms = 10;

	/**
	Horizontal room-to-room coupling

	有効にすると部屋と部屋を結合します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
	bool MergeRooms = false;

	/**
	Candidate Number of Generated Hierarchies This is used as a reference number of hierarchies for generation,
	not as the final number of hierarchies.

	生成される階層数の候補
	これは最終的な階層の数ではなく生成時の参考階層数として利用されます。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "5", EditCondition = "!Flat"))
	uint8 NumberOfCandidateFloors = 3;

	/**
	Generates a flat dungeon

	平面的なダンジョンを生成します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (EditCondition = "!MergeRooms"))
	bool Flat = false;

	/**
	PlayerStart is automatically moved to the start room at the start

	開始時にPlayerStartを自動的にスタート部屋に移動します
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite)
	bool MovePlayerStartToStartingPoint = true;

	/**
	Enable MissionGraph to generate missions with keys.
	MergeRooms must be unchecked and AisleComplexity must be 0 for UseMissionGraph to be enabled.

	MissionGraphを有効にして、鍵を使ったミッション情報を生成します
	UseMissionGraphを有効にするには、MergeRoomsのチェックを外しAisleComplexityを0にする必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadWrite, meta = (EditCondition = "!MergeRooms"))
	bool UseMissionGraph = false;

	/**
	Aisle complexity (0 being the minimum aisle)
	MergeRooms and UseMissionGraph must be unchecked to enable AisleComplexity

	通路の複雑さ（０が最低限の通路）
	AisleComplexityを有効にするには、MergeRoomsとUseMissionGraphのチェックを外す必要があります
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "10", EditCondition = "!MergeRooms && !UseMissionGraph"))
	uint8 AisleComplexity = 5;

	/**
	 * Generate slopes in the room.
	 * May be enabled by future forcing.
	 *
	 * 部屋の中にスロープを生成する
	 * 将来的強制的に有効になる可能性があります。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
	bool GenerateSlopeInRoom = false;

	/**
	 * Generate structural columns in the room
	 * May be enabled by future forcing.
	 * 
	 * 部屋の中に構造柱を生成する
	 * 将来的強制的に有効になる可能性があります。
	 */
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator", BlueprintReadOnly)
	bool GenerateStructuralColumn = false;

	/**
	Horizontal voxel size

	水平方向のボクセルサイズ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|GridSize", meta = (ClampMin = 1), DisplayName = "Horizontal Size")
	float GridSize = 400.f;

	/**
	Vertical voxel size

	垂直方向のボクセルサイズ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|GridSize", meta = (ClampMin = 1), DisplayName = "Vertical Size")
	float VerticalGridSize = 400.f;

	/**
	Room mesh parts database

	部屋のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts")
	TObjectPtr<UDungeonMeshSetDatabase> DungeonRoomMeshPartsDatabase;

	/**
	Aisle mesh parts database

	通路のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts")
	TObjectPtr<UDungeonMeshSetDatabase> DungeonAisleMeshPartsDatabase;

	/**
	How to generate parts of pillar

	柱のパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Pillar")
	EDungeonPartsSelectionMethod PillarPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Pillar Parts

	柱のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Pillar")
	TArray<FDungeonMeshParts> PillarParts;

	/**
	How to generate parts for torch

	燭台のパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Torch")
	EDungeonPartsSelectionMethod TorchPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Frequency of torchlight generation

	燭台の生成頻度
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Torch")
	EFrequencyOfGeneration FrequencyOfTorchlightGeneration = EFrequencyOfGeneration::Rarely;

	/**
	Torch (pillar lighting) parts

	燭台のメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Torch")
	TArray<FDungeonRandomActorParts> TorchParts;

	/**
	How to generate door parts

	ドアのパーツを生成する方法
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Door")
	EDungeonPartsSelectionMethod DoorPartsSelectionMethod = EDungeonPartsSelectionMethod::Random;

	/**
	Door Parts

	ドアのメッシュパーツデータベース
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Parts|Door")
	TArray<FDungeonDoorActorParts> DoorParts;


	/**
	Specify the DungeonRoomSensorBase class,
	which is a box sensor that covers the room and controls doors and enemy spawn.

	DungeonRoomSensorBaseクラスを指定して下さい。
	DungeonRoomSensorBaseは部屋を覆う箱センサーで、ドアや敵のスポーンを制御します。
	*/
	UPROPERTY(EditAnywhere, Category = "DungeonGenerator|RoomSensor", BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensorBase"))
	TObjectPtr<UClass> DungeonRoomSensorClass;

	/**
	PluginVersion

	プラグインバージョン
	*/
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	uint8 PluginVersion;

	friend class ADungeonGenerateBase;
	friend class ADungeonGenerateActor;
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

inline bool UDungeonGenerateParameter::IsUseMissionGraph() const noexcept
{
	return GetAisleComplexity() <= 0;
}

inline uint8 UDungeonGenerateParameter::GetAisleComplexity() const noexcept
{
	return UseMissionGraph == false ? AisleComplexity : 0;
}

inline bool UDungeonGenerateParameter::IsAisleComplexity() const noexcept
{
	return GetAisleComplexity() > 0;
}

inline EFrequencyOfGeneration UDungeonGenerateParameter::GetFrequencyOfTorchlightGeneration() const noexcept
{
	return FrequencyOfTorchlightGeneration;
}

inline const UDungeonMeshSetDatabase* UDungeonGenerateParameter::GetDungeonRoomMeshPartsDatabase() const noexcept
{
	return DungeonRoomMeshPartsDatabase;
}

inline const UDungeonMeshSetDatabase* UDungeonGenerateParameter::GetDungeonAisleMeshPartsDatabase() const noexcept
{
	return DungeonAisleMeshPartsDatabase;
}

inline UClass* UDungeonGenerateParameter::GetRoomSensorClass() const
{
	return DungeonRoomSensorClass;
}

inline void UDungeonGenerateParameter::EachFloorParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	EachRoomFloorParts(function);
	EachAisleFloorParts(function);
}

inline void UDungeonGenerateParameter::EachWallParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomWallParts(function);
	EachAisleWallParts(function);
}

inline void UDungeonGenerateParameter::EachRoofParts(const std::function<void(const FDungeonMeshPartsWithDirection&)>& function) const
{
	EachRoomRoofParts(function);
	EachAisleRoofParts(function);
}

inline void UDungeonGenerateParameter::EachSlopeParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomSlopeParts(function);
	EachAisleSlopeParts(function);
}

inline void UDungeonGenerateParameter::EachCatwalkParts(const std::function<void(const FDungeonMeshParts&)>& function) const
{
	EachRoomCatwalkParts(function);
	EachAisleCatwalkParts(function);
}
