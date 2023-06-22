/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonInteriorAssetFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonInteriorAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	/*
	Constructor
	*/
	UDungeonInteriorAssetFactory();

	/*
	Destructor
	*/
	virtual ~UDungeonInteriorAssetFactory() = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
