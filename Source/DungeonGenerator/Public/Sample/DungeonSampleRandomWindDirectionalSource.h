/**
 * @author		Shun Moriya
 * @copyright	2026- Shun Moriya
 * All Rights Reserved.
 */

 #pragma once
#include <Engine/WindDirectionalSource.h>
#include "DungeonSampleRandomWindDirectionalSource.generated.h"

/*
 * Wind directional source that randomly changes wind strength, speed, yaw, and pitch over time for the sample map.
 * サンプルマップ用に、風の強さ、速度、ヨー角、ピッチ角を時間経過でランダムに変化させるDirectional Wind Sourceです。
 */
UCLASS(meta = (ToolTip = "Sample wind actor that randomly changes wind strength, speed, yaw, and pitch over time."))
class DUNGEONGENERATOR_API ADungeonSampleRandomWindDirectionalSource : public AWindDirectionalSource
{
	GENERATED_BODY()

protected:
	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	 * Minimum time in seconds before choosing the next random wind target.
	 * 次のランダムな風の目標値を選ぶまでの最短時間（秒）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Time", meta = (ToolTip = "Minimum time in seconds before the wind picks a new random target. Keep this less than or equal to Max Time."))
	float MinTime = 10.f;

	/*
	 * Maximum time in seconds before choosing the next random wind target.
	 * 次のランダムな風の目標値を選ぶまでの最長時間（秒）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Time", meta = (ToolTip = "Maximum time in seconds before the wind picks a new random target. Keep this greater than or equal to Min Time."))
	float MaxTime = 60.f;

	/*
	 * Remaining time in seconds before the next random target is selected.
	 * 次のランダムな目標値を選ぶまでの残り時間（秒）です。
	 */
	float Remain = 0.f;

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	 * Minimum wind strength selected when the target changes.
	 * 目標値を変更するときに選ばれる風の最小強度です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Strength", meta = (ToolTip = "Minimum wind strength used for random targets. Keep this less than or equal to Max Strength."))
	float MinStrength = .0f;

	/*
	 * Maximum wind strength selected when the target changes.
	 * 目標値を変更するときに選ばれる風の最大強度です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Strength", meta = (ToolTip = "Maximum wind strength used for random targets. Keep this greater than or equal to Min Strength."))
	float MaxStrength = 0.2f;

	/*
	 * Maximum wind strength change applied per second while moving toward the target strength.
	 * 目標強度へ近づくときに1秒あたり適用される風の強度の最大変化量です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Strength", meta = (ToolTip = "Maximum strength change per second. Larger values make gusts react faster."))
	float StrengthVelocity = 0.01f;

	/*
	 * Random target strength currently being approached.
	 * 現在近づいているランダムな目標強度です。
	 */
	float TargetStrength = .0f;

	/*
	 * Current wind strength applied to the wind component.
	 * Windコンポーネントへ適用している現在の風の強度です。
	 */
	float CurrentStrength = .0f;

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	 * Minimum wind speed selected when the target changes.
	 * 目標値を変更するときに選ばれる風の最小速度です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Speed", meta = (ToolTip = "Minimum wind speed used for random targets. Keep this less than or equal to Max Speed."))
	float MinSpeed = .0f;

	/*
	 * Maximum wind speed selected when the target changes.
	 * 目標値を変更するときに選ばれる風の最大速度です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Speed", meta = (ToolTip = "Maximum wind speed used for random targets. Keep this greater than or equal to Min Speed."))
	float MaxSpeed = 0.2f;

	/*
	 * Maximum wind speed change applied per second while moving toward the target speed.
	 * 目標速度へ近づくときに1秒あたり適用される風の速度の最大変化量です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Speed", meta = (ToolTip = "Maximum speed change per second. Larger values make wind speed changes react faster."))
	float SpeedVelocity = 0.01f;

	/*
	 * Random target speed currently being approached.
	 * 現在近づいているランダムな目標速度です。
	 */
	float TargetSpeed = .0f;

	/*
	 * Current wind speed applied to the wind component.
	 * Windコンポーネントへ適用している現在の風の速度です。
	 */
	float CurrentSpeed = .0f;

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	 * Minimum yaw angle in degrees selected when the target changes.
	 * 目標値を変更するときに選ばれる風向きの最小ヨー角（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind yaw angle", meta = (ToolTip = "Minimum wind yaw angle in degrees used for random targets. Keep this less than or equal to Max Yaw."))
	float MinYaw = -180.f;

