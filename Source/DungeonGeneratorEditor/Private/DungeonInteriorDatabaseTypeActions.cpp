/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonInteriorDatabaseTypeActions.h"
#include "../../DungeonGenerator/Public/Decorator/DungeonInteriorDatabase.h"

UClass* FDungeonInteriorDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonInteriorDatabase::StaticClass();
}
