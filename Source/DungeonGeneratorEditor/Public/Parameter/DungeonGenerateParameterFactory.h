/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonGenerateParameterFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonGenerateParameterFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonGenerateParameterFactory();
	virtual ~UDungeonGenerateParameterFactory() override = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
