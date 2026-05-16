# ADungeonSubLevelScriptActor Guide

`ADungeonSubLevelScriptActor` is the parent class that lets Dungeon Generator read a hand-authored special-room sublevel.  
Every level registered in `UDungeonSubLevelDatabase` should use this parent class so the plugin can read its size and grid information.

## When to use it
- You want a boss room with a fixed design
- You want a hand-authored start room or goal room
- You want to insert occasional special rooms such as hidden rooms

## How to create it
1. Create a level for the special room.
2. Change the parent class of that level's Level Blueprint to `ADungeonSubLevelScriptActor`.
3. Set `Horizontal Size` and `Vertical Size` to match the main `UDungeonGenerateParameter`.
4. Enter `Width`, `Depth`, and `Height` as room size in grid counts, not centimeters.
5. Enable `ShowGrid` if needed and place meshes so they align with the grid.
6. Register the level in [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md), then run `Build`.

## Important points
- `Width`, `Depth`, and `Height` are grid counts, not centimeters.
- `Horizontal Size` and `Vertical Size` must match `GridSize` and `VerticalGridSize` in the main settings.
- `Build` in `UDungeonSubLevelDatabase` reads the size and grid information from this class and copies it into the database.

## Editing tips
- Place the sublevel entrance and passage openings assuming the room will later connect to auto-generated rooms.
- `Build` temporarily loads the level for analysis, so it is safer not to build the level you are actively editing at that moment.
- If the grid size does not match, `UDungeonSubLevelDatabase` will report an error during `Build`.

## Read Next
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)  
  Register this actor as a start room, goal room, or special room.
- [LobbyConnection.en.md](./LobbyConnection.en.md)  
  See how start-room sublevels differ from preloaded lobby connections.

## Related Pages
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)
- [LobbyConnection.en.md](./LobbyConnection.en.md)
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)

