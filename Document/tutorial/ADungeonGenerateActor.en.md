# ADungeonGenerateActor Guide

`ADungeonGenerateActor` is the runtime actor you place in a level to generate a dungeon.  
If you only want a quick editor preview, `Window > DungeonGenerator` is faster. Use this actor when you want generation during gameplay or as a fixed part of a level.

## Role
- Generate a dungeon from `DungeonGenerateParameter`
- Destroy a previously generated dungeon
- Coordinate optional features such as minimaps and sublevels

## Minimum setup
1. Place `ADungeonGenerateActor` in the level.
2. Assign a `Generate parameter` asset to `DungeonGenerateParameter`.
3. Set `AutoGenerateAtStart` if needed.
4. Call `GenerateDungeon` when play starts or from Blueprint.

## Main properties
- `DungeonGenerateParameter`  
  Required. Most rules for generation, visuals, sublevels, and sensors come from this asset.
- `AutoGenerateAtStart`  
  Whether to generate automatically when the level begins. Keeping this `true` is often the easiest first check.
- `Dungeon Floor Slope Mesh Generation Method`  
  Generation method for floors, slopes, and catwalks. Instanced methods are better for large counts.
- `DungeonWallRoofPillarMeshGenerationMethod`  
  Generation method for walls, roofs, and pillars. `Hierarchical Instanced Static Mesh` is effective when many wall pieces are used.
- `StartRoomSubLevelScriptActor`  
  Used when a preloaded lobby sublevel should be treated as the start room.

## How to trigger generation
- `GenerateDungeon`  
  Generates using the already assigned `DungeonGenerateParameter`.
- `GenerateDungeonWithParameter`  
  Generates using a different `DungeonGenerateParameter` passed at call time. This can also receive a temporary Parameter created in memory.
- `DestroyDungeon`  
  Removes the generated dungeon.

## Reproducing a generation failure
In a development Editor build (`JENKINS_FOR_DEVELOP=1`), a failed `GenerateDungeonWithParameter` call in a standalone session automatically saves the supplied Parameter as a reproducible asset. This also works when the Parameter was created only in memory with `GenerateRandomParameter`. Release builds do not create failure assets.

- If `GenerateRandomParameter` received a saved source asset, the failed copy is saved beside that source as `<SourceAssetName>_Failed_Seed_<Seed>`.
- If the temporary Parameter has no saved source, a dialog asks where to save it. Cancelling the dialog discards the captured copy.
- The actual failed seed is fixed as `RandomSeed`, so assigning the saved asset and generating again reproduces the same attempt.
- Each failure creates a new uniquely named asset. Successful generation does not create an asset.

Automatic failure saving is available only in development Editor builds and standalone sessions. Multiplayer sessions, release Editor builds, and packaged builds continue generation normally but do not create assets. When sharing a problem, send the saved Parameter together with its referenced databases and meshes, not only the seed.

## When you want to connect a lobby
If you want to use a preloaded lobby as the start room, use `StartRoomSubLevelScriptActor`.  
In that case, the following restrictions apply.

- Set `Path.bMovePlayerStartToStartRoom = false` in `UDungeonGenerateParameter`
- Set `Path.StartRoomPolicy = UseCentralPoint` in `UDungeonGenerateParameter`
- `UseMultiStart` cannot be combined with it
- The referenced sublevel must already be loaded and must contain `ADungeonSubLevelScriptActor`
- `DungeonSubLevelDatabase.StartRoom` is ignored while this preloaded start room is set

If you want a fixed start room without preloading a lobby, use [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md) instead.  
See [LobbyConnection.en.md](./LobbyConnection.en.md) for the full comparison.

## Operating tips
- Run `Verify` on `DungeonGenerateParameter` first, then assign it to this actor. That reduces rework.
- When you only want to inspect the look, the plugin window preview is faster.
- Add features such as minimaps and sublevels only after the base generation is stable. Debugging is easier that way.

## Read Next
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)  
  Review the full settings asset used by this actor.
- [LobbyConnection.en.md](./LobbyConnection.en.md)  
  Dedicated guide for connecting a preloaded lobby or start-room sublevel.

## Related Pages
- [QuickStart.en.md](./QuickStart.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [LobbyConnection.en.md](./LobbyConnection.en.md)
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)

