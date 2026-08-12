/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#include "Parameter/Selector/DungeonSamplePartsSelector.h"

namespace
{
	/*
	 * Salts are used to split random streams by "selection phase".
	 * Even when SeedKey is the same, mesh-set selection and parts selection will produce
	 * different deterministic results because the salt value is different.
	 *
	 * Salt は「選択フェーズ」ごとに乱数系列を分離するために使います。
	 * SeedKey が同じでも、MeshSet 選択とパーツ選択で異なる決定的結果になるようにします。
	 */
	constexpr uint32 kMeshSetSalt = 0xA17E31C5u;
	constexpr uint32 kPartsSalt = 0x6D2B79F5u;

	uint32 MakeTargetSalt(const EDungeonPartsSelectorTarget target)
	{
		/*
		 * This prevents "same SeedKey => same index pattern" across unrelated targets.
		 *
		 * Target（Floor / Wall / Roof など）ごとに、さらに決定的な系列分離を追加します。
		 * これにより、同じ SeedKey でも無関係な Target 間で同じ選択パターンになりにくくします。
		 */
		return static_cast<uint32>(target) * 0x9E3779B9u;
	}
}

uint32 UDungeonSamplePartsSelector::HashSeed(const int32 SeedKey, const uint32 Salt)
{
	/*
	 * Small deterministic integer hash (fast and stable across platforms).
	 *
	 * Why not use FMath::Rand / global RNG?
	 * - This callback runs in the generation hot path.
	 * - We want reproducible results from Query.SeedKey only.
	 * - Global / unsynchronized randomness can cause server/client divergence.
	 *
	 * 小さく軽量な決定的整数ハッシュです（高速で、プラットフォーム間で安定）。
	 *
	 * FMath::Rand / グローバル RNG を使わない理由:
	 * - このコールバックは生成処理のホットパスで呼ばれます。
	 * - Query.SeedKey のみから再現可能な結果を得たいからです。
	 * - グローバル/非同期な乱数はサーバー・クライアント差異の原因になります。
	 */
	uint32 x = static_cast<uint32>(SeedKey) ^ Salt;
	x ^= (x >> 16);
	x *= 0x7FEB352Du;
	x ^= (x >> 15);
	x *= 0x846CA68Bu;
	x ^= (x >> 16);
	return x;
}

int32 UDungeonSamplePartsSelector::PickDeterministicWeightedIndex(const TArray<FDungeonSampleSelectorWeightedIndex>& Candidates, const int32 NumCandidates, const int32 SeedKey, const uint32 Salt)
{
	/*
	 * Returning INDEX_NONE tells the generator to use its built-in fallback selection.
	 * This sample intentionally uses INDEX_NONE for invalid or empty data so users can
	 * configure the selector incrementally without hard failures.
	 *
	 * INDEX_NONE を返すと、ジェネレータ側の組み込みフォールバック選択が使われます。
	 * このサンプルでは、無効/空データ時に意図的に INDEX_NONE を返し、
	 * 利用者が段階的に設定しても破綻しにくいようにしています。
	 */
	if (NumCandidates <= 0 || Candidates.Num() <= 0)
		return INDEX_NONE;

	int32 totalWeight = 0;
	for (const FDungeonSampleSelectorWeightedIndex& candidate : Candidates)
	{
		/*
		 * Ignore invalid entries so designers can leave placeholders in arrays.
		 *
		 * 無効な要素は無視します。配列にプレースホルダが残っていても動作するようにするためです。
		 */
		if (candidate.Weight <= 0)
			continue;
		if (candidate.Index < 0 || candidate.Index >= NumCandidates)
			continue;

		totalWeight += candidate.Weight;
	}

	if (totalWeight <= 0)
		return INDEX_NONE;

	/*
	 * Deterministic weighted draw:
	 * 1) Hash (SeedKey + Salt)
	 * 2) Fold into [0, TotalWeight)
	 * 3) Walk candidate weights
	 *
	 * 決定的な重み付き抽選の手順:
	 * 1) (SeedKey + Salt) をハッシュ化
	 * 2) [0, TotalWeight) の範囲に折りたたむ
	 * 3) 候補の重みを順に走査して決定
	 */
	const uint32 hash = HashSeed(SeedKey, Salt);
	int32 draw = static_cast<int32>(hash % static_cast<uint32>(totalWeight));

	for (const FDungeonSampleSelectorWeightedIndex& candidate : Candidates)
	{
		if (candidate.Weight <= 0)
			continue;
		if (candidate.Index < 0 || candidate.Index >= NumCandidates)
			continue;

		if (draw < candidate.Weight)
			return candidate.Index;

		draw -= candidate.Weight;
	}

	return INDEX_NONE;
}

