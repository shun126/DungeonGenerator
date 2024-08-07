/**
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "Parameter/DungeonGenerateParameterFactory.h"
#include "Parameter/DungeonGenerateParameter.h"

UDungeonGenerateParameterFactory::UDungeonGenerateParameterFactory()
{
	SupportedClass = UDungeonGenerateParameter::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDungeonGenerateParameterFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDungeonGenerateParameter>(InParent, InClass, InName, Flags);
}
