/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>
#include "Parameter/Selector/DungeonBlueprintMeshSetSelector.h"
#include "Parameter/Selector/DungeonBlueprintPartsSelector.h"
#include "DungeonSamplePartsSelector.generated.h"


/*
 * Deterministic weighted candidate for custom selector samples.
 * Uses Query.SeedKey for reproducible picks (server/client safe if the same data is used).
 *
 * カスタムセレクタサンプル用の決定論的加重候補です。
 * 再現性のある選択のために Query.SeedKey を使用します（同一データ使用時、サーバー/クライアント双方で安全です）。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonSampleSelectorWeightedIndex
{
	GENERATED_BODY()

	/*
	 * Candidate index returned to the generator.
	 *
	 * ジェネレータへ返す候補インデックスです。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	int32 Index = 0;

	/*
	 * Relative weight (must be > 0 to participate).
	 *
	 * 相対重みです（抽選に参加するには 0 より大きい必要があります）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0"))
	int32 Weight = 1;
};

/*
 * Parts selection rule keyed by FDungeonPartsQuery::Target + FDungeonPartsQuery::NeighborMask6.
 * NeighborMask6 bit layout:
 *  bit0 = North, bit1 = East, bit2 = South, bit3 = West, bit4 = Floor, bit5 = Ceiling
 *
 * FDungeonPartsQuery::Target + FDungeonPartsQuery::NeighborMask6 をキーにしたパーツ選択ルールです。
 * NeighborMask6 のビット配置:
 *  bit0 = North, bit1 = East, bit2 = South, bit3 = West, bit4 = Floor, bit5 = Ceiling
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonSampleNeighborMaskPartsRule
{
	GENERATED_BODY()

	/*
	 * Which parts target this rule applies to.
	 *
	 * このルールを適用するパーツ対象です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	EDungeonPartsSelectorTarget Target = EDungeonPartsSelectorTarget::Floor;

	/*
	 * All bits in this mask must be ON.
	 *
	 * このマスクに含まれるビットはすべて ON である必要があります。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "63"))
	uint8 RequiredBits = 0;

	/*
	 * All bits in this mask must be OFF.
	 *
	 * このマスクに含まれるビットはすべて OFF である必要があります。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator", meta = (ClampMin = "0", ClampMax = "63"))
	uint8 ForbiddenBits = 0;

	/*
	 * Deterministic weighted part candidates.
	 *
	 * 決定的な重み付きパーツ候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator")
	TArray<FDungeonSampleSelectorWeightedIndex> Candidates;
};


class UDungeonSampleMeshSetSelector;

/*
 * C++ sample custom selector for plugin users.
 * Demonstrates:
 * - MeshSet selection by deterministic weighted lottery (SeedKey)
 * - Mesh parts selection by NeighborMask6
 * - Deterministic weighted lottery using SeedKey
 *
 * プラグイン利用者向けの C++ カスタムセレクタサンプルです。
 * 以下を実演します:
 * - SeedKey による決定的な重み付き MeshSet 選択
 * - NeighborMask6 によるメッシュパーツ選択
 * - SeedKey を用いた決定的な重み付き抽選
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonSamplePartsSelector : public UDungeonBlueprintPartsSelector
{
	GENERATED_BODY()

public:
	virtual int32 SelectPartsIndex_Implementation(const FDungeonPartsQuery& Query, int32 NumCandidates) const override;

protected:
	/*
	 * Mesh-set weighted candidates used for deterministic selection.
	 *
	 * MeshSet を決定的に選択するための重み付き候補です。
	 */
	/*
	 * Neighbor-mask rules for parts selection. First matching rule is used.
	 *
	 * パーツ選択用の NeighborMask ルールです。最初に一致したルールを使用します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleNeighborMaskPartsRule> PartsRulesByNeighborMask;

	/*
	 * Optional per-target fallback candidates when no neighbor rule matches.
	 *
	 * Neighbor ルールに一致しない場合の、Target ごとのフォールバック候補（任意）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> FloorFallbackCandidates;

	/**
	 * Fallback wall candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使う壁のフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> WallFallbackCandidates;

	/**
	 * Fallback roof candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使う屋根のフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> RoofFallbackCandidates;

	/**
	 * Fallback slope candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使うスロープのフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> SlopeFallbackCandidates;

	/**
	 * Fallback catwalk candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使うキャットウォークのフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> CatwalkFallbackCandidates;

	/**
	 * Fallback pillar candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使う柱のフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> PillarFallbackCandidates;

	/**
	 * Fallback torch candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使うたいまつのフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> TorchFallbackCandidates;

	/**
	 * Fallback chandelier candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使うシャンデリアのフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> ChandelierFallbackCandidates;

	/**
	 * Fallback door candidates used when policy-based selection yields no match.
	 *
	 * ポリシー選択で一致がない場合に使うドアのフォールバック候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|Parts")
	TArray<FDungeonSampleSelectorWeightedIndex> DoorFallbackCandidates;

private:
	static uint32 HashSeed(int32 SeedKey, uint32 Salt);
	static int32 PickDeterministicWeightedIndex(const TArray<FDungeonSampleSelectorWeightedIndex>& Candidates, int32 NumCandidates, int32 SeedKey, uint32 Salt);
	static bool IsNeighborRuleMatch(const FDungeonSampleNeighborMaskPartsRule& Rule, const FDungeonPartsQuery& Query);
	const TArray<FDungeonSampleSelectorWeightedIndex>& GetPartsFallbackCandidates(const EDungeonPartsSelectorTarget Target) const;
	friend class UDungeonSampleMeshSetSelector;
};

/*
 * C++ sample mesh-set selector for plugin users.
 * プラグイン利用者向けの C++ メッシュセットセレクターサンプルです。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOR_API UDungeonSampleMeshSetSelector : public UDungeonBlueprintMeshSetSelector
{
	GENERATED_BODY()

public:
	virtual int32 SelectMeshSetIndex_Implementation(const FDungeonMeshSetQuery& Query, int32 NumCandidates) const override;

protected:
	/*
	 * Mesh-set weighted candidates used for deterministic selection.
	 * MeshSet を決定論的に選択するための重み付き候補です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DungeonGenerator|Sample|MeshSet")
	TArray<FDungeonSampleSelectorWeightedIndex> MeshSetFallbackCandidates;
};