bool UDungeonSamplePartsSelector::IsNeighborRuleMatch(const FDungeonSampleNeighborMaskPartsRule& Rule, const FDungeonPartsQuery& Query)
{
	/*
	 * Rules are matched against both target and NeighborMask6.
	 * Example:
	 * - Target=Wall, RequiredBits = North, ForbiddenBits = South
	 *   => "north is occupied and south is open" style routing.
	 *
	 * ルールは Target と NeighborMask6 の両方で判定します。
	 * 例:
	 * - Target=Wall, RequiredBits = North, ForbiddenBits = South
	 *   => 「北が埋まっていて南が開いている」条件の分岐
	 */
	if (Rule.Target != Query.Target)
		return false;

	/*
	 * Required bits must be ON.
	 *
	 * RequiredBits に含まれるビットはすべて ON である必要があります。
	 */
	if ((Query.NeighborMask6 & Rule.RequiredBits) != Rule.RequiredBits)
		return false;

	/*
	 * Forbidden bits must be OFF.
	 *
	 * ForbiddenBits に含まれるビットはすべて OFF である必要があります。
	 */
	if ((Query.NeighborMask6 & Rule.ForbiddenBits) != 0)
		return false;

	return true;
}

const TArray<FDungeonSampleSelectorWeightedIndex>& UDungeonSamplePartsSelector::GetPartsFallbackCandidates(const EDungeonPartsSelectorTarget Target) const
{
	/*
	 * Per-target fallback lets users tune Floor/Wall/Roof independently even when
	 * no neighbor-mask rule is matched.
	 *
	 * NeighborMask6 のルールに一致しない場合でも、Target ごとのフォールバックで
	 * Floor / Wall / Roof を個別に調整できるようにしています。
	 */
	switch (Target)
	{
	case EDungeonPartsSelectorTarget::Floor:
		return FloorFallbackCandidates;
	case EDungeonPartsSelectorTarget::Wall:
		return WallFallbackCandidates;
	case EDungeonPartsSelectorTarget::Roof:
		return RoofFallbackCandidates;
	case EDungeonPartsSelectorTarget::Slope:
		return SlopeFallbackCandidates;
	case EDungeonPartsSelectorTarget::Catwalk:
		return CatwalkFallbackCandidates;
	case EDungeonPartsSelectorTarget::Pillar:
		return PillarFallbackCandidates;
	case EDungeonPartsSelectorTarget::Torch:
		return TorchFallbackCandidates;
	case EDungeonPartsSelectorTarget::Chandelier:
		return ChandelierFallbackCandidates;
	case EDungeonPartsSelectorTarget::Door:
	case EDungeonPartsSelectorTarget::UniqueDoor:
		return DoorFallbackCandidates;
	default:
		return FloorFallbackCandidates;
	}
}

int32 UDungeonSampleMeshSetSelector::SelectMeshSetIndex_Implementation(const FDungeonMeshSetQuery& Query, int32 NumCandidates) const
{
	/*
	 * This is called by UDungeonMeshSetDatabase when SelectionPolicy == CustomSelector.
	 * The function must be lightweight and deterministic.
	 *
	 * この関数は、UDungeonMeshSetDatabase の SelectionPolicy が CustomSelector のときに呼ばれます。
	 * 軽量かつ決定的（同じ入力で同じ結果）である必要があります。
	 */
	if (NumCandidates <= 0)
		return INDEX_NONE;

	/*
	 * 1) Deterministic weighted lottery for mesh-set candidates.
	 *    This sample keeps mesh-set selection simple and uses SeedKey only.
	 *    (Topology / target-specific branching is demonstrated in SelectPartsIndex.)
	 *
	 * 1) MeshSet 候補に対する決定的な重み付き抽選です。
	 *    このサンプルでは MeshSet 選択はシンプルに保ち、SeedKey のみを使います。
	 *    （形状や対象ごとの分岐は SelectPartsIndex 側で実演します。）
	 */
	const int32 fallback = UDungeonSamplePartsSelector::PickDeterministicWeightedIndex(MeshSetFallbackCandidates, NumCandidates, Query.SeedKey, kMeshSetSalt ^ 0x13579BDFu);
	if (fallback != INDEX_NONE)
		return fallback;

	/*
	 * 2) Let the generator use its built-in selection for the configured policy fallback.
	 *
	 * 2) このサンプルで選べない場合は、ジェネレータの組み込み選択へ処理を戻します。
	 */
	return INDEX_NONE;
}

