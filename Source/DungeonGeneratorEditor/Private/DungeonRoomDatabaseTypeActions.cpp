/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomDatabaseTypeActions.h"
#include "../../DungeonGenerator/Public/SubLevel/DungeonRoomDatabase.h"

UClass* FDungeonRoomDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonRoomDatabase::StaticClass();
}
