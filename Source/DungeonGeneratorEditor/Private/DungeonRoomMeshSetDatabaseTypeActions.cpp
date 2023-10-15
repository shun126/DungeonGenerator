/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonRoomMeshSetDatabaseTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonRoomMeshSetDatabase.h"

UClass* FDungeonRoomMeshSetDatabaseTypeActions::GetSupportedClass() const
{
	return UDungeonRoomMeshSetDatabase::StaticClass();
}
