# Dungeon Generator Tutorial Index

This directory collects beginner-oriented walkthroughs and per-asset/class reference pages for the plugin.  
If you are new to the plugin, new to Unreal Engine, or not mainly an engineer, start with the "Read First" section in order.

If you want to build the visuals with your own meshes, reading the **mesh preparation page** right after `QuickStart` is usually the easiest path.

## Read First
1. [QuickStart.en.md](./QuickStart.en.md)  
   The shortest path to generating and previewing one dungeon.
2. [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)  
   The runtime actor you place in a level, including its role and main settings.
3. [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)  
   The central asset that controls layout, start position, and database references.

## Improve Visuals
- [PrepareMeshParts.en.md](./PrepareMeshParts.en.md)  
  The starting point for using your own meshes. It explains the minimum floor, wall, roof, and slope setup, along with orientation, pivot, and validation tips.
- [StaticMeshFitTool.en.md](./StaticMeshFitTool.en.md)  
  Check selected Static Mesh assets against the Dungeon Generator grid and generate fitted copies.
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)  
  Manage visual themes such as floors, walls, roofs, and chandeliers.
- [FDungeonMeshParts.en.md](./FDungeonMeshParts.en.md)  
  How to register a single mesh part.
- [FDungeonRandomActorParts.en.md](./FDungeonRandomActorParts.en.md)  
  Probability-based actor parts.
- [FDungeonDoorActorParts.en.md](./FDungeonDoorActorParts.en.md)  
  Actor parts for doors.

## Add Special Rooms and Events
- [ADungeonSubLevelScriptActor.en.md](./ADungeonSubLevelScriptActor.en.md)  
  Parent class for hand-authored special-room sublevels.
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)  
  Register start rooms, goal rooms, and random special rooms.
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)  
  Base class for room-entry events, traps, and BGM switching.
- [UDungeonRoomSensorDatabase.en.md](./UDungeonRoomSensorDatabase.en.md)  
  Decide which sensor class is used in which room.
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)  
  Spawn furniture, decorations, and vegetation by tag.

## Advanced
- [GenerateMinimapTexture.en.md](./GenerateMinimapTexture.en.md)  
  Generate minimaps from the dungeon and use them for saved textures or widget display.
- [ApplyMissionGraph.en.md](./ApplyMissionGraph.en.md)  
  Build a progression route with locked doors and key placement using MissionGraph.
- [CustomSelector.en.md](./CustomSelector.en.md)  
  Replace mesh-set or part selection rules with selector assets.
- [LobbyConnection.en.md](./LobbyConnection.en.md)  
  Connect a prebuilt lobby or a hand-authored start-room sublevel to the dungeon.

## Minimum Required Assets
- `Generate parameter` asset
- Two `Mesh set database` assets  
  One is usually for rooms and the other for aisles.
- A level for previewing the result  
  You can either preview from `Window > DungeonGenerator` or place `ADungeonGenerateActor` in a level.

## Common Blockers
- The room or aisle `Mesh set database` is not assigned
- The `Mesh set database` contains no floor, wall, or roof mesh
- The sublevel `GridSize` does not match `UDungeonGenerateParameter`
- You edited `UDungeonSubLevelDatabase` or `UDungeonInteriorDatabase` but forgot to run `Build`

## Read Next
- [QuickStart.en.md](./QuickStart.en.md)  
  Start here if you want the fastest path to a working dungeon.
- [PrepareMeshParts.en.md](./PrepareMeshParts.en.md)  
  Read this next if you want to start building your own floor, wall, roof, and slope meshes.
- [StaticMeshFitTool.en.md](./StaticMeshFitTool.en.md)  
  Read this next if you want to check existing Static Mesh sizes or generate grid-fitted copies.
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)  
  Read this next if you want to generate dungeons in-game from a level.
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)  
  Read this next if you want a full overview of the main generation settings.
