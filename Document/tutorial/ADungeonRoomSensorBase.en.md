# ADungeonRoomSensorBase Guide

`ADungeonRoomSensorBase` is the base class for room events such as entering, leaving, initialization, and cleanup.  
Create a Blueprint derived from this class when you want traps, BGM switching, enemy spawning, or room-specific interior tags.

## Typical flow
1. Create a Blueprint whose parent class is `ADungeonRoomSensorBase`.
2. Implement the events you need.
3. Register that Blueprint in [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md).
4. Assign that database from `UDungeonGenerateParameter`.

## Frequently used events
- `OnPrepare`  
  Setup right after spawn. Return `false` if the sensor should not be placed.
- `OnInitialize`  
  Main logic after placement. You can inspect room type, item type, and depth information.
- `OnFinalize`  
  Cleanup before destruction.
- `OnReset`  
  Use this when you want to restore state after the player leaves the room.
- `OnResume`  
  Called when the player enters the room again and processing should resume.

## Useful properties
- `Bounding` / `HorizontalMargin` / `VerticalMargin`  
  Adjust the sensor range.
- `AutoReset`  
  Whether `OnReset` is called automatically after the player leaves.
- `DoorAddingProbability`  
  Probability of adding doors in that room.
- `SpawnActors`  
  Blueprints additionally spawned into the room.
- `SpawnKeyActor` / `SpawnUniqueKeyActor`  
  Key actors used with MissionGraph.

## Integrating with interiors
If you implement `GetInquireInteriorTags`, the sensor can return interior tags for that room.  
For example, return `kitchen` or `library`, then let [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md) spawn furniture with matching tags.

## Networking notes
- This actor is not designed around replication by default.
- If you use randomness that must stay synchronized, the server and client need to call it the same number of times.
- When in doubt, avoid heavy custom logic or asynchronous work and start with local-only effects.

## Common misunderstandings
- `SpawnActorInAisle` is not part of `ADungeonRoomSensorBase`. It is configured in [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md).
- The database decides which sensor class is used. What happens inside the room belongs in the Blueprint derived from this class.

## Read Next
- [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md)  
  Decide which room uses which sensor class.
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)  
  Review the interior-tag flow used by room sensors.

## Related Pages
- [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md)
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)

