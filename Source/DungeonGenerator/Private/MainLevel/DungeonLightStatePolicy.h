/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include <CoreMinimal.h>

namespace dungeon::LightStatePolicy
{
	/**
	 * Computes the facing dot from the configured light-facing direction toward the camera.
	 * 設定されたライト正面方向とカメラ方向から正面判定用の内積を計算します。
	 */
	inline float ComputeActorFacingDot(const FVector& lightFacingDirection, const FVector& fromLightToCamera) noexcept
	{
		const FVector facingDirection = lightFacingDirection.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
		const FVector directionToCamera = fromLightToCamera.GetSafeNormal(SMALL_NUMBER, facingDirection);
		return FMath::Clamp(FVector::DotProduct(facingDirection, directionToCamera), -1.f, 1.f);
	}

	/**
	 * Normalizes the light-facing hysteresis angles and clamps them to a valid range.
	 * ライト正面判定のヒステリシス角度を正規化し、有効な範囲に制限します。
	 */
	inline void NormalizeFacingAngles(const float angle0, const float angle1, float& turnOnAngle, float& turnOffAngle) noexcept
	{
		turnOnAngle = FMath::Clamp(FMath::Min(angle0, angle1), 0.f, 180.f);
		turnOffAngle = FMath::Clamp(FMath::Max(angle0, angle1), 0.f, 180.f);
	}

	/**
	 * Resolves the managed light state from identifier priority and facing-angle hysteresis.
	 * Identifierの優先判定と正面角度のヒステリシスから管理対象ライトの状態を決定します。
	 */
	inline bool ResolveManagerEnabled(
		const bool identifierMatches,
		const float facingDot,
		const bool previouslyEnabled,
		const float configuredTurnOnAngle,
		const float configuredTurnOffAngle) noexcept
	{
		if (identifierMatches)
			return true;

		float turnOnAngle;
		float turnOffAngle;
		NormalizeFacingAngles(configuredTurnOnAngle, configuredTurnOffAngle, turnOnAngle, turnOffAngle);
		if (facingDot >= FMath::Cos(FMath::DegreesToRadians(turnOnAngle)))
			return true;
		if (facingDot <= FMath::Cos(FMath::DegreesToRadians(turnOffAngle)))
			return false;
		return previouslyEnabled;
	}

	/**
	 * Returns whether ShowLight.
	 * 全ての表示条件を適用した後に管理対象ライトを表示できるか返します。
	 */
	inline bool ShouldShowLight(
		const bool partitionActive,
		const bool initiallyVisible,
		const bool activatorVisibilityEnabled,
		const bool managerEnabled) noexcept
	{
		return partitionActive && initiallyVisible && activatorVisibilityEnabled && managerEnabled;
	}

	/**
	 * Computes the shadow priority score from the camera direction and distance to the light.
	 * カメラの正面方向とライトまでの距離から、影の優先スコアを計算します。
	 */
	inline float ComputeCameraPriorityScore(const FVector& cameraDirection, const FVector& toLight) noexcept
	{
		const float distance = toLight.Size();
		if (FMath::IsNearlyZero(distance))
			return TNumericLimits<float>::Max();

		const float facingDot = FMath::Clamp(
			FVector::DotProduct(cameraDirection.GetSafeNormal(), toLight / distance),
			-1.f,
			1.f
		);
		const float directionFactor = facingDot + 1.f;
		return directionFactor / FMath::Sqrt(distance);
	}

	/**
	 * Represents ResolveShadowCastingCount.
	 * 影を落とせる候補数を返します。上限の0は無制限を表します。
	 */
	inline size_t ResolveShadowCastingCount(const uint8 maximumShadowCastingLights, const size_t candidateCount) noexcept
	{
		return maximumShadowCastingLights == 0
			? candidateCount
			: FMath::Min(static_cast<size_t>(maximumShadowCastingLights), candidateCount);
	}

	/**
	 * Rendering state selected for one sorted light candidate by the shadow budget.
	 * 影予算によって、ソート済みライト候補1つに選択される描画状態です。
	 */
	struct FShadowBudgetState final
	{
		/**
		 * Whether the Light Component should be visible.
		 * Light Componentを表示するかを表します。
		 */
		bool Visible = false;

		/**
		 * Whether the Light Component should retain shadow casting while visible or fading out.
		 * Light Componentの表示中またはフェードアウト中に影生成を維持するかを表します。
		 */
		bool CastShadows = true;
	};

	/**
	 * Resolves visibility and shadow casting for a sorted candidate without allowing a visible shadow-free state.
	 * 表示中の影なし状態を許可せず、ソート済み候補の表示と影生成を決定します。
	 */
	inline FShadowBudgetState ResolveShadowBudgetState(const size_t candidateIndex, const size_t shadowCastingCount) noexcept
	{
		return FShadowBudgetState{ candidateIndex < shadowCastingCount, true };
	}

	/**
	 * Returns whether the left light has higher shadow priority, comparing camera score before identifier match.
	 * カメラスコア、Identifier一致の順に比較し、左のライトが高い影優先度を持つか返します。
	 */
	inline bool HasHigherPriority(
		const bool leftIdentifierMatches,
		const float leftScore,
		const bool rightIdentifierMatches,
		const float rightScore) noexcept
	{
		if (leftScore != rightScore)
			return leftScore > rightScore;
		return leftIdentifierMatches && !rightIdentifierMatches;
	}
}
