# Lobby Connection Guide

If you want to connect an existing lobby to the dungeon, there are two main approaches: using `ADungeonGenerateActor::StartRoomSubLevelScriptActor`, or using `UDungeonSubLevelDatabase::StartRoom`.

## When to use it
- You want to connect the dungeon directly to a lobby that already exists in the level
- You want to manage the start room as a hand-authored sublevel
- You want the packaged build to use a fixed start-room look and size

## Method A: Use a placed lobby as the start room
Assign a level-side `ADungeonSubLevelScriptActor` to `StartRoomSubLevelScriptActor` on `ADungeonGenerateActor`.

### Good fit for
- A lobby already exists in the level
- You want the dungeon to connect directly to that lobby
- You want the generated start room to align to the placed lobby after generation

### Steps
1. Place `ADungeonSubLevelScriptActor` in the lobby level.
2. Assign that actor to `StartRoomSubLevelScriptActor` on `ADungeonGenerateActor`.
3. Set `MovePlayerStartToStartingPoint = false` in `UDungeonGenerateParameter`.
4. Set `StartLocationPolicy = NoAdjustment` in `UDungeonGenerateParameter`.
5. Do not use `UseMultiStart`; keep a normal single start point.

### Important notes in the current implementation
- `UseMultiStart` is not supported and causes an error
- In the editor, it can work with temporary metadata even if `DungeonSubLevelDatabase.StartRoom` is not configured
- In packaged builds, metadata for `DungeonSubLevelDatabase.StartRoom` is required
- If `DungeonSubLevelDatabase.StartRoom` is configured, its grid size, room size, and level asset must match the lobby sublevel

## Method B: Insert a start-room sublevel from the generation side
Use `StartRoom` in `UDungeonSubLevelDatabase` so the start room is part of the normal generation flow.

### Good fit for
- You want the start room to be managed from generation assets
- You do not want to keep the lobby permanently loaded
- You want the same metadata to be managed explicitly in both editor and packaged builds

### Steps
1. Place `ADungeonSubLevelScriptActor` in the start-room sublevel.
2. Match the grid size and room size to `UDungeonGenerateParameter`.
3. Register that sublevel in `StartRoom` of [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md).
4. Run `Build` to update metadata.
5. Assign that asset to `DungeonSubLevelDatabase` in `UDungeonGenerateParameter`.

## Which one to choose
- Connect directly to an existing lobby  
  Method A
- Keep the start room fully managed from generation assets  
  Method B

## Read Next
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)  
  Review actor-side settings such as `StartRoomSubLevelScriptActor`.
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)  
  Review `StartRoom` registration and the `Build` flow.

## Related Pages
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)
- [ADungeonSubLevelScriptActor.en.md](./ADungeonSubLevelScriptActor.en.md)
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)

