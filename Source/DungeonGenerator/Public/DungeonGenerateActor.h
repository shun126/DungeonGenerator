/*!
\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include "DungeonGenerateActor.generated.h"

class CDungeonGenerator;
class UDungeonGenerateParameter;
class UDungeonMiniMapTexture;
class UTransactionalHierarchicalInstancedStaticMeshComponent;

namespace dungeon
{
	class Room;
}


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonGeneratorActorSignature, const FTransform&, transform);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDungeonGeneratorDoorSignature, AActor*, doorActor, EDungeonRoomProps, props);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDungeonGeneratorPlayerStartSignature, const FVector&, location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FDungeonGeneratorDelegete, bool, StreamingLevel, EDungeonRoomParts, DungeonRoomParts, const FBox&, RoomRect);

UCLASS(BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API ADungeonGenerateActor : public AActor
{
	GENERATED_BODY()

public:
	explicit ADungeonGenerateActor(const FObjectInitializer& initializer);
	virtual ~ADungeonGenerateActor();

	/*!
	ミニマップ用のテクスチャを生成します
	*/
	UFUNCTION(BlueprintCallable)
		UDungeonMiniMapTexture* GenerateMiniMapTexture(const int32 textureWidth, const uint8 currentLevel = 255, const uint8 lowerLevel = 255);

	/*!
	最後に生成したミニマップ用のテクスチャを取得します
	*/
	UFUNCTION(BlueprintCallable)
		UDungeonMiniMapTexture* GetLastGeneratedMiniMapTexture() const;

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
	static void StartAddInstance(TArray<UTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs);
	static void AddInstance(TArray<UTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs, const UStaticMesh* staticMesh, const FTransform& transform);
	static void EndAddInstance(TArray<UTransactionalHierarchicalInstancedStaticMeshComponent*>& meshs);

	static inline FBox ToWorldBoundingBox(const std::shared_ptr<const dungeon::Room>& room, const float gridSize);

	void LoadStreamLevels();
	void UnloadStreamLevels();
	void LoadStreamLevel(const FName& levelName, const FTransform& levelTransform);

	void MovePlayerStart();

	void PostGenerate();

#if WITH_EDITOR
	void DrawDebugInfomation();
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UDungeonGenerateParameter* DungeonGenerateParameter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool InstancedStaticMesh = true;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> FloorMeshs;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> SlopeMeshs;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> WallMeshs;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> RoomRoofMeshs;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> AisleRoofMeshs;

	UPROPERTY(Transient, BlueprintReadOnly)
		TArray<TObjectPtr<UTransactionalHierarchicalInstancedStaticMeshComponent>> PillarMeshs;

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
	プレイヤーをスタート位置へ移動する通知
	PreInitializeComponentsのタイミングで呼び出されます
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

#if WITH_EDITORONLY_DATA
	// 部屋と接続情報のデバッグ情報を表示します
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient)
		bool ShowRoomAisleInfomation = false;

	// ボクセルグリッドのデバッグ情報を表示します
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient)
		bool ShowVoxelGridType = false;
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
	TWeakObjectPtr<UDungeonMiniMapTexture> mLastGeneratedMiniMapTexture = nullptr;
	bool mPostGenerated = false;
};
