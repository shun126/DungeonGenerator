/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonRoomAsset.h"
#include "DungeonRoomSensor.h"
#include "Core/Math/Random.h"
#include <functional>
#include "DungeonGenerateParameter.generated.h"

// 前方宣言
class UDungeonRoomAsset;

/*
メッシュパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonMeshParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMesh* StaticMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform RelativeTransform;
	FTransform GetWorldTransform(const FTransform& toWorldTransform) const;
	FTransform GetWorldTransform(const float yaw, const FVector& position) const;
};

/*
アクターパーツ構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonActorParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "Actor"))
		UClass* ActorClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform RelativeTransform;
	FTransform GetWorldTransform(const FTransform& toWorldTransform) const;
	FTransform GetWorldTransform(const float yaw, const FVector& position) const;
};

/*
アクターおよびブループリント構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonObjectParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "Actor, Blueprint"))
		FSoftObjectPath ActorPath;

	UPROPERTY(Transient, BlueprintReadOnly)
		UClass* ActorClass = nullptr;
	UClass* GetActorClass();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FTransform RelativeTransform;
	FTransform GetWorldTransform(const FTransform& toWorldTransform) const;
	FTransform GetWorldTransform(const float yaw, const FVector& position) const;
};

/*
確率付きアクターおよびブループリント構造体
*/
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonRandomObjectParts : public FDungeonObjectParts
{
	GENERATED_BODY()

	// 生成頻度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin="0.", ClampMax = "1."))
		float Frequency = 1.f;
};

/*
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

	const FDungeonMeshParts* SelectFloorParts(dungeon::Random& random) const;
	void EachFloorParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectSlopeParts(dungeon::Random& random) const;
	void EachSlopeParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectWallParts(dungeon::Random& random) const;
	void EachWallParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectRoomRoofParts(dungeon::Random& random) const;
	void EachRoomRoofParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectAisleRoofParts(dungeon::Random& random) const;
	void EachAisleRoofParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonMeshParts* SelectPillarParts(dungeon::Random& random) const;
	void EachPillarParts(std::function<void(const FDungeonMeshParts&)> func) const;

	const FDungeonRandomObjectParts* SelectTorchParts(dungeon::Random& random) const;
	//const FDungeonRandomObjectParts* SelectChandelierParts(dungeon::Random& random) const;

	const FDungeonObjectParts* SelectDoorParts(dungeon::Random& random) const;

	const FDungeonActorParts& GetStartParts() const;

	const FDungeonActorParts& GetGoalParts() const;

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

private:
	FDungeonMeshParts* SelectDungeonMeshParts(dungeon::Random& random, const TArray<FDungeonMeshParts>& parts) const;

	void EachMeshParts(const TArray<FDungeonMeshParts>& parts, std::function<void(const FDungeonMeshParts&)> func) const;

	FDungeonObjectParts* SelectDungeonObjectParts(dungeon::Random& random, const TArray<FDungeonObjectParts>& parts) const;

	FDungeonRandomObjectParts* SelectDungeonRandomObjectParts(dungeon::Random& random, const TArray<FDungeonRandomObjectParts>& parts) const;

protected:
	//! 乱数の種 (0なら自動生成)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 RandomSeed = 0;

	//! 乱数の種 (0なら自動生成)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		int32 GeneratedRandomSeed = 0;

	/*!
	生成する部屋の数の候補
	部屋の初期生成数であり、最終的に生成される部屋の数ではありません。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
		int32 NumberOfCandidateRooms = 8;

	//! 部屋の幅
	UPROPERTY(EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomWidth = { 4, 8 };

	//! 部屋の奥行き
	UPROPERTY(EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomDepth = { 4, 8 };

	//! 部屋の高さ
	UPROPERTY(EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
		FInt32Interval RoomHeight = { 2, 3 };

	//! 床下の高さ
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 UnderfloorHeight = 0;

	//! 部屋と部屋の余白
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
		int32 RoomMargin = 1;

	//! 部屋と部屋の結合
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
		bool MergeRooms = false;

	//! グリッドサイズ
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float GridSize = 100.f;

	//! 床のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> FloorParts;

	//! 階段やスロープのパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> SlopeParts;

	//! 壁のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> WallParts;

	//! 部屋の屋根のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> RoomRoofParts;

	//! 通路の屋根のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> AisleRoofParts;

	//! 柱のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonMeshParts> PillarParts;

	//! たいまつ（柱の照明）のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonRandomObjectParts> TorchParts;

/*
	シャンデリア（天井の照明）のパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonRandomObjectParts> ChandelierParts;
*/

	// ドアのパーツ
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<FDungeonObjectParts> DoorParts;

	// スタート地点
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FDungeonActorParts StartParts;

	// ゴール位置
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FDungeonActorParts GoalParts;

	// 部屋のセンサークラス
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "DungeonRoomSensor"))
		UClass* DungeonRoomSensorClass = ADungeonRoomSensor::StaticClass();

	/*
	ダミー敵を指定します
	正式版ではこの機能は削除して下さい。
	敵の配置はADungeonRoomSensorを継承したクラスで行って下さい。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowedClasses = "Actor, Blueprint"))
		FSoftObjectPath DummyEnemyPath;

	//! サブレベル配置
	UPROPERTY(EditAnywhere)
		TArray<UDungeonRoomAsset*> DungeonRoomAssets;

	friend class CDungeonGenerator;
};
