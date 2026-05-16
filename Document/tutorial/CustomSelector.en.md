# Custom Selector Guide

`Custom Selector` is the mechanism used when `Random`, `Identifier`, or `Depth From Start` are not enough and you want to replace the selection rule with a `UDungeonPartsSelector`-derived asset.

## When to use it
- You want visual choices to react to surrounding room shape or progression depth
- You want deterministic weighted selection so server and client stay aligned
- You want theme switching that is hard to express with built-in policies

## Basic flow
1. Create a Blueprint or C++ class derived from `UDungeonPartsSelector`.
2. Implement `SelectMeshSetIndex` or `SelectPartsIndex`.
3. Change the target `Selection Policy` to `Custom Selector`.
4. Assign your selector asset to the corresponding selector property.

## Choosing mesh sets with a custom rule
Set up `UDungeonMeshSetDatabase` like this:

1. `Mesh Set Selection Policy = Custom Selector`
2. Assign a `UDungeonPartsSelector`-derived asset to `Custom Mesh Set Selector`

`SelectMeshSetIndex` is then called with `FMeshSetQuery` and the number of candidates, and should return the chosen mesh-set index.

## Choosing individual parts with a custom rule
Floors, walls, roofs, slopes, catwalks, chandeliers, pillars, torches, and doors can also use `Custom Selector`.

- On `FDungeonMeshSet` for floor / wall / roof / slope / catwalk / chandelier  
  Set the corresponding `*PartsSelectionPolicy` to `Custom Selector` and assign `Custom Mesh Parts Selector`.
- On `UDungeonGenerateParameter` for pillars / torches / doors and similar fixtures  
  Set the corresponding `*SelectionPolicy` to `Custom Selector` and assign `Custom Dungeon Parts Selector`.

In that case, `SelectPartsIndex` is called with `FPartsQuery` and the candidate count, and should return the final part index.

## Using the sample selector
`UDungeonSamplePartsSelector` is included as a sample implementation.

- `FMeshSetQuery`  
  The query passed when selecting a mesh set.
- `FPartsQuery`  
  The query passed when selecting an individual part. The sample includes deterministic rules using `NeighborMask6` and `SeedKey`.

A practical starting point is to duplicate the sample and add conditions gradually.

## Implementation cautions
- Selectors run on the generation hot path on the game thread
- Avoid heavy processing, asynchronous work, and expensive external lookups
- Avoid unsynchronized randomness or time-dependent branching because they can create server/client divergence
- Keep the return value in the range `0` through `NumCandidates - 1`

## Difference from the legacy approach
`UDungeonGenerateParameter::SelectMeshSetIndex` still exists, but in the current setup it is mainly a legacy-compatibility path.  
For new content, selector assets make it much easier to understand where the selection rule is defined.

## Read Next
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)  
  Review mesh-set policies and chandelier settings.
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)  
  Apply these advanced settings to the runtime generation actor.

## Related Pages
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)

