/**
 * @author		Shun Moriya
 * @copyright	2025- Shun Moriya
 * All Rights Reserved.
 */

 #pragma once

#include <CoreMinimal.h>
DECLARE_LOG_CATEGORY_EXTERN(DungeonGeneratorEditorLogger, Log, All);
#define DUNGEON_GENERATOR_EDITOR_ERROR(Format, ...)		UE_LOG(DungeonGeneratorEditorLogger, Error, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_EDITOR_WARNING(Format, ...)	UE_LOG(DungeonGeneratorEditorLogger, Warning, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_EDITOR_DISPLAY(Format, ...)	UE_LOG(DungeonGeneratorEditorLogger, Display, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_EDITOR_LOG(Format, ...)		UE_LOG(DungeonGeneratorEditorLogger, Log, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_EDITOR_VERBOSE(Format, ...)	UE_LOG(DungeonGeneratorEditorLogger, Verbose, Format, ##__VA_ARGS__)
