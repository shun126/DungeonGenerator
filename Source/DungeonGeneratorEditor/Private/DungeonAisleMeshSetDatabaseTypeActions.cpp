/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonAisleMeshSetDatabaseTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonAisleMeshSetDatabase.h"

UClass* FDungeonAisleMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonAisleMeshSetDatabase::StaticClass();
}
