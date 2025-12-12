# UDungeonRoomSensorDatabase Guide

UDungeonRoomSensorDatabase is a **product-only database that pairs rooms with sensor logic** to trigger events, scoring, or analytics.

## Typical use
- Define which sensor setups should be applied to generated rooms (e.g., encounter triggers, loot tracking, or player presence sensors).
- Assign this database so the generator attaches the correct sensor Blueprints when building rooms.

## Key UPROPERTY fields
- **RoomSensorData (`FDungeonRoomSensorData`, EditAnywhere/BlueprintReadOnly)**  
  Settings for which sensors to use and how they activate within rooms.

## Editing tips
- Keep sensor Blueprints modular so they can be reused across room types.
- Validate that sensors fire at the right time by running quick test generations before release.