	/*
	 * Maximum yaw angle in degrees selected when the target changes.
	 * 目標値を変更するときに選ばれる風向きの最大ヨー角（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind yaw angle", meta = (ToolTip = "Maximum wind yaw angle in degrees used for random targets. Keep this greater than or equal to Min Yaw."))
	float MaxYaw = 180.f;

	/*
	 * Maximum yaw angle change in degrees per second while moving toward the target yaw.
	 * 目標ヨー角へ近づくときに1秒あたり適用されるヨー角の最大変化量（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind yaw angle", meta = (ToolTip = "Maximum yaw change in degrees per second. Larger values make wind direction turn faster."))
	float YawAngularVelocity = 360.f / 4.f;

	/*
	 * Random target yaw currently being approached.
	 * 現在近づいているランダムな目標ヨー角です。
	 */
	float TargetYaw = 0.f;

	/*
	 * Current yaw angle applied to this actor.
	 * このActorへ適用している現在のヨー角です。
	 */
	float CurrentYaw = 0.f;

	////////////////////////////////////////////////////////////////////////////////////////////////
	/*
	 * Minimum pitch angle in degrees selected when the target changes.
	 * 目標値を変更するときに選ばれる風向きの最小ピッチ角（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind pitch angle", meta = (ToolTip = "Minimum wind pitch angle in degrees used for random targets. Keep this less than or equal to Max Pitch."))
	float MinPitch = -5.625f;

	/*
	 * Maximum pitch angle in degrees selected when the target changes.
	 * 目標値を変更するときに選ばれる風向きの最大ピッチ角（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind pitch angle", meta = (ToolTip = "Maximum wind pitch angle in degrees used for random targets. Keep this greater than or equal to Min Pitch."))
	float MaxPitch = 5.625f;

	/*
	 * Maximum pitch angle change in degrees per second while moving toward the target pitch.
	 * 目標ピッチ角へ近づくときに1秒あたり適用されるピッチ角の最大変化量（度）です。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Wind pitch angle", meta = (ToolTip = "Maximum pitch change in degrees per second. Larger values make wind direction tilt faster."))
	float PitchAngularVelocity = 5.625f;

	/*
	 * Random target pitch currently being approached.
	 * 現在近づいているランダムな目標ピッチ角です。
	 */
	float TargetPitch = 0.f;

	/*
	 * Current pitch angle applied to this actor.
	 * このActorへ適用している現在のピッチ角です。
	 */
	float CurrentPitch = 0.f;

	////////////////////////////////////////////////////////////////////////////////////////////////
#if WITH_EDITORONLY_DATA
	/*
	 * Shows current wind values and direction in the viewport while running in the editor.
	 * エディター実行中に現在の風の値と向きをビューポートへ表示します。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random|Debug", meta = (ToolTip = "Shows current wind values and direction in the viewport while running in the editor."))
	bool ShowDebugInformation = false;
#endif

public:
	/*
	 * Creates the random wind source and enables ticking.
	 * ランダム風源を作成し、Tickを有効にします。
	 */
	ADungeonSampleRandomWindDirectionalSource(const FObjectInitializer& ObjectInitializer);

	/*
	 * Destroys the random wind source.
	 * ランダム風源を破棄します。
	 */
	virtual ~ADungeonSampleRandomWindDirectionalSource() override = default;

protected:
	/*
	 * Initializes the first wind target when gameplay starts.
	 * ゲームプレイ開始時に最初の風の目標値を初期化します。
	 */
	virtual void BeginPlay() override;

public:
	/*
	 * Updates the wind values and optional editor debug display every frame.
	 * 毎フレーム、風の値と任意のエディターデバッグ表示を更新します。
	 */
	virtual void Tick(float DeltaSeconds) override;

private:
	/*
	 * Selects the next random target values for time, strength, speed, yaw, and pitch.
	 * 時間、強度、速度、ヨー角、ピッチ角の次のランダムな目標値を選択します。
	 */
	void Change();
};
