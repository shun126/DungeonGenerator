# FDungeonDoorActorParts Guide

FDungeonDoorActorParts is a **container for door Blueprint classes** used when spawning doors in generated rooms or corridors. It keeps your door variations organized.

## Typical use
- Register one or more door Blueprint classes that include meshes, animations, and collision.
- The generator spawns the class you specify for each door position.

## Key UPROPERTY fields
- **ActorClass (`UClass*`, EditAnywhere/BlueprintReadWrite)**  
  The Blueprint (or other Actor class) used as the door. If left empty, no door is spawned at that slot.

## Editing tips
- Keep doors self-contained: include frame, open/close logic, and sounds in the Blueprint so swapping classes is painless.
- Test that the pivot and collision line up with the dungeon grid to avoid gaps or overlaps.
