/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include <CoreMinimal.h>
#include "UObject/SoftObjectPath.h"

#include "DungeonValidationIssue.generated.h"

UENUM()
enum class EDungeonValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info", ToolTip = "Informational validation result."),
	Warning UMETA(DisplayName = "Warning", ToolTip = "Validation warning that should be reviewed."),
	Error UMETA(DisplayName = "Error", ToolTip = "Validation error that should be fixed."),
};

USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FDungeonValidationIssue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	EDungeonValidationSeverity Severity = EDungeonValidationSeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FName Code;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FText Message;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FText FixHint;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FName ParameterName;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	FSoftObjectPath RelatedAsset;

	UPROPERTY(BlueprintReadOnly, Category = "DungeonGenerator|Validation")
	bool bCanAutoFix = false;
};
