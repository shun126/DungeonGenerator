# UDungeonInteriorDatabase Guide

`UDungeonInteriorDatabase` places furniture, decoration actors, and vegetation in generated rooms, flat aisles, and slopes. Version 2 separates physical placement from semantic context, making each rule easier to understand and tune.

## Interior Parts

Use `Interior Parts` for furniture and decoration actors.

- `Placement Areas`: Check any combination of `Room`, `Aisle`, and `Slope`. Interior Actors use floor candidates beside generated walls; they are not spawned automatically at the room center. `Aisle` means a flat passage. `Slope` includes slopes, stairwells, and multi-height passage spaces, and is disabled by default for new Parts.
- `Priority`: Higher values are attempted first. If placement fails, lower priorities are tried.
- `Spawn Chance`: The chance to participate in one placement attempt, shown from `0%` to `100%`.
- `Required Context Tags`: Every listed tag must be provided by the room sensor or an Interior Location. Normal area placement may leave this empty, but nested Interior Location spawning requires at least one explicit tag.

Priority, chance, and weight are different:

- Priority controls placement order.
- Spawn Chance controls whether a part participates.
- Selection Weight controls relative lottery odds where a weighted selector is used.

## Vegetation Parts

Use `Vegetation Parts` for grass, moss, vines, hanging plants, and similar meshes.

- `Placement Areas`: Check any combination of `Room`, `Aisle`, and `Slope`. Gate surfaces and the generated walls and ceilings of slopes owned by a room use `Room`. Slopes owned by a passage use `Slope`. `Slope` is disabled by default for new Parts.
- `Placement Surfaces`: Check any combination of `Floor`, `Wall`, and `Ceiling`.
- `Density`: Evaluated independently for each enabled surface using that surface's area.
- `Max Spawn Count`: Applied independently to each enabled surface.
- `Required Context Tags`: Restricts vegetation to contexts such as `kitchen`, `library`, or `overgrown`.

Floor keeps its downward terrain trace. Wall and Ceiling use generated grid faces directly. Gate floors, walls, and ceilings use the containing room's `Room` settings, Interior Database, and Provided Context Tags. The generated walls and ceilings of room-owned slopes, stairwells, and multi-height spaces use those same room settings; their floor behavior is unchanged. Passage-owned slope-family grids continue to use `Slope`. Structural columns contribute only the wall faces beside Floor or Deck grids. Wall and ceiling vegetation meshes should place their pivot at the attachment point and use local +Z as the direction extending away from the surface.

## Context Tags

Provided Context Tags describe meaning, not geometry. They can come from:

- `ADungeonRoomSensorBase::GetProvidedContextTags`
- `UDungeonInteriorLocationComponent::ProvidedContextTags`

For example, a Part requiring `library` is eligible only when the current room or location includes `library` in its Provided Context Tags. Every Required Context Tag must be provided. Context tags cannot substitute for Placement Area or Surface.

An Interior Location never selects a Part with empty Required Context Tags. If a nested Actor contains more Interior Locations, generation may continue up to the existing depth limit, but an Actor Class already present in that branch is skipped to prevent cycles such as `Shelf → Shelf` or `Shelf → Box → Shelf`. The same child Class can still be used at separate sibling locations.

## Gameplay Roles and special rooms

Use `EDungeonRoomGameplayRole` to switch the complete Interior Database for gameplay experiences such as Combat, Treasure, Puzzle, Rest, Boss, or Secret. Resolution order is Gameplay Role override, Zone override, then the default Theme database.

Interior Parts do not filter by Hall, Hanare, Start, or Goal. If the start or goal needs a hand-authored layout, use a Start/Goal sublevel. A sublevel is better suited to controlling the entire room shape and presentation reliably.

## Typical workflow

1. Add furniture actors to `Interior Parts` and vegetation meshes to `Vegetation Parts`.
2. Select the Interior Actor Areas or Vegetation Areas and Surfaces.
3. Add Required Context Tags only when a semantic restriction is needed, then provide them from a Room Sensor or Interior Location.
4. Set Priority and Spawn Chance.
5. Run `Build` after changing an Interior Actor class or its bounds.
6. Assign the database to the default Theme, a Zone override, or a Gameplay Role override.

## Related Pages

- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [UDungeonSubLevelDatabase.en.md](./UDungeonSubLevelDatabase.en.md)
