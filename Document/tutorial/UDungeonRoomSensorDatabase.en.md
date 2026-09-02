# UDungeonRoomSensorDatabase Deprecated API Note

`UDungeonRoomSensorDatabase` is deprecated and is not part of the Version 2 setup. Do not create or use this database for new content.

Configure room gameplay directly in `UDungeonGenerateParameter`:

- `Gameplay.DungeonRoomSensorClass` sets the default `ADungeonRoomSensorBase` Blueprint.
- `Gameplay.SpawnActorInAisle` sets default actor candidates for generated aisles.
- `Zones[].GameplayOverride` can replace the sensor or aisle actors in one Zone.
- `Gameplay.RoomRoles.Roles[].GameplayOverride` can replace the sensor for one gameplay role.

Implementation-only legacy fields or load-time conversions may still exist so old serialized data can be recognized. They do not constitute supported Version 1-to-Version 2 migration. Recreate the settings manually in new Version 2 assets, preferably in a separate project or source-control branch so the Version 1 project remains recoverable.

## Related Pages

- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
- [VersionComparison.en.md](./VersionComparison.en.md)
