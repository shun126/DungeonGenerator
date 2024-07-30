/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "Mission/DungeonRoomParts.h"
#include "Mission/DungeonRoomProps.h"
#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <memory>
#include "DungeonGenerateActor.generated.h"

class CDungeonGeneratorCore;
class UDungeonGenerateParameter;
class UDungeonMiniMapTextureLayer;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;

namespace dungeon
{
	class Room;
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationSuccessSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDungeonGeneratorActorNotifyGenerationFailureSignature);

/**
メッシュの生成方法
*/
UENUM()
enum class EDungeonMeshGenerationMethod : uint8
{
	StaticMesh,
	InstancedStaticMesh,
	HierarchicalInstancedStaticMesh
};

/**
Dungeon generation actor
ダンジョン生成アクター
*/
UCLASS(Blueprintable, BlueprintType)
class DUNGEONGENERATOR_API ADungeonGenerateActor : public AActor
{
	GENERATED_BODY()

public:
	/**
	constructor
	*/
	explicit ADungeonGenerateActor(const FObjectInitializer& initializer);

	/**
	destructor
	*/
	virtual ~ADungeonGenerateActor();

	/**
	Generate new dungeon
	ダンジョンを生成します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void GenerateDungeon();

	/**
	Generate new dungeon
	ダンジョンを生成します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void GenerateDungeonWithParameter(UDungeonGenerateParameter* DungeonGenerateParameter);

	/**
	Destroy  dungeon
	ダンジョンを破棄します
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void DestroyDungeon();

	/**
	Finds the floor from the world Z coordinate
	高さからダンジョンの階層を検索します
	@param[in]	z	Z coordinate of world
	@return			floor
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 FindFloorHeight(const float z) const;

	/**
	Finds the Z coordinate of the grid from the world Z coordinate
	ワールドのZ座標からボクセルのZ座標を検索します
	@param[in]	z	Z coordinate of world
	@return		Z coordinate of the grid
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 FindVoxelHeight(const float z) const;


	/**
	Calculate CRC32
	ダンジョンのCRC32を計算します
	@return		CRC32
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 CalculateCRC32() const noexcept;

#if WITH_EDITOR
	/*
	Get CRC32 at generation
	生成時のCRC32を取得します
	*/
	int32 GetGeneratedDungeonCRC32() const noexcept;
#endif

	/*
	Calculate the bounding box for the dungeon
	ダンジョン全体のバウンディングボックスを計算します
	*/
	FBox CalculateBoundingBox() const;

	/*
	Get the largest room size
	最も大きい部屋のサイズを取得します
	*/
	FVector GetRoomMaxSize() const;

	/*
	Sets the culling distance for InstancedMesh
	InstancedMeshのカリング距離を設定します
	*/
	void SetInstancedMeshCullDistance(const double cullDistance);

	// AActor overrides
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void CreateInstancedMeshComponent(UStaticMesh* staticMesh);
	void ClearInstancedMeshComponents();
	void AddInstance(const UStaticMesh* staticMesh, const FTransform& transform);
	void CommitAddInstance();

	static inline FBox ToWorldBoundingBox(const std::shared_ptr<const dungeon::Room>& room, const float gridSize);

	void PreGenerateImplementation();
	void DestroyImplementation();

protected:
	/*
	Dungeon generation parameters
	ダンジョン生成パラメータ
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TObjectPtr<UDungeonGenerateParameter> DungeonGenerateParameter;

#if WITH_EDITORONLY_DATA
	/*
	Generated random number seeds
	生成時の乱数の種
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
	int32 GeneratedRandomSeed = 0;

	/*
	Generated dungeon hash
	生成時のCRC32
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
	int32 GeneratedDungeonCRC32 = 0;
#endif

	/*
	PlayerStart is automatically moved to the start room at the start
	開始時にPlayerStartを自動的にスタート部屋に移動します
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool AutoGenerateAtStart = true;

	/*
	Type of mesh used for dungeon generation
	ダンジョン生成に使用するメッシュの種類
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonMeshGenerationMethod DungeonMeshGenerationMethod = EDungeonMeshGenerationMethod::StaticMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> InstancedStaticMeshes;


	/*
	build job tag
	ビルドジョブのタグ
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString BuildJobTag;

	/*
	license tag
	ライセンスタグ
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString LicenseTag;

	/*
	License ID
	ライセンスID
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString LicenseId;

#if WITH_EDITORONLY_DATA
	/*
	Displays voxel grid debugging information
	ボクセルグリッドのデバッグ情報を表示する
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	bool ShowVoxelGridType = false;

	/*
	Displays debugging information for the voxel grid at the player's position
	プレイヤーの位置にあるボクセルグリッドのデバッグ情報を表示します。
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	bool ShowVoxelGridTypeAtPlayerLocation = false;
#endif

	/*
	Notification when a dungeon is successfully created
	ダンジョンの生成に成功した時の通知
	*/
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationSuccessSignature OnGenerationSuccess;

	/*
	Notification when dungeon creation fails
	ダンジョンの生成に失敗した時の通知
	*/
	UPROPERTY(BlueprintAssignable, Category = "DungeonGenerator|Event")
	FDungeonGeneratorActorNotifyGenerationFailureSignature OnGenerationFailure;

	/*
	Location of the starting room of the dungeon
	PlayerStart location can be found in Get All Actors of Class
	ダンジョンのスタート部屋の位置
	PlayerStartの位置はGet All Actors of Classで検索して下さい
	*/
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	FVector StartRoomLocation;

	/*
	Location of the goal room in the dungeon
	ダンジョンのゴール部屋の位置
	*/
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	FVector GoalRoomLocation;

#if WITH_EDITOR
private:
	void DrawDebugInformation();
#endif

private:
	// Cache of the UIDungeonMiniMapTextureLayer
	std::shared_ptr<CDungeonGeneratorCore> mDungeonGeneratorCore;
};
