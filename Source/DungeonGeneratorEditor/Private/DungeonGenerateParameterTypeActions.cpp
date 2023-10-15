/**
\author		Shun Moriya
\copyright	2023- Shun Moriya
All Rights Reserved.
*/

#include "DungeonGenerateParameterTypeActions.h"
#include "../../DungeonGenerator/Public/DungeonGenerateParameter.h"

UClass* UDungeonGenerateParameterTypeActions::GetSupportedClass() const
{
	return UDungeonGenerateParameter::StaticClass();
}
