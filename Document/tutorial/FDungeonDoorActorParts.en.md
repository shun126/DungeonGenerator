# FDungeonDoorActorParts Reference

`FDungeonDoorActorParts` is the container used to register door actors spawned at room and aisle entrances. In addition to transform offsets (`FDungeonPartsTransform`), it specifies the actor class that should be treated as the door.

## Main usage
- Assign an existing door Blueprint derived from `DungeonDoorBase` to `ActorClass`, and that door is spawned during generation.
- Create multiple door variants with different meshes or animations and swap them per mesh set or floor theme to change the atmosphere of the same dungeon.

## What the UPROPERTY means
- **ActorClass (`UClass*`, EditAnywhere/BlueprintReadOnly, AllowedClasses=`DungeonDoorBase`)**  
  The door actor spawned during generation. Only Blueprints or C++ classes derived from `DungeonDoorBase` can be selected. If left unset, no door is placed.

## Editing tips
- Door opening direction and alignment can be adjusted with the base-class transform settings. Small depth adjustments often help the door fit the wall thickness naturally.
- Door behavior such as locked or breakable logic should be implemented in the door class itself. This struct only decides which door is placed.

