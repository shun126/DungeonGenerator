# UDungeonRoomSensorDatabase Migration Note

`UDungeonRoomSensorDatabase` is deprecated in v2.0.0 and is kept only so older assets can be loaded and migrated.

v2.0.0 is the migration release for v1 Room Sensor Database references. If your project still uses v1 assets, open the project in v2.0.0, review the migrated settings, save the affected assets, and then upgrade to v2.1 or later. v1-to-v2 migration support and this legacy database may be removed in v2.1 or later.

For new setup, configure Room Sensor gameplay directly in `UDungeonGenerateParameter`:

- `Gameplay.DungeonRoomSensorClass` sets the default `ADungeonRoomSensorBase` Blueprint for generated rooms.
- `Gameplay.SpawnActorInAisle` sets the default actor Blueprint candidates for generated aisles.
- `Zones[].GameplayOverride` can override the room sensor and aisle actor list for a zone.
- `Gameplay.RoomRoles.Roles[].GameplayOverride` can override the room sensor for a gameplay role.

When an older parameter references a `UDungeonRoomSensorDatabase`, the first valid legacy `DungeonRoomSensorClass` is copied to `Gameplay.DungeonRoomSensorClass`, and legacy `SpawnActorInAisle` is copied to `Gameplay.SpawnActorInAisle` if the new field is still empty.

Recommended migration flow:

1. Open the v1 project with Dungeon Generator v2.0.0.
2. Open or load the affected `UDungeonGenerateParameter` assets.
3. Confirm that Room Sensor settings were copied to `Gameplay.DungeonRoomSensorClass` and `Gameplay.SpawnActorInAisle`.
4. Save the migrated assets.
5. Upgrade to v2.1 or later only after the v2.0.0 save is complete.

## Related Pages
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
