/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "DungeonGenerateBase.h"
#include "DungeonGenerateActor.generated.h"

class CDungeonGeneratorCore;
class UDungeonGenerateParameter;
class UDungeonMiniMapTextureLayer;
class ADungeonSubLevelScriptActor;
class UInstancedStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class ADungeonMainLevelScriptActor;

namespace dungeon
{
	class Generator;
	class Room;
}

/**
 * Mesh generation method
 * メッシュの生成方法
 */
UENUM()
enum class EDungeonMeshGenerationMethod : uint8
{
	StaticMesh UMETA(DisplayName = "Static Mesh", ToolTip = "Spawn individual static mesh components."),
	InstancedStaticMesh UMETA(DisplayName = "Instanced Static Mesh", ToolTip = "Use instanced static mesh components."),
	HierarchicalInstancedStaticMesh UMETA(DisplayName = "Hierarchical Instanced Static Mesh", ToolTip = "Use hierarchical instanced static mesh components.")
};

/**
 * Transient authoritative state used to reproduce one dungeon generation on clients.
 * クライアントで同じダンジョン生成を再現するための一時的なAuthority状態です。
 */
USTRUCT()
struct FDungeonGenerationReplicatedState
{
	GENERATED_BODY()

	/**
	 * Monotonically increasing generation identifier.
	 * 単調増加する生成識別子です。
	 */
	UPROPERTY()
	uint64 GenerationId = 0;

	/**
	 * Seed resolved once by the authoritative server.
	 * Authorityサーバーが一度だけ確定するSeedです。
	 */
	UPROPERTY()
	int32 Seed = 0;

	/**
	 * CRC calculated by the authoritative server.
	 * Authorityサーバーが計算したCRCです。
	 */
	UPROPERTY()
	uint32 ServerCRC32 = 0;

	/**
	 * Parameter asset shared by every peer.
	 * すべてのピアで共有するParameter Assetです。
	 */
	UPROPERTY()
	TObjectPtr<UDungeonGenerateParameter> Parameter = nullptr;

	/**
	 * Floor, slope, and catwalk generation method.
	 * 床、スロープ、キャットウォークの生成方式です。
	 */
	UPROPERTY()
	EDungeonMeshGenerationMethod FloorGenerationMethod = EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh;

	/**
	 * Wall, roof, and pillar generation method.
	 * 壁、屋根、柱の生成方式です。
	 */
	UPROPERTY()
	EDungeonMeshGenerationMethod WallGenerationMethod = EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh;

	/**
	 * Whether this revision represents a generated dungeon.
	 * このRevisionが生成済みダンジョンを表すかを示します。
	 */
	UPROPERTY()
	bool bGenerated = false;
};

/**
 * Placeable runtime actor that owns one generated dungeon and synchronizes deterministic revisions over the network.
 * The authority resolves the parameter and seed, while each peer reconstructs non-replicated terrain from the same state.
 * 1つの生成Dungeonを所有し、決定的なRevisionをNetwork同期する配置可能なRuntime Actorです。
 * AuthorityがParameterとSeedを確定し、各Peerは同じ状態から非複製地形を再構築します。
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
	 * Registers replicated generation state.
	 * 複製する生成状態を登録します。
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if WITH_EDITOR

#endif

	/**
	 * Requests authoritative generation using DungeonGenerateParameter.
	 * DungeonGenerateParameterを使用したAuthority生成を要求します。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Generate new dungeon"))
	void GenerateDungeon();

	/**
	 * Requests authoritative generation using the supplied parameter asset for this revision.
	 * このRevisionに指定したParameter Assetを使用してAuthority生成を要求します。
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Generate a dungeon with the supplied parameter"))
	void GenerateDungeonWithParameter(UDungeonGenerateParameter* dungeonGenerateParameter);

protected:
	/**
	 * Applies finalized generation state to connected clients.
	 * 確定した生成状態を接続済みクライアントへ適用します。
	 */
	UFUNCTION(NetMulticast, Reliable, Category = "DungeonGenerator")
	void MulticastApplyGenerationState(const FDungeonGenerationReplicatedState& generationState);

