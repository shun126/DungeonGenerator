/**
 * @author      Shun Moriya
 * @copyright   2026- Shun Moriya
 * All Rights Reserved.
 */

 #pragma once
#include <GameFramework/Character.h>
#include "DungeonSampleCharacterBase.generated.h"

/**
 * Base character used by the sample AI actors. It records the spawn transform so behavior tree decorators can compare the current position with the home position.
 * サンプルAIアクター用の基底キャラクターです。スポーン時のTransformを記録し、Behavior Treeデコレーターが現在位置とホーム位置を比較できるようにします。
 */
UCLASS(meta = (ToolTip = "Base character for sample AI actors. Stores the spawn transform as the character home position."))
class DUNGEONGENERATOR_API ADungeonSampleCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	/**
	 * 指定されたObject Initializerでサンプルキャラクターを作成します。
	 */
	explicit ADungeonSampleCharacterBase(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

	/**
	 * Destroys the ~ADungeonSampleCharacterBase instance.
	 * サンプルキャラクターを破棄します。
	 */
	virtual ~ADungeonSampleCharacterBase() override = default;

	/**
	 * Stores the initial transform as the home transform when play begins.
	 * プレイ開始時のTransformをホームTransformとして保存します。
	 */
	virtual void BeginPlay() override;

	/**
	 * Returns HomeTransform.
	 * キャラクターのホーム姿勢として記録された姿勢を返します。
	 */
	const FTransform& GetHomeTransform() const;

	/**
	 * Returns HomeLocation.
	 * キャラクターのホーム位置として記録された座標を返します。
	 */
	FVector GetHomeLocation() const;

protected:
	/**
	 * Initial transform used as the home position for sample patrol and territory checks.
	 * サンプルの巡回や縄張り判定でホーム位置として使う初期Transformです。
	 */
	FTransform HomeTransform;
};
