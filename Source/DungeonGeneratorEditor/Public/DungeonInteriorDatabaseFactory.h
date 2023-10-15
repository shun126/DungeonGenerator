/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonInteriorDatabaseFactory.generated.h"

UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonInteriorDatabaseFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonInteriorDatabaseFactory();
	virtual ~UDungeonInteriorDatabaseFactory() = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS(Deprecated, meta = (DeprecationMessage = "UDungeonInteriorAssetFactory is deprecated, use UDungeonInteriorDatabaseFactory."))
class DUNGEONGENERATOREDITOR_API UDEPRECATED_DungeonInteriorAssetFactory : public UDungeonInteriorDatabaseFactory
{
	GENERATED_BODY()
};
