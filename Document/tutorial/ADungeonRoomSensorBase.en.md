# ADungeonRoomSensorBase Guide

`ADungeonRoomSensorBase` is the base class for room-specific gameplay such as room entry events, enemy placement, rewards, traps, BGM switching, and interior tags.

## Typical flow
1. Create a Blueprint whose parent class is `ADungeonRoomSensorBase`.
2. Implement the events you need.
3. Assign that Blueprint to `Gameplay.DungeonRoomSensorClass` in `UDungeonGenerateParameter`.
4. Use `Zones[].GameplayOverride` or `Gameplay.RoomRoles.Roles[].GameplayOverride` only when a zone or role needs a different sensor.

## Base fill lights
Room Sensors create low-intensity, shadow-free Point Lights so that characters, furniture, and paths remain readable between decorative lights. These lights provide minimum gameplay visibility; torches, chandeliers, and other presentation lights remain responsible for the room's mood.

Use the properties in `DungeonGenerator|Lights|BaseFillLight` to adjust them:

- `Enable Base Fill Lights` turns generation on or off. It is disabled by default, so enable it deliberately when the room needs minimum fill lighting.
- `Cell Size` decides how much floor area one light covers. Large rooms are divided into several equal cells.
- `Max Base Fill Light Count X/Y` limits the number of lights and therefore the rendering cost. A small room uses one light, while a long room can use a `1 x N` row.
- `Intensity Units` selects `Unitless`, `Candelas`, or `Lumens`. The default is `Unitless`.
- `Intensity` sets the brightness in the selected units, and `Color` adjusts the fill color. The default intensity is `10.0` Unitless. Values above the slider range can be entered directly.
- `Height From Floor Ratio` places each light between the floor (`0`) and ceiling (`1`). The default `0.6` places it above characters without pushing it against the ceiling.
- `Attenuation Scale` and the minimum/maximum radius control how far the light reaches. The defaults are `1.05`, `100 cm`, and `2000 cm`. The automatic radius reaches from the light to the farthest floor corner of its cell.

Each light is placed at the horizontal center of its cell and always has Cast Shadow disabled. `Lumens` and `Candelas` use physically based inverse-square falloff. `Unitless` is an artistic adjustment mode that uses the legacy falloff exponent of `2`. Because shadow-free Point Lights are not blocked by walls or ceilings, an unnecessarily large attenuation radius can illuminate neighboring rooms or floors. Prefer increasing the X/Y light count before greatly increasing the maximum radius.

When dungeon partition load control is available, the replicated lighting proxy hides generated lights in distant partitions and restores them nearby. The Room Sensor also owns a generic visibility activator: other visible Scene Components added to the Room Sensor Blueprint are controlled by that activator too. If a component must remain visible independently, place it on a separate actor or account for the partition visibility behavior.

Room Sensors remain authority-only and are not replicated, so their gameplay events continue to run only on the server. A replicated visual proxy automatically constructs the same Base Fill and Guidance lights for every client, including clients that join later. Call `SetBaseFillLightIntensity`, `SetGuidanceLightIntensity`, or `SetRoomLightIntensities` on the server to change one room at runtime. Use `ADungeonGenerateActor::SetAllRoomLightIntensities` to update every generated room. Negative values are clamped to zero.

## Guidance lights
Room Sensors can also create shadow-free Spot Lights that make generated doors and room stairs easier to notice. Only doors that were actually spawned and slopes that belong to the room are used. Corridor stairs are not included.

Use `DungeonGenerator|Lights|GuidanceLight` to tune the result. Guidance lights and door targets are enabled by default; stair targets are disabled by default. The default base intensity is `8` Candelas, attenuation radius is `1200 cm`, inner/outer cone angles are `18/32` degrees, and the room limit is `8` lights. `Lumens` and `Candelas` use inverse-square falloff, while `Unitless` uses the artistic falloff exponent of `2`. `Door Guidance Light Height Offset Scale` defaults to `0.5` vertical grids above the door. `Stair Guidance Light Height Below Ceiling` defaults to `100 cm`; the target height defaults to `120 cm`. Door and stair intensity multipliers default to `1.0` and `1.15`. Doors are considered before stairs when the limit is reached.

Door lights are placed on the room side, above the door, and point back toward the entrance. A door light is omitted when the grid directly above the door is not open `Floor` space—for example, in a one-grid-high room or beneath an interior slope. Stair lights remain ceiling-relative. Keep the cone and attenuation radius as narrow as the art allows because these lights do not cast shadows and can otherwise leak through nearby walls or floors. `GuidanceTargets` remains available as read-only server-side Blueprint information for projects that want to add matching decals, meshes, or effects; the replicated visual proxy owns the rendered light components after generation.

## Frequently used events
- `OnPrepare`  
  Setup right after the sensor is spawned. Return `false` if the sensor should not be placed.
- `OnInitialize`  
  Main logic after placement. It receives `FDungeonGeneratedRoomInfo`, which is the standard v2 room-information API.
- `OnFinalize`  
  Cleanup before destruction.
- `OnReset`  
  Use this when you want to restore state after the player leaves the room.
- `OnResume`  
  Called when the player enters the room again and processing should resume.

