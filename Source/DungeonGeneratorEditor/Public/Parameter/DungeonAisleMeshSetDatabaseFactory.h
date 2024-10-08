/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonAisleMeshSetDatabaseFactory.generated.h"

/*
UDungeonAisleMeshSetDatabaseを生成するファクトリークラス
*/
UCLASS()
class DUNGEONGENERATOREDITOR_API UDungeonAisleMeshSetDatabaseFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonAisleMeshSetDatabaseFactory();
	virtual ~UDungeonAisleMeshSetDatabaseFactory() override = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
