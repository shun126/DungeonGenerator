/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <CoreMinimal.h>
#include "UObject/SoftObjectPath.h"

#include "DungeonValidationIssue.generated.h"

/**
 * Severity levels used for dungeon validation results.
 *
 * ダンジョン検証結果で使用する重大度レベルです。
 */
UENUM()
enum class EDungeonValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info", ToolTip = "Informational validation result."),
	Warning UMETA(DisplayName = "Warning", ToolTip = "Validation warning that should be reviewed."),
	Error UMETA(DisplayName = "Error", ToolTip = "Validation error that should be fixed."),
};

/**
 * Single validation issue entry produced by dungeon parameter checks.
 *
 * ダンジョンパラメータの検証処理で生成される単一の問題エントリです。
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonValidationIssue
{
	GENERATED_BODY()

	/**
	 * Severity of this validation issue.
	 *
	 * この検証問題の重大度です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	EDungeonValidationSeverity Severity = EDungeonValidationSeverity::Info;

	/**
	 * Stable identifier used to classify the issue type.
	 *
	 * 問題の種類を分類するための安定した識別子です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FName Code;

	/**
	 * Human-readable message that explains the issue.
	 *
	 * 問題内容を説明する人間向けメッセージです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FText Message;

	/**
	 * Suggested fix guidance shown to users.
	 *
	 * 利用者に表示する修正案内です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FText FixHint;

	/**
	 * Name of the related parameter, when applicable.
	 *
	 * 関連するパラメータ名（該当する場合）です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FName ParameterName;

	/**
	 * Asset reference associated with this issue.
	 *
	 * この問題に関連付けられたアセット参照です。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FSoftObjectPath RelatedAsset;

	/**
	 * Whether an automatic fix can be applied.
	 *
	 * 自動修正を適用できるかどうかです。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	bool bCanAutoFix = false;
};
