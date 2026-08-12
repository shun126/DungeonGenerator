# Custom Selector Guide

Selectors decide which visual candidate is used during generation. Version 2 exposes selector objects directly: one kind chooses a mesh set, and another chooses a part inside that mesh set.

## Choose the correct selector type

- Derive a Blueprint from `UDungeonBlueprintMeshSetSelector` when you need to choose an `FDungeonMeshSet` from a `UDungeonMeshSetDatabase`. Implement `Select Mesh Set Index`.
- Derive a Blueprint from `UDungeonBlueprintPartsSelector` when you need to choose a floor, wall, roof, slope, catwalk, chandelier, pillar, torch, or door candidate. Implement `Select Parts Index`.

`UDungeonPartsSelector` is an include that collects selector types for C++; it is not the Blueprint parent class to select.

## Where to assign selectors

For a whole mesh set, open the `Mesh set database` and set `Mesh Set Selector`.

For a part inside an `FDungeonMeshSet`, use the matching property:

- `Floor Parts Selector`
- `Wall Parts Selector`
- `Roof Parts Selector`
- `Slope Parts Selector`
- `Catwalk Parts Selector`
- `Chandelier Parts Selector`

For shared fixtures, use `Theme.Fixtures.Pillar Parts Selector`, `Torch Parts Selector`, `Door Parts Selector`, or `Unique Door Parts Selector` in `UDungeonGenerateParameter`.

These are inline selector objects. If a selector is left empty, the plugin assigns Uniform Random automatically. You do not need to change a separate Selection Policy.

## Built-in choices

Mesh-set selectors include Uniform Random, Identifier, and Depth From Start. Parts selectors include Uniform Random, Grid Index, and Direction. Start with a built-in selector when it expresses the rule you need; use a Blueprint selector for project-specific conditions.

## Blueprint flow

1. Create a Blueprint class derived from `UDungeonBlueprintMeshSetSelector` or `UDungeonBlueprintPartsSelector`.
2. Override `Select Mesh Set Index` or `Select Parts Index`.
3. Inspect `Query` and return an index from `0` through `NumCandidates - 1`.
4. Select that Blueprint selector in the appropriate inline selector property above.
5. Generate several fixed seeds and confirm that the result is stable.

An out-of-range result produces a warning and falls back to Uniform Random, so generation can continue. Treat that warning as a selector bug rather than relying on the fallback.

## Query values

`FDungeonMeshSetQuery` is passed when choosing a mesh set. `FDungeonPartsQuery` is passed when choosing an individual part.

Both queries provide local dungeon grid coordinates such as `GridX`, `GridY`, and `GridZ`, plus a deterministic `SeedKey`. They are not Unreal world-space positions. A parts query also describes details such as the target kind, rotation, and neighboring cells, which can be used for topology-aware rules.

Use `Query.SeedKey` when a Blueprint rule needs deterministic variation. Avoid wall-clock time, unsynchronized random nodes, asynchronous work, or external lookups: selectors run frequently during generation and the same inputs must produce the same decision.

## Samples

The plugin includes `UDungeonSampleMeshSetSelector` and `UDungeonSamplePartsSelector`, plus the Blueprint samples `BP_SampleDungeonMeshSetSelector` and `BP_SampleDungeonPartsSelector`. They demonstrate deterministic weighted choices and query-based rules. Copy the idea you need and add conditions gradually.

## Version 1 note

Hidden policy and legacy selector fields may still exist in serialized implementation data, but they are not the Version 2 authoring workflow and do not provide supported Version 1-to-Version 2 migration. Recreate the selector setup using the visible Version 2 selector properties.

## Related Pages

- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)
