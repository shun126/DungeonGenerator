# UDungeonGenerateParameter Guide

`UDungeonGenerateParameter` is the central settings asset for dungeon generation.  
It references layout rules, start location, visuals, sublevels, sensors, and interiors.

## Settings to configure first
- `DungeonRoomMeshPartsDatabase`  
  The room `Mesh set database`. Generation cannot run without it.
- `DungeonAisleMeshPartsDatabase`  
  The aisle `Mesh set database`. Generation cannot run without it.
- `GridSize` / `VerticalGridSize`  
  The base size used by meshes and sublevels.
- `RandomSeed`  
  `0` means fully random. A fixed value is useful for reproducible results.

For a first check, those four fields are enough.

## Settings that shape the layout
- `RoomWidth` / `RoomDepth` / `RoomHeight`  
  Minimum and maximum room size. Larger values make rooms feel more like halls; smaller values feel more maze-like.
- `NumberOfCandidateRooms`  
  How many room candidates are tried during generation. If this is too small, generation fails more easily.
- `RoomMargin` / `VerticalRoomMargin`  
  Spacing between rooms. These affect density and how tightly floors stack vertically.
- `MergeRooms`  
  Makes it easier to merge adjacent rooms into larger rooms. When enabled, some spacing settings stop applying.
- `ExpansionPolicy`  
  Decides whether the dungeon expands horizontally, vertically, or both.  
  If you want a single-layer dungeon, use `ExpansionPolicy = Flat` instead of the legacy `Flat` setting.
- `NumberOfCandidateFloors`  
  Number of floor candidates used when trying multi-floor layouts. Not used when `ExpansionPolicy = Flat`.

## Settings for start location and progression
- `StartLocationPolicy`  
  How the start room is chosen. `UseSouthernMost` or `UseCentralPoint` are usually the easiest starting options.
- `MovePlayerStartToStartingPoint`  
  Legacy setting. In new content, prefer `StartLocationPolicy`.
- `UseMissionGraph`  
  Enables key-route or mission-style progression.  
  In practice, using it together with `MergeRooms = false` and `AisleComplexity = 0` is the safer setup.
- `AisleComplexity`  
  Controls how complex aisle routing becomes. For normal dungeons without MissionGraph, keep this at `1` or higher.
- `AisleCeilingHeightPolicy`  
  Switches aisle ceiling height between `1 Grid`, `2 Grids`, and `Random`. This affects not only the look but also how much vertical space aisle-side decoration has.

How to choose `AisleCeilingHeightPolicy`:
- `OneGrid`  
  Good when you want a lower ceiling and a more compressed feeling.
- `TwoGrids`  
  Good when you want a more open feeling, or when aisle-side decoration needs more height.
- `Random`  
  Good when you want variation between low and tall aisles.

Tune the final aisle look together with the mesh sets in `DungeonAisleMeshPartsDatabase`.  
If you want stronger ceiling-side decoration, review [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md) at the same time.

## Settings that change room atmosphere
- `GenerateSlopeInRoom`  
  Allows slopes inside rooms.
- `GenerateStructuralColumn`  
  Allows structural columns inside rooms.
- `SkylightChancePercent`  
  Probability of creating skylight voxels inside rooms.

## Parts and fixture settings
- `PillarPartsSelectionPolicy` / `PillarParts`
- `TorchPartsSelectionPolicy` / `TorchParts`
- `DoorPartsSelectionPolicy` / `DoorParts`

This is where you define candidates and selection rules for pillars, torches, and doors.  
The current editable setting is **`SelectionPolicy`**, not `SelectionMethod`.

- `Random`  
  Choose randomly.
- `Grid Index` / `Direction` / `Identifier` / `Depth From Start`  
  Choose deterministically from grid position or progression.
- `Custom Selector`  
  Use a custom selector asset.

When you use `Custom Selector`, assign a `UDungeonPartsSelector`-derived object to `Custom Dungeon Parts Selector`.  
`UDungeonSamplePartsSelector` is available as a sample implementation.

## Referenced databases
- `DungeonRoomMeshPartsDatabase`  
  Database for room floors, walls, roofs, chandeliers, and similar visuals
- `DungeonAisleMeshPartsDatabase`  
  Database for aisle floors, walls, roofs, and similar visuals
- `DungeonInteriorDatabase`  
  Database that spawns furniture and vegetation by tag
- `DungeonSubLevelDatabase`  
  Database that registers start, goal, and special-room sublevels
- `DungeonRoomSensorDatabase`  
  Database that registers room-entry sensors and aisle effects
- `DungeonRoomSensorClass`  
  Legacy setting. For new content, use `DungeonRoomSensorDatabase` instead.

## What `Verify` checks
`Verify` in `Window > DungeonGenerator` mainly checks the following issues:

- Missing room or aisle database assignments
- Room or aisle databases without floor, wall, or roof meshes
- Too few room candidates
- Broken referenced asset paths

Running `Verify` before generation removes many of the most common beginner mistakes.

## Suggested initial settings
- `RandomSeed = 0`
- `NumberOfCandidateRooms = 10`
- `ExpansionPolicy = ExpandHorizontally`
- `StartLocationPolicy = UseSouthernMost`
- `UseMissionGraph = false`
- `AisleComplexity = 5`

## Notes
- `PluginVersion` is mainly for support and bug-report checks.
- `Flat` is a legacy setting. In the current setup, use `ExpansionPolicy = Flat`.

## Read Next
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)  
  Review how mesh sets control the final look.
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)  
  Review how to insert special rooms and start-room sublevels.

## Related Pages
- [QuickStart.en.md](./QuickStart.en.md)
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)
- [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md)

