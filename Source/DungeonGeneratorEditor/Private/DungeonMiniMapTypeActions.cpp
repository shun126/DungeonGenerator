/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonMiniMapTypeActions.h"
#include "../../DungeonGenerator/Public/MiniMap/DungeonMiniMap.h"

UClass* FDungeonMiniMapTypeActions::GetSupportedClass() const
{
	return UDungeonMiniMap::StaticClass();
}
