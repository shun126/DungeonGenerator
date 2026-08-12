/**
 * @author      Shun Moriya
 * @copyright   2025- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <Math/Box.h>
#include <Math/Interval.h>
#include "DungeonInstancedMeshCluster.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UFoliageInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * Identifies independently managed component lifecycles inside one spatial cluster.
 * 1つの空間クラスター内で個別管理するコンポーネントのライフサイクルを識別します。
 */
enum class EDungeonInstancedMeshCategory : uint8
{
	GeneratedMesh,
	Vegetation
};

/**
 * Owns and operates the ISM, HISM, and Foliage HISM components in one spatial cluster.
 * 1つの空間クラスターに属するISM、HISM、Foliage HISMコンポーネントを所有して操作します。
 */
USTRUCT()
struct FDungeonInstancedMeshCollection final
{
	GENERATED_BODY()

public:
	/**
	 * Begins a batched update for one component category.
	 * 1種類のコンポーネントカテゴリーに対する一括更新を開始します。
	 */
	void BeginTransaction(EDungeonInstancedMeshCategory category);

	/**
	 * Queues a world-space transform for an ISM component.
	 * ISMコンポーネントへワールド空間Transformを一括登録用として保留します。
	 * @param affectsNavigation Whether the component contributes geometry to NavMesh generation. NavMesh生成へ形状を提供するかどうかです。
	 */
	void AddInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, uint64 generationId, int32 seed, const FIntVector& groupCoordinate, bool affectsNavigation);

	/**
	 * Queues a world-space transform for an HISM component.
	 * HISMコンポーネントへワールド空間Transformを一括登録用として保留します。
	 * @param affectsNavigation Whether the component contributes geometry to NavMesh generation. NavMesh生成へ形状を提供するかどうかです。
	 */
	void AddHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, uint64 generationId, int32 seed, const FIntVector& groupCoordinate, bool affectsNavigation);

	/**
	 * Adds one Foliage HISM instance immediately.
	 * Foliage HISMインスタンスを1つ即時追加します。
	 */
	bool AddFoliageInstance(AActor* actor, UStaticMesh* staticMesh, const FTransform& transform, const FInt32Interval& cullDistances);

	/**
	 * Finds or creates an ISM component with deterministic identity.
	 * 決定的な識別情報を持つISMコンポーネントを検索または生成します。
	 * @param affectsNavigation Navigation identity included in component lookup and naming. Component検索と名前へ含めるNavigation属性です。
	 */
	UInstancedStaticMeshComponent* FindOrCreateInstance(AActor* actor, UStaticMesh* staticMesh, uint64 generationId, int32 seed, const FIntVector& groupCoordinate, bool affectsNavigation);

	/**
	 * Finds or creates an HISM component with deterministic identity.
	 * 決定的な識別情報を持つHISMコンポーネントを検索または生成します。
	 * @param affectsNavigation Navigation identity included in component lookup and naming. Component検索と名前へ含めるNavigation属性です。
	 */
	UHierarchicalInstancedStaticMeshComponent* FindOrCreateHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh, uint64 generationId, int32 seed, const FIntVector& groupCoordinate, bool affectsNavigation);

	/**
	 * Finalizes a batched update for one component category.
	 * 1種類のコンポーネントカテゴリーに対する一括更新を確定します。
	 */
	void EndTransaction(EDungeonInstancedMeshCategory category);

	/**
	 * Immediately removes generated-mesh components from navigation generation without destroying them; other categories are ignored.
	 * 生成メッシュコンポーネントを破棄せずナビゲーション生成対象から即時解除し、それ以外のカテゴリーは無視します。
	 */
	void DisableNavigation(EDungeonInstancedMeshCategory category);

	/**
	 * Rebuilds Foliage HISM trees until the frame budget is exhausted.
	 * フレーム予算が尽きるまでFoliage HISMツリーを再構築します。
	 */
	bool BuildFoliageTreesBudgeted(int32& nextComponentIndex, int32& remainingBuildCount, double startSecond, double maxTimeSecond);

	/**
	 * Immediately rebuilds every Foliage HISM tree and optionally keeps batching active.
	 * 全Foliage HISM Treeを即時再構築し、必要に応じて一括更新を継続します。
	 */

	/**
	 * Destroys every component in one category.
	 * 1種類のカテゴリーに属する全コンポーネントを破棄します。
	 */
	void Destroy(EDungeonInstancedMeshCategory category);

	/**
	 * Destroys every managed component.
	 * 管理する全コンポーネントを破棄します。
	 */
	void DestroyAll();

	/**
	 * Sets Cull Distance on generated ISM and HISM components without changing Foliage settings.
	 * Foliage設定を変更せず生成ISM/HISMコンポーネントへCull Distanceを設定します。
	 */
	void SetGeneratedMeshCullDistance(const FInt32Interval& cullDistances);

	/**
	 * Returns the world-space bounds occupied by one category.
	 * 1種類のカテゴリーが占有するワールド空間境界を返します。
	 */
	const FBox& GetInstanceBounds(EDungeonInstancedMeshCategory category) const noexcept;

	/**
	 * Returns the union of generated-mesh and vegetation instance bounds.
	 * 生成メッシュと植生のインスタンス境界の和集合を返します。
	 */
	FBox GetCombinedInstanceBounds() const noexcept;

	/**
	 * Shifts cached world-space bounds by a world offset.
	 * キャッシュ済みワールド空間境界をワールドオフセットで平行移動します。
	 */
	void ShiftWorldOffset(const FVector& delta);

	/**
	 * Applies partition-controlled visibility to all components.
	 * 全コンポーネントへパーティション制御の表示状態を適用します。
	 */
	void SetPartitionVisible(bool visible);

	/**
	 * Returns whether partition load control allows this collection to be visible.
	 * パーティション負荷制御がこのCollectionの表示を許可しているか返します。
	 */
	bool IsPartitionVisible() const noexcept;

	/**
	 * Returns whether one category contains no components or pending transforms.
	 * 1種類のカテゴリーにコンポーネントと保留Transformが無いか返します。
	 */
	bool IsEmpty(EDungeonInstancedMeshCategory category) const noexcept;

