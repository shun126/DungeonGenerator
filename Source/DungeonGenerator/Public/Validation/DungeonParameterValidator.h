/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once

#include "Validation/DungeonValidationIssue.h"

class UDungeonGenerateParameter;

class DUNGEONGENERATOR_API FDungeonParameterValidator
{
public:
	static void Validate(const UDungeonGenerateParameter* Params, TArray<FDungeonValidationIssue>& OutIssues, bool bDeepCheck);
};
