/**
@author		Shun Moriya
@copyright	2025- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <Math/Interval.h>
#include "DungeonInstancedMeshCluster.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * UInstancedStaticMeshComponentまたはUHierarchicalInstancedStaticMeshComponentのクラスターを管理します
 */
USTRUCT()
struct FDungeonInstancedMeshCluster final
{
	GENERATED_BODY()

public:
	/**
	 * 大量に登録する開始処理
	 * FoliageInstancedStaticMeshComponentのツリー生成などを抑制します
	 */
	void BeginTransaction();

	/**
	 * 指定アクターに指定メッシュが含まれているか検索します。
	 * 含まれていない場合はInstancedStaticMeshComponentを生成してメッシュを登録します
	 *
	 * @param[inout]	actor			検索先、登録先アクター
	 * @param[in]		staticMesh		検索または登録するメッシュコンポーネント
	 * @return			発見または生成したInstancedStaticMeshComponent
	 */
	UInstancedStaticMeshComponent* FindOrCreateInstance(AActor* actor, UStaticMesh* staticMesh);

	/**
	 * 指定アクターに指定メッシュが含まれているか検索します。
	 * 含まれていない場合はHierarchicalInstancedStaticMeshComponentを生成してメッシュを登録します
	 *
	 * @param[inout]	actor			検索先、登録先アクター
	 * @param[in]		staticMesh		検索または登録するメッシュコンポーネント
	 * @return			発見または生成したInstancedStaticMeshComponent
	 */
	UHierarchicalInstancedStaticMeshComponent* FindOrCreateHierarchicalInstance(AActor* actor, UStaticMesh* staticMesh);

	/**
	 * 大量に登録する終了処理
	 * FoliageInstancedStaticMeshComponentのツリー再構築を要求します
	 */
	void EndTransaction();

	/**
	 * 全てを破棄する
	 */
	void DestroyAll();

	/**
	 * カリング距離を設定します
	 */
	void SetCullDistance(const FInt32Interval& cullDistances);

protected:
	/**
	 * 登録するInstancedStaticMeshComponentまたはHierarchicalInstancedStaticMeshComponent
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> mComponents;
};