private:
	UFoliageInstancedStaticMeshComponent* FindOrCreateFoliageInstance(AActor* actor, UStaticMesh* staticMesh, const FInt32Interval& cullDistances);
	void ExpandInstanceBounds(EDungeonInstancedMeshCategory category, UStaticMesh* staticMesh, const FTransform& transform);
	void ResumeVegetationTransaction();
	static bool IsComponentInCategory(const UInstancedStaticMeshComponent* component, EDungeonInstancedMeshCategory category);

	/**
	 * ISM, HISM, and Foliage HISM components owned by this collection.
	 * このCollectionが所有するISM、HISM、Foliage HISMコンポーネントです。
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> mComponents;

	/**
	 * World-space transforms queued for generated-mesh batch updates.
	 * 生成メッシュの一括更新用として保留しているワールド空間Transformです。
	 */
	TMap<UInstancedStaticMeshComponent*, TArray<FTransform>> mPendingTransforms;

	/**
	 * Cached Foliage component lookup by StaticMesh.
	 * StaticMesh別のFoliageコンポーネント検索キャッシュです。
	 */
	TMap<UStaticMesh*, UFoliageInstancedStaticMeshComponent*> mFoliageComponentByStaticMesh;

	/**
	 * World-space bounds occupied by generated ISM and HISM instances.
	 * 生成ISM/HISMインスタンスが占有するワールド空間境界です。
	 */
	FBox mGeneratedMeshInstanceBounds = FBox(EForceInit::ForceInit);

	/**
	 * World-space bounds occupied by Foliage HISM instances.
	 * Foliage HISMインスタンスが占有するワールド空間境界です。
	 */
	FBox mVegetationInstanceBounds = FBox(EForceInit::ForceInit);

	/**
	 * Whether generated-mesh batching is active.
	 * 生成メッシュの一括登録が有効か示します。
	 */
	bool mGeneratedMeshTransactionActive = false;

	/**
	 * Whether vegetation batching is active.
	 * 植生の一括登録が有効か示します。
	 */
	bool mVegetationTransactionActive = false;

	/**
	 * Visibility state contributed by partition load control.
	 * パーティション負荷制御による表示状態です。
	 */
	bool mPartitionVisible = true;

};

/**
 * Represents one partition-linked spatial group containing generated meshes and vegetation state.
 * 生成メッシュと植生状態を含むパーティション連携済み空間グループを表します。
 */
USTRUCT()
struct FDungeonInstancedMeshCluster final
{
	GENERATED_BODY()

public:
	/**
	 * Returns the managed component collection.
	 * 管理対象のコンポーネントCollectionを返します。
	 */
	FDungeonInstancedMeshCollection& GetInstances() noexcept;

	/**
	 * Returns the managed component collection.
	 * 管理対象のコンポーネントCollectionを返します。
	 */
	const FDungeonInstancedMeshCollection& GetInstances() const noexcept;


	/**
	 * Returns actual and provisional bounds used for partition linking.
	 * パーティション連携に使用する実境界と暫定境界を返します。
	 */
	FBox GetPartitionBounds() const noexcept;

	/**
	 * Applies partition-controlled visibility to every component in the cluster.
	 * クラスター内の全コンポーネントへパーティション制御の表示状態を適用します。
	 */
	void SetPartitionVisible(bool visible);

	/**
	 * Returns whether partition load control allows this cluster to be visible.
	 * パーティション負荷制御がこのクラスターの表示を許可しているか返します。
	 */
	bool IsPartitionVisible() const noexcept;

	/**
	 * Shifts actual and provisional bounds by a world offset.
	 * 実境界と暫定境界をワールドオフセットで平行移動します。
	 */
	void ShiftWorldOffset(const FVector& delta);

	/**
	 * Returns whether the cluster contains no components or deferred work.
	 * コンポーネントと遅延処理が存在しないか返します。
	 */
	bool IsEmpty() const noexcept;

private:
	/**
	 * Component storage and operations shared by generated meshes and vegetation.
	 * 生成メッシュと植生が共有するコンポーネント格納・操作機能です。
	 */
	UPROPERTY(Transient)
	FDungeonInstancedMeshCollection mInstances;

};

inline FDungeonInstancedMeshCollection& FDungeonInstancedMeshCluster::GetInstances() noexcept
{
	return mInstances;
}

inline const FDungeonInstancedMeshCollection& FDungeonInstancedMeshCluster::GetInstances() const noexcept
{
	return mInstances;
}


inline bool FDungeonInstancedMeshCluster::IsPartitionVisible() const noexcept
{
	return mInstances.IsPartitionVisible();
}