## Room information API
Use the `FDungeonGeneratedRoomInfo` value passed to `OnInitialize` when you initialize the room.  
Use `GetGeneratedRoomInfo()` when another Blueprint function needs the same information later.

`FDungeonGeneratedRoomInfo` contains the room information that gameplay Blueprints usually need:

- `Identifier`  
  Unique identifier assigned to this room.
- `RoomStructuralRole`  
  Route-structure role assigned to the room, such as `Start`, `Goal`, `Hub`, `Connector`, `Branch`, or `DeadEnd`.
- `RoomGameplayRole`  
  Gameplay role assigned to the room, such as `Combat`, `Treasure`, `Puzzle`, `Rest`, `Boss`, or `Secret`.
- `Parts` / `Item`  
  Legacy room type and MissionGraph item information.
- `bSecretRoom`  
  `true` for secret rooms.
- `bDeadEndRoom`  
  `true` for dead-end rooms.
- `bMainPathRoom`  
  `true` for rooms on the main start-to-goal route.
- `bLockedRouteRoom` / `bHasLockedDoor`  
  Whether the room is related to a locked route or locked door.
- `ZoneName` / `ZoneIndex`  
  The Zone this room belongs to.
- `BranchId`  
  Branch identifier generated by the MissionGraph.
- `DepthFromStart` / `DeepestDepthFromStart` / `DepthFromStartRatio`  
  Progress from the start room. Useful for changing behavior between early, middle, and late dungeon areas.

## Deprecated room fields
Some deprecated room-information fields still exist directly on `ADungeonRoomSensorBase` as implementation compatibility remnants. They do not provide supported migration from Version 1.
For new v2 Blueprints, do not read `Identifier`, `Parts`, `Item`, `BranchId`, `DepthFromStart`, or `DeepestDepthFromStart` directly from the sensor. Use `GetGeneratedRoomInfo()` instead.

## Integrating with interiors
Implement `GetProvidedContextTags` to provide semantic context such as `kitchen` or `library` from the room to Interior Parts. A Part is eligible only when every one of its `Required Context Tags` is provided.
For example, return `Secret` for secret rooms, `Treasure` for treasure rooms, `DeadEnd` for dead ends, or `Locked` for locked-route rooms.

The plugin uses `Theme.DungeonInteriorDatabase` to place furniture, decoration, and effects that match those tags.

## Common use cases
- Place a hidden chest only when `bSecretRoom` is `true`.
- Place a reward or short event in `bDeadEndRoom` rooms.
- Spawn enemies when `RoomGameplayRole == Combat`.
- Avoid enemy placement and add recovery props when `RoomGameplayRole == Rest`.
- Change enemy types or interior tags based on `ZoneName`.

## Helper spawn parameters
Properties in `DungeonGenerator|Helper|SpawnActorInRoom` and `DungeonGenerator|Helper|MissionGraph` are helper settings for quickly spawning actors from a room sensor. They are a good starting point for Quick Start checks and early tuning before you build detailed Blueprint logic.

- `SpawnActors` is the list of enemy actor Blueprint candidates to spawn in the room.
- `AreaRequiredPerPerson` and `MaxNumberOfActor` control the area-based enemy count before role multipliers are applied.
- `SpawnKeyActor` and `SpawnUniqueKeyActor` help spawn key actors used by MissionGraph key-and-lock routes.

The key helpers only spawn the configured actors. Pickup, inventory registration, door checks, and key consumption must be implemented in your gameplay Blueprints.

Use Blueprint logic when you need fine control, such as switching rewards, traps, events, enemy types, or presentation by room role, Zone, depth, or route flags.

## Enemy spawn scaling
`SpawnActors` is treated as the room's enemy actor list. The sensor first calculates the ideal count from room area, then multiplies it by the matching fields in `GameplayRoleEnemySpawnMultipliers` and `StructuralRoleEnemySpawnMultipliers`.

Each scale setting has one field per role, so edit the field for the role you want to tune. There is no need to add array entries, and the same role cannot be configured twice.

`IdealNumberOfActor()` returns the final helper count after these gameplay and structural role multipliers are applied.

Room flags such as `bMainPathRoom`, `bDeadEndRoom`, `bLockedRouteRoom`, and `bSecretRoom` are room information for Blueprint logic. Use them to branch rewards, traps, events, and interior presentation; they do not change the helper enemy spawn count by themselves.

Default gameplay multipliers are `None = 0.5`, `Combat = 1.0`, `Treasure = 0.8`, `Puzzle = 0.5`, `Rest = 0.0`, `Boss = 2.0`, and `Secret = 0.7`. `Start` and `Goal` structural roles default to `0.0`, so they do not spawn enemies unless you override the structural scale.

## Common misunderstandings
- `SpawnActorInAisle` is not part of `ADungeonRoomSensorBase`. It is configured in `Gameplay.SpawnActorInAisle` or `Zones[].GameplayOverride.SpawnActorInAisle`.
- `Gameplay.DungeonRoomSensorClass` decides the default sensor class. What happens inside the room belongs in the Blueprint derived from this class.
- This actor is not designed around replication by default. If synchronized randomness is required, server and client code must call it the same number of times.

## Related Pages
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)