public:
	/**
	 * Destroy dungeon
	 * ダンジョンを破棄します
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Destroy dungeon on the server and synchronize the empty state to all clients."))
	void DestroyDungeon();

	/**
	 * Changes both generated light intensities in every room on the authoritative server.
	 * Authorityを持つサーバーで、すべての部屋の2種類の生成ライトの明るさを変更します。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "DungeonGenerator|Lights", meta = (ClampMin = "0", ToolTip = "Changes base fill and guidance-light intensities in every generated room. Call this on the server; current values are synchronized to existing and late-joining clients."))
	void SetAllRoomLightIntensities(float baseFillIntensity, float guidanceIntensity);

	/**
	 * Finds the floor whose elevation is the highest one at or below the specified world Z coordinate.
	 * 指定したワールドZ座標以下で最も高い床面に対応する階層を返します。
	 * @param[in]	z	World Z coordinate. ワールドZ座標です。
	 * @return			Resolved zero-based floor index. 0始まりの階層番号です。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Returns the floor whose elevation is the highest one at or below the specified world Z coordinate. Heights below the first floor clamp to the first floor, and heights above the last floor clamp to the last floor."))
	int32 FindFloorHeight(const float z) const;

	/**
	 * Converts a world Z coordinate to a voxel Z coordinate relative to this dungeon actor.
	 * ワールドZ座標を、このダンジョンアクターを原点とするボクセルZ座標へ変換します。
	 * @param[in]	z	World Z coordinate. ワールドZ座標です。
	 * @return		Voxel Z coordinate rounded down to the containing grid. 所属グリッドへ切り下げたボクセルZ座標です。
	 */
	UFUNCTION(BlueprintCallable, Category = "DungeonGenerator", meta = (ToolTip = "Converts a world Z coordinate to a voxel Z coordinate relative to this dungeon actor and rounds down to the containing grid."))
	int32 FindVoxelHeight(const float z) const;


#if WITH_EDITOR
	/**
	 * Get CRC32 at generation
	 * 生成時のCRC32を取得します
	 */
	int32 GetGeneratedDungeonCRC32() const noexcept;
#endif

	/**
	 * Returns the horizontal world-unit size of one generated grid cell.
	 * 生成Grid Cell 1つ分の水平方向World Unit Sizeを返します。
	 */
	float GetGridSize() const;

	/**
	 * Returns the configured maximum room dimensions plus the requested grid-cell margin on every side.
	 * 設定された最大Room寸法へ、指定Grid Cell Marginを各辺に加えたSizeを返します。
	 */
	FVector GetRoomMaxSizeWithMargin(const int32_t margin) const;

	/**
	 * Applies a cull-distance override to generated ISM and HISM components in every spatial cluster.
	 * 全空間Clusterの生成ISM/HISM ComponentへCull距離Overrideを適用します。
	 */
	void SetInstancedMeshCullDistance(const FInt32Interval& cullDistance);

	/**
	 * Starts automatic authority generation or applies replicated state after components become available.
	 * Component利用可能後にAuthority自動生成、または複製済み状態の適用を開始します。
	 */
	virtual void PostInitializeComponents() override;

#if WITH_EDITOR
	/**
	 * Keeps editor viewport ticking available for generation debug visualization.
	 * 生成Debug表示のため、Editor ViewportでのTickを有効にします。
	 */
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// ADungeonGenerateBase overrides
	virtual void OnPreDungeonGeneration() override;
	virtual void OnPostDungeonGeneration(const bool result) override;
	virtual void Dispose(const bool flushStreamLevels) override;
	virtual void FinalizeGeneratedTerrain() override;

	static FIntVector CalculateInstancedMeshGroupCoordinate(const FVector& worldPosition, const FVector& actorLocation, const FVector& gridSize);
	void BeginInstanceTransaction();
	void AddInstance(UStaticMesh* staticMesh, const FTransform& transform, EDungeonMeshGenerationMethod meshGenerationMethod, bool affectsNavigation);
	void EndInstanceTransaction();
	void DestroyAllInstance();
	void ApplyInstancedMeshCullDistance();

	bool PreGenerateImplementation();
	void PostGenerateImplementation() const;
	void GenerateAuthoritative(UDungeonGenerateParameter* parameter);
	bool ApplyGenerationState(const FDungeonGenerationReplicatedState& generationState);

	/**
	 * Returns whether the parameter can be referenced over the network.
	 * パラメータをネットワーク越しに参照できるかを返します。
	 * @param[in]	parameter	判定するパラメータ
	 * @return		参照できるならtrue
	 */
	static bool IsParameterReplicable(const UDungeonGenerateParameter* parameter);

	/**
	 * Reports that the locally generated dungeon differs from the one the server generated.
	 * ローカルで生成したダンジョンがサーバーのものと異なる事を報告します。
	 * @param[in]	generationId	生成識別子
	 * @param[in]	serverCRC32		サーバーが計算したCRC
	 * @param[in]	localCRC32		ローカルで計算したCRC
	 * @param[in]	seed			生成に使用した乱数の種
	 */
	void ReportGenerationCrcMismatch(const uint64 generationId, const uint32 serverCRC32, const uint32 localCRC32, const int32 seed);

	/**
	 * Reports a parameter that clients cannot resolve over the network.
	 * クライアントがネットワーク越しに解決できないパラメータを報告します。
	 * @param[in]	parameter	配信するパラメータ
	 */
	void ReportNonReplicableParameter(const UDungeonGenerateParameter* parameter);
	void RefreshGeneratedWorldState() const;
	uint64 AllocateGenerationId() const;
	static int32 ResolveGenerationSeed(const UDungeonGenerateParameter* parameter);

	/**
	 * 直前に失敗した乱数の種から、再試行用の種を導出します
	 * @param[in]	previousSeed	直前に失敗した乱数の種
	 * @param[in]	attempt			何回目の再試行か
	 * @return		再試行用の乱数の種
	 */
	static int32 MakeRetryGenerationSeed(const int32 previousSeed, const int32 attempt);