int32 UDungeonSamplePartsSelector::SelectPartsIndex_Implementation(const FDungeonPartsQuery& Query, int32 NumCandidates) const
{
	/*
	 * This is called when a mesh-set part policy / fixture policy is CustomSelector.
	 * The same selector class can handle Floor/Wall/Roof/... because Query.Target tells
	 * us what is currently being selected.
	 *
	 * この関数は、MeshSet 内パーツ選択 / Fixture 選択の Policy が CustomSelector のときに呼ばれます。
	 * Query.Target で現在の選択対象が分かるため、1つの selector クラスで
	 * Floor / Wall / Roof / ... をまとめて扱えます。
	 */
	if (NumCandidates <= 0)
		return INDEX_NONE;

	/*
	 * 1) NeighborMask6-based routing.
	 *    This is useful for shape-aware variations, for example:
	 *    - wall cap when top is open
	 *    - corner variant when N/E are occupied
	 *    - isolated tile variant when NSEW are all open/closed
	 *
	 *    Rule order matters: first matching rule is attempted first.
	 *
	 * 1) NeighborMask6 ベースの振り分けです。
	 *    形状に応じたバリエーション分岐に向いています。例:
	 *    - 上側が開いているときの wall cap
	 *    - N/E が埋まっているときの corner variant
	 *    - NSEW が全開/全閉のときの isolated tile variant
	 *
	 *    ルール順序は重要で、最初に一致したルールから順に試行します。
	 */
	for (const FDungeonSampleNeighborMaskPartsRule& rule : PartsRulesByNeighborMask)
	{
		if (!IsNeighborRuleMatch(rule, Query))
			continue;

		/*
		 * Include target + NeighborMask6 in the salt so each topology pattern can produce
		 * a different deterministic distribution while still being stable per SeedKey.
		 *
		 * target と NeighborMask6 を Salt に混ぜることで、
		 * トポロジー（接続パターン）ごとに異なる決定的分布を作りつつ、
		 * SeedKey に対する安定性は維持します。
		 */
		const uint32 salt = kPartsSalt ^ MakeTargetSalt(Query.Target) ^ static_cast<uint32>(Query.NeighborMask6);
		const int32 picked = PickDeterministicWeightedIndex(rule.Candidates, NumCandidates, Query.SeedKey, salt);
		if (picked != INDEX_NONE)
			return picked;

		/*
		 * Rule matched but candidates were invalid/out-of-range.
		 * Continue searching later rules, then fallback candidates.
		 *
		 * ルール自体は一致したが、候補が無効または範囲外でした。
		 * 後続ルール、最後にフォールバック候補を続けて探索します。
		 */
	}

	/*
	 * 2) Per-target fallback weighted lottery.
	 *    Keeps behavior customizable even without neighbor-specific rules.
	 *
	 * 2) Target ごとのフォールバック重み付き抽選です。
	 *    NeighborMask6 の個別ルールがなくても、Target 単位で挙動を調整できます。
	 */
	const TArray<FDungeonSampleSelectorWeightedIndex>& fallbackCandidates = GetPartsFallbackCandidates(Query.Target);
	const int32 fallback = PickDeterministicWeightedIndex(fallbackCandidates, NumCandidates, Query.SeedKey, kPartsSalt ^ MakeTargetSalt(Query.Target));
	if (fallback != INDEX_NONE)
		return fallback;

	/*
	 * 3) If no sample rule can choose, allow the generator's built-in selector to run.
	 *
	 * 3) サンプルルールで選べない場合は、ジェネレータの組み込み selector に処理を委譲します。
	 */
	return INDEX_NONE;
}
