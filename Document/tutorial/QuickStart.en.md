# Dungeon Generator Quick Start

This page explains the shortest path to generating and previewing one dungeon.  
The recommended flow is to first preview the look in the editor, then connect the same settings to a level-placed `ADungeonGenerateActor`.

## Goal
- Create one `Generate parameter`
- Create one room `Mesh set database` and one aisle `Mesh set database`
- Use `Verify` to find missing settings
- Use `Generate dungeon` to preview the result

## Before You Start: Enable the Plugin
The first time only, enable the `DungeonGenerator` plugin before creating assets.  
In Unreal Editor, use the following steps.

![enable-plugin-ja](./images/LoadPlugin.png)

1. Open `Edit > Plugins`.
2. Find the `DungeonGenerator` plugin and enable it.
3. If Unreal asks to restart, restart the editor.

After the plugin is enabled, you can create the required assets from the `DungeonGenerator` category in the Content Browser.

## 1. Create the required assets
Create the following assets from the `DungeonGenerator` category in the Content Browser.

1. `Generate parameter`
2. `Mesh set database`  
   For rooms
3. `Mesh set database`  
   For aisles

These three assets are enough for the first test.  
You can add `Interior database`, `Sub level database`, and `Room sensor database` later.

## 2. Register the minimum room and aisle meshes
Open both `Mesh set database` assets and add the following parts to the first `Mesh Set`.

- `Floor Parts`
- `Wall Parts`
- `Roof Parts`
- `Slope Parts`

However, generation will fail if there is no floor, wall, roof or slope mesh at all.

## 3. Assign the databases to `Generate parameter`
Open `Generate parameter` and at minimum set the following fields.

- `DungeonRoomMeshPartsDatabase`  
  Room `Mesh set database`
- `DungeonAisleMeshPartsDatabase`  
  Aisle `Mesh set database`

The following defaults are fine for an initial check.

- `RandomSeed = 0`
- `NumberOfCandidateRooms = 10`
- `StartLocationPolicy = UseSouthernMost`
- `AisleComplexity = 5`

For a first test without `UseMissionGraph`, keeping `AisleComplexity` at `1` or higher usually avoids confusion.

## 4. Preview in the editor
Open `Window > DungeonGenerator`.  
Select the `Generate parameter` asset in that window and use the following order.

1. `Verify`
2. Fix any reported issue
3. `Generate dungeon`

`Verify` checks issues such as:

- Missing room or aisle database assignments
- Missing floor, wall, or roof meshes
- Too few room candidates, which can make generation fail

If you want to share the current validation result, use `Copy diagnostics`.

## 5. Use it in a level
To use the dungeon in an actual level, place `ADungeonGenerateActor` and assign the same `Generate parameter` to `DungeonGenerateParameter`.

- Generate automatically when the level starts  
  `AutoGenerateAtStart = true`
- Generate at any time  
  Call `GenerateDungeon` or `GenerateDungeonWithParameter` from Blueprint

See [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md) for details.

## Common mistakes
- You press the generate button and nothing appears  
  Run `Verify` and make sure floor, wall, and roof meshes exist.
- The preview works but the level setup does not  
  Make sure `ADungeonGenerateActor` has `DungeonGenerateParameter` assigned.
- A sublevel is used and the size is wrong  
  Check `Build` in `UDungeonSubLevelDatabase` and the grid size of `ADungeonSubLevelScriptActor`.

## Read Next
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)

