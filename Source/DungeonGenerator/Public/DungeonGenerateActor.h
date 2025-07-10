/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonGenerateBase.h"
#include "DungeonInstancedMeshCluster.h"
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

/**
 * Mesh generation method
 * メッシュの生成方法
 */
UENUM()
enum class EDungeonMeshGenerationMethod : uint8
{
	StaticMesh,
	InstancedStaticMesh,
	HierarchicalInstancedStaticMesh
};

/**
 * Dungeon generation actor
 * ダンジョン生成アクター
 */
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API ADungeonGenerateActor : public ADungeonGenerateBase
{
	GENERATED_BODY()

public:
	/**
	 * constructor
	 * コンストラクタ
	 */
	explicit ADungeonGenerateActor(const FObjectInitializer& initializer);

	/**
	 * destructor
	 * デストラクタ
	 */
	virtual ~ADungeonGenerateActor() override = default;

	/**
	 * Generate new dungeon
	 * ダンジョンを生成します
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "DungeonGenerator")
	void GenerateDungeon();

protected:
	/**
	 * Multicast dungeon generation to all clients
	 * ダンジョン生成を全クライアントにマルチキャストします
	 */
	UFUNCTION(NetMulticast, Reliable, Category = "DungeonGenerator")
	void MulticastOnGenerateDungeon();

public:
	/**
	 * Generate new dungeon
	 * ダンジョンを生成します
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void GenerateDungeonWithParameter(UDungeonGenerateParameter* DungeonGenerateParameter);

	/**
	 * Destroy dungeon
	 * ダンジョンを破棄します
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	void DestroyDungeon();

	/**
	 * Finds the floor from the world Z coordinate
	 * 高さからダンジョンの階層を検索します
	 * @param[in]	z	Z coordinate of world
	 * @return			floor
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 FindFloorHeight(const float z) const;

	/**
	 * Finds the Z coordinate of the grid from the world Z coordinate
	 * ワールドのZ座標からボクセルのZ座標を検索します
	 * @param[in]	z	Z coordinate of world
	 * @return		Z coordinate of the grid
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
	int32 FindVoxelHeight(const float z) const;


#if WITH_EDITOR
	/**
	 * Get CRC32 at generation
	 * 生成時のCRC32を取得します
	 */
	int32 GetGeneratedDungeonCRC32() const noexcept;
#endif

	/**
	 * Gets the size of the grid
	 * グリッドのサイズを取得します
	 */
	float GetGridSize() const;

	/**
	 * Get the largest room size
	 * 最も大きい部屋のサイズを取得します
	 */
	FVector GetRoomMaxSize() const;

	/**
	 * Sets the culling distance for InstancedMesh
	 * InstancedMeshのカリング距離を設定します
	 */
	void SetInstancedMeshCullDistance(const FInt32Interval& cullDistance);

	// AActor overrides
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	// ADungeonGenerateBase overrides
	virtual void OnPreDungeonGeneration() override;
	virtual void OnPostDungeonGeneration(const bool result) override;
	virtual void Dispose(const bool flushStreamLevels) override;
	virtual void FitNavMeshBoundsVolume() override;

	static uint32 InstancedMeshHash(const FVector& position, const double quantizationSize = 50 * 100);
	void BeginInstanceTransaction();
	void AddInstance(UStaticMesh* staticMesh, const FTransform& transform, const EDungeonMeshGenerationMethod meshGenerationMethod);
	void EndInstanceTransaction();
	void DestroyAllInstance();
	void ApplyInstancedMeshCullDistance();

	void PreGenerateImplementation();

#if WITH_EDITOR
	void DrawDebugInformation() const;
#endif

protected:
	/**
	 * Dungeon generation parameters
	 * ダンジョン生成パラメータ
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	TObjectPtr<UDungeonGenerateParameter> DungeonGenerateParameter;

#if WITH_EDITORONLY_DATA
	/**
	 * Generated random number seeds
	 * 生成時の乱数の種
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
	int32 GeneratedRandomSeed = 0;

	/**
	 * Generated dungeon hash
	 * 生成時のCRC32
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
	int32 GeneratedDungeonCRC32 = 0;
#endif

	/**
	 * Generates a dungeon at the start of the level
	 * レベル開始時にダンジョンを生成します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	bool AutoGenerateAtStart = true;

	/**
	 * Types of floors, ramps, and mezzanine meshes used for dungeon generation
	 * ダンジョン生成に使用する床、スロープ、中二階のメッシュの種類
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", DisplayName = "DungeonFloorSlopeMeshGenerationMethod")
	EDungeonMeshGenerationMethod DungeonMeshGenerationMethod = EDungeonMeshGenerationMethod::StaticMesh;

	/**
	 * Wall, ceiling, and pillar mesh types used for dungeon generation
	 * ダンジョン生成に使用する壁、天井、柱のメッシュの種類
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
	EDungeonMeshGenerationMethod DungeonWallRoofPillarMeshGenerationMethod = EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh;


	/**
	 * build job tag
	 * ビルドジョブのタグ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString BuildJobTag;

	/**
	 * license tag
	 * ライセンスタグ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString LicenseTag;

	/**
	 * License ID
	 * ライセンスID
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
	FString LicenseId;

#if WITH_EDITORONLY_DATA
	/**
	 * Displays voxel grid debugging information
	 * ボクセルグリッドのデバッグ情報を表示する
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	bool ShowVoxelGridType = false;

	/**
	 * Displays debugging information for the voxel grid at the player's position
	 * プレイヤーの位置にあるボクセルグリッドのデバッグ情報を表示します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug")
	bool ShowVoxelGridTypeAtPlayerLocation = false;
#endif

	/**
	 * Location of the starting room of the dungeon
	 * PlayerStart location can be found in Get All Actors of Class
	 * 
	 * ダンジョンのスタート部屋の位置
	 * PlayerStartの位置はGet All Actors of Classで検索して下さい
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	FVector StartRoomLocation;

	/**
	 * Location of the goal room in the dungeon
	 * ダンジョンのゴール部屋の位置
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator")
	FVector GoalRoomLocation;

	/**
	 * Manage clusters of UnstancedStaticMeshComponent or UHierarchicalInstancedStaticMeshComponent
	 * UInstancedStaticMeshComponentまたはUHierarchicalInstancedStaticMeshComponentのクラスターを管理します
	 */
	UPROPERTY(Transient)
	TMap<uint32, FDungeonInstancedMeshCluster> mInstancedMeshCluster;

private:
	FInt32Interval mInstancedMeshCullDistance = { 0, 0};
};
