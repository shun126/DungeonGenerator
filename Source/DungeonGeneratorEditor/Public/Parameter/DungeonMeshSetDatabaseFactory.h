/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include <CoreMinimal.h>
#include <Factories/Factory.h>
#include "DungeonMeshSetDatabaseFactory.generated.h"

/*
UDungeonMeshSetDatabaseを生成するファクトリークラス
*/
UCLASS(ClassGroup = "DungeonGenerator")
class DUNGEONGENERATOREDITOR_API UDungeonMeshSetDatabaseFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDungeonMeshSetDatabaseFactory();
	virtual ~UDungeonMeshSetDatabaseFactory() override = default;

	// UFactory overrides
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
