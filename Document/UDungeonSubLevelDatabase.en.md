# UDungeonSubLevelDatabase Guide

UDungeonSubLevelDatabase is a **product-only database for special sublevel rooms** such as boss arenas or mission-specific areas.

## Typical use
- Register sublevels or map assets that should replace normal rooms under certain conditions.
- Link this database in generation settings so the system can swap in these special rooms when required.

## Key UPROPERTY fields
- **SubLevelData (`FDungeonSubLevelData`, EditAnywhere/BlueprintReadOnly)**  
  Configuration for sublevel assets, including what to spawn and when.

## Editing tips
- Keep sublevels self-contained with their own lighting and navigation data.
- Test transition points so doors, corridors, and player spawn points align cleanly with the main dungeon.
