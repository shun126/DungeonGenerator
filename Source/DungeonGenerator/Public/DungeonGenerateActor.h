/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "DungeonRoomParts.h"
#include <GameFramework/Actor.h>
#include <memory>
#include "DungeonGenerateActor.generated.h"

// 前方宣言
class CDungeonGenerator;
class UDungeonGenerateParameter;
class UDungeonMiniMapTextureLayer;
class UDungeonTransactionalHierarchicalInstancedStaticMeshComponent;

namespace dungeon
{
	class Room;
}

// イベント宣言
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonGeneratorActorSignature, const FTransform&, transform);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonGeneratorDoorSignature, AActor*, doorActor, EDungeonRoomProps, props);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonGeneratorPlayerStartSignature, const FVector&, location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDungeonGeneratorDelegete, bool, StreamingLevel, EDungeonRoomParts, DungeonRoomParts, const FBox&, RoomRect);

/**
dungeon generation actor
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


	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 FindFloorHeight(const double z) const;


	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		int32 FindVoxelHeight(const double z) const;

	/**
	Generates a texture layer for the minimap
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTextureLayer* GenerateMiniMapTextureLayer(const int32 textureWidth);

	/**
	Get a texture layer for a generated minimap
	*/
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator")
		UDungeonMiniMapTextureLayer* GetGeneratedMiniMapTextureLayer() const;

	UFUNCTION()
		void OnMoveStreamLevel();

	// AActor overrides
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

private:
	void ReleaseHierarchicalInstancedStaticMeshComponents();

	static void StartAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs);

	static void AddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs, const UStaticMesh* staticMesh, const FTransform& transform);

	static void EndAddInstance(TArray<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs);

	static inline FBox ToWorldBoundingBox(const std::shared_ptr<const dungeon::Room>& room, const float gridSize);

	void LoadStreamLevels();
	void UnloadStreamLevels();
	void LoadStreamLevel(const FName& levelName, const FTransform& levelTransform);

	void MovePlayerStart();

	void PostGenerate();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		UDungeonGenerateParameter* DungeonGenerateParameter = nullptr;

#if WITH_EDITORONLY_DATA
	//! Random number seeds
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator")
		int32 GeneratedRandomSeed = 0;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator")
		bool InstancedStaticMesh = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> FloorMeshs;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> SlopeMeshs;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> WallMeshs;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> RoomRoofMeshs;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> AisleRoofMeshs;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "DungeonGenerator")
		TArray<TObjectPtr<UDungeonTransactionalHierarchicalInstancedStaticMeshComponent>> PillarMeshs;

	// event
	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreateFloor;

	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreateSlope;

	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreateWall;

	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreateRoomRoof;

	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreateAisleRoof;

	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorActorSignature OnCreatePillar;


	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorDoorSignature OnResetDoor;





	/*
	Notification to move the player to the starting position
	Called at the timing of PreInitializeComponents
	*/
	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorPlayerStartSignature OnMovePlayerStart;

	/*
	部屋の生成通知
	通知先で敵アクターなどを生成する事を想定しています
	最初のTickで呼び出されます
	（検討中の機能）
	*/
	UPROPERTY(BlueprintAssignable, Category = "Event")
		FDungeonGeneratorDelegete OnRoomCreated;

	// Cache of the UIDungeonMiniMapTextureLayer
	UPROPERTY(BlueprintReadOnly, Transient, Category = "DungeonGenerator")
		UDungeonMiniMapTextureLayer* DungeonMiniMapTextureLayer;
	

	// build job tag
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
		FString BuildJobTag;

	// license tag
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
		FString LicenseTag;

	// License ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail")
		FString LicenseId;

#if WITH_EDITORONLY_DATA && (UE_BUILD_SHIPPING == 0)
	// Displays debugging information on room and connection information
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug")
		bool ShowRoomAisleInfomation = false;

	// Displays voxel grid debugging information
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug")
		bool ShowVoxelGridType = false;

	// Displays debugging information for the voxel grid at the player's position
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug")
		bool ShowVoxelGridTypeAtPlayerLocation = false;

private:
	void DrawDebugInfomation();
#endif

private:
	struct OnMoveStreamLevelParameter final
	{
		FName mName;
		FTransform mTransform;
	} mOnMoveStreamLevel;

	TMap<FName, FTransform> mCreateRequestLevels;

private:
	std::shared_ptr<CDungeonGenerator> mDungeonGenerator = nullptr;
	bool mPostGenerated = false;
};
