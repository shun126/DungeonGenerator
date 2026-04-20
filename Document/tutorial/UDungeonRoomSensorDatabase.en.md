# UDungeonRoomSensorDatabase Guide

`UDungeonRoomSensorDatabase` is the database that decides which `ADungeonRoomSensorBase`-derived class is used in which room.  
Use it when you want room-entry events, BGM switching, enemy spawns, or trap behavior to vary by room.

## Typical flow
1. Create a Blueprint whose parent class is `ADungeonRoomSensorBase`.
2. Register that Blueprint in `DungeonRoomSensorClass`.
3. Use `SelectionMethod` to decide how the sensor class is chosen.
4. Optionally register aisle-effect Blueprints in `SpawnActorInAisle`.
5. Assign this database to `DungeonRoomSensorDatabase` in `UDungeonGenerateParameter`.

## Main properties
- `SelectionMethod`
  - `Random`  
    Select randomly each time.
  - `Identifier`  
    Select deterministically from room identifier.
  - `Depth From Start`  
    Select by progression distance from the start.
- `DungeonRoomSensorClass`  
  The list of `ADungeonRoomSensorBase`-derived classes that may be placed.
- `SpawnActorInAisle`  
  Blueprints additionally placed in aisles after generation completes.

## About `SpawnActorInAisle`
This is not for actors inside rooms. It is for extra effects placed on the aisle side.  
For enemies, treasure, or other gameplay inside the room itself, it is usually clearer to handle them in the `ADungeonRoomSensorBase` Blueprint.

## Editing tips
- If deeper floors should contain more dangerous rooms, `Depth From Start` is a good fit.
- Sensor selection only decides which class is placed. The behavior after entering the room belongs in each sensor Blueprint.
- Advanced custom behavior is not primarily driven by this database alone. For most setups, start with `Random`, `Identifier`, or `Depth From Start`.

## Read Next
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)  
  Review which events and properties are implemented in the sensor Blueprint.
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)  
  Review the interior-tag flow used from room sensors.

## Related Pages
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [UDungeonInteriorDatabase.en.md](./UDungeonInteriorDatabase.en.md)