#if WITH_EDITOR
	static const UDungeonGenerateParameter* ResolveParameterAssetSource(const UDungeonGenerateParameter* parameter);
	void CaptureFailedGenerationParameterSnapshot();
	void SavePendingFailedGenerationParameter();
	bool SaveFailedParameterAsAsset(UDungeonGenerateParameter* parameter, const UDungeonGenerateParameter* sourceAsset, int32 savedSeed);
#endif

	UFUNCTION()
	void OnRep_GenerationState();

#if WITH_EDITOR
	void DrawDebugInformation() const;
	void DrawFloorInformation(const dungeon::Generator& generator) const;
	void DrawVoxelGridInformation(const dungeon::Generator& generator) const;
	void ShowPerformanceStats() const;
	void HidePerformanceStats() const;
#endif

protected:
	/**
	 * Dungeon generation parameters
	 * ダンジョン生成パラメータ
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Parameter asset that defines structure, progression, gameplay, and visual content for generated dungeons."))
	TObjectPtr<UDungeonGenerateParameter> DungeonGenerateParameter;

#if WITH_EDITORONLY_DATA
	/**
	 * Generated random number seeds
	 * 生成時の乱数の種
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Generated random number seeds"))
	int32 GeneratedRandomSeed = 0;

	/**
	 * Generated dungeon hash
	 * 生成時のCRC32
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Generated dungeon hash"))
	int32 GeneratedDungeonCRC32 = 0;

#endif

	/**
	 * Represents AutoGenerateAtStart.
	 * レベル開始時にダンジョンを生成します
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Generates a dungeon at the start of the level"))
	bool AutoGenerateAtStart = true;

	/**
	 * Types of floors, ramps, and mezzanine meshes used for dungeon generation
	 * ダンジョン生成に使用する床、スロープ、中二階のメッシュの種類
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", DisplayName = "Dungeon Floor Slope Mesh Generation Method", meta = (ToolTip = "Component type used to generate floors, slopes, and catwalks. ISM and HISM reduce component cost and support NavMesh generation. Runtime-generated terrain requires RecastNavMesh Runtime Generation to be Dynamic."))
	EDungeonMeshGenerationMethod DungeonMeshGenerationMethod = EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh;

	/**
	 * Wall, ceiling, and pillar mesh types used for dungeon generation
	 * ダンジョン生成に使用する壁、天井、柱のメッシュの種類
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Component type used to generate walls, roofs, and pillars. Hierarchical instancing is recommended for large dungeons."))
	EDungeonMeshGenerationMethod DungeonWallRoofPillarMeshGenerationMethod = EDungeonMeshGenerationMethod::HierarchicalInstancedStaticMesh;


	/**
	 * build job tag
	 * ビルドジョブのタグ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail", meta = (ToolTip = "Build job tag embedded in this plugin build."))
	FString BuildJobTag;

	/**
	 * license tag
	 * ライセンスタグ
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail", meta = (ToolTip = "License tag embedded in this plugin build."))
	FString LicenseTag;

	/**
	 * License ID
	 * ライセンスID
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "DungeonGenerator|Detail", meta = (ToolTip = "License identifier embedded in this plugin build."))
	FString LicenseId;

#if WITH_EDITORONLY_DATA
	/**
	 * Displays the generated floor hierarchy as translucent horizontal sections.
	 * 生成された階層構造を半透明の水平断面として表示します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows every generated floor as a translucent horizontal section with its floor number, grid height, and world height in the editor and during PIE. Editor-only."))
	bool ShowFloorInformation = false;

	/**
	 * Displays voxel grid debugging information around the active camera.
	 * アクティブなカメラ周辺のボクセルグリッドのデバッグ情報を表示します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Draw voxel grid types and details near the active camera during PIE. Also follows the debug camera when ToggleDebugCamera is active. Editor-only."))
	bool ShowVoxelGridType = false;

	/**
	 * Displays basic performance statistics when play begins.
	 * プレイ開始時に基本的なパフォーマンス統計を表示します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DungeonGenerator|Debug", meta = (ToolTip = "Shows basic FPS and CPU performance stats on screen when PIE starts. Intended for development and testing; not used in Shipping builds."))
	bool bShowPerformanceStats = false;
#endif

	/**
	 * Location of the starting room of the dungeon
	 * PlayerStart location can be found in Get All Actors of Class
	 *
	 * ダンジョンのスタート部屋の位置
	 * PlayerStartの位置はGet All Actors of Classで検索して下さい
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Location of the starting room of the dungeon PlayerStart location can be found in Get All Actors of Class"))
	FVector StartRoomLocation;

	/**
	 * Location of the goal room in the dungeon
	 * ダンジョンのゴール部屋の位置
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator", meta = (ToolTip = "Location of the goal room in the dungeon"))
	FVector GoalRoomLocation;

private:
	/**
	 * Latest authoritative generation state, including the empty state after destruction.
	 * 破棄後の空状態を含む、最新のAuthority生成状態です。
	 */
	UPROPERTY(ReplicatedUsing = OnRep_GenerationState, Transient)
	FDungeonGenerationReplicatedState mReplicatedGenerationState;

	/**
	 * CRC the server reported for the revision being generated.
	 * 生成中のRevisionについてサーバーが報告したCRCです。
	 */
	uint32 mExpectedServerCRC32 = 0;

	/**
	 * Whether mExpectedServerCRC32 holds a CRC to verify against.
	 * mExpectedServerCRC32に突き合わせるCRCが入っているかどうかです。
	 */
	bool mHasExpectedServerCRC32 = false;

	/**
	 * Instanced mesh cull-distance override.
	 * インスタンスメッシュのカリング距離Overrideです。
	 */
	FInt32Interval mInstancedMeshCullDistance = { 0, 0};
	/**
	 * Latest revision already applied locally.
	 * ローカルで適用済みの最新Revisionです。
	 */
	uint64 mAppliedGenerationId = 0;

	/**
	 * Revision currently used to construct deterministic component names.
	 * 決定論的コンポーネント名の構築に使用中のRevisionです。
	 */
	uint64 mActiveGenerationId = 0;

	/**
	 * Seed currently used to construct deterministic component names.
	 * 決定論的コンポーネント名の構築に使用中のSeedです。
	 */
	int32 mActiveGenerationSeed = 0;

	/**
	 * Whether dungeon generation is currently running.
	 * ダンジョン生成処理中かを示します。
	 */
	bool mIsGeneratingDungeon = false;

	/**
	 * Whether actor components are ready for replicated state application.
	 * 複製状態を適用できるようActor Componentが初期化済みかを示します。
	 */
	bool mComponentsInitialized = false;

#if WITH_EDITORONLY_DATA
	/**
	 * Whether the active GenerateDungeonWithParameter call should save its parameter after failure.
	 * 実行中のGenerateDungeonWithParameterが失敗したParameterを保存するかを示します。
	 */
	bool mAutoSaveFailedGenerationParameter = false;

	/**
	 * Immutable parameter copy captured before failed-generation cleanup.
	 * 生成失敗後のCleanup前に取得した変更されないParameter Copyです。
	 */
	UPROPERTY(Transient, DuplicateTransient, NonPIEDuplicateTransient)
	TObjectPtr<UDungeonGenerateParameter> PendingFailedGenerationParameterSnapshot;

	/**
	 * Saved source asset used to choose the automatic failure-asset folder and name.
	 * 失敗Assetの自動保存Folderと名前を決める保存済み作成元Assetです。
	 */
	UPROPERTY(Transient)
	TObjectPtr<UDungeonGenerateParameter> PendingFailedGenerationParameterSourceAsset;

	/**
	 * Cached performance-stat visibility used by development automation tests.
	 * 開発用自動テストで使用するPerformance Stat表示状態のCacheです。
	 */
	bool mShowPerformanceStats = false;
#endif

	friend class ADungeonMainLevelScriptActor;

};
