# UDungeonMeshSetDatabase Guide

`UDungeonMeshSetDatabase` is a visual theme collection that groups floors, walls, roofs, slopes, catwalks, and chandeliers.  
You usually prepare separate databases for rooms and aisles, then reference them from `UDungeonGenerateParameter`.

## Role
- Decide which `FDungeonMeshSet` is used
- Decide which floor, wall, roof, and other parts are used inside the selected `FDungeonMeshSet`

In other words, Phase 1 is the database deciding "which theme to use", and Phase 2 is the mesh set deciding "which part inside that theme to use".

## Minimum usage
1. Create a `Mesh set database` asset.
2. Add one `Mesh Set`.
3. At minimum, add these parts to it:  
   `Floor Parts` / `Wall Parts` / `Roof Parts`
4. Assign that database as the room or aisle database in `UDungeonGenerateParameter`.

## Main properties
- `Mesh Set Selection Policy` (`SelectionPolicy`)  
  Decides which `Mesh Set` is chosen from this database.
- `Custom Mesh Set Selector`  
  A custom selector used only when `Mesh Set Selection Policy = Custom Selector`.
- `Mesh Set` (`Parts`)  
  The actual theme list. Each item is an `FDungeonMeshSet`.

## How to think about `Mesh Set Selection Policy`
- `Random`  
  Good when you want variation within the same theme band.
- `Identifier`  
  Chooses deterministically from the room identifier.
- `Depth From Start`  
  Useful when you want the theme to change with progression from the start.
- `Custom Selector`  
  Use this when you want fully custom logic for choosing a mesh set.

In the current setup, focus on **`SelectionPolicy`**, not `SelectionMethod`.  
`SelectionMethod` is kept only for legacy asset compatibility.

## What each `FDungeonMeshSet` decides
Each `FDungeonMeshSet` contains arrays and selection policies such as:

- `Floor Parts`
- `Wall Parts`
- `Roof Parts`
- `Slope Parts`
- `Catwalk Parts`
- `Chandelier Parts`

Chandeliers are managed inside each mesh set, not at the database-wide level.  
If you want different levels of luxury by progression depth, splitting mesh sets is usually easier to manage.

## Configuring chandeliers
Chandeliers are not a shared setting of `UDungeonMeshSetDatabase`. They are decoration settings on each `FDungeonMeshSet`.

### Steps
1. Open `UDungeonMeshSetDatabase`.
2. Select the target `Mesh Set`.
3. Add `FDungeonRandomActorParts` entries to `Chandelier Parts`.
4. Adjust the following settings if needed:
   - `ChandelierPartsSelectionPolicy`
   - `ChandelierMinSpacing`
   - `ChandelierMinCeilingHeight`
   - `ChandelierRadius`
   - `ChandelierWallWeight`
   - `ChandelierCombatWeight`

### What the main chandelier settings mean
- `Chandelier Parts`  
  Candidate chandeliers that can actually be placed. Configure `ActorClass` and spawn behavior in `FDungeonRandomActorParts`.
- `ChandelierPartsSelectionPolicy`  
  Decides which chandelier candidate is picked when multiple entries exist.
- `ChandelierMinSpacing`  
  Minimum spacing between chandeliers. Increase it if they appear too dense.
- `ChandelierMinCeilingHeight`  
  Minimum required ceiling height. This matters when you want to avoid collisions in low-ceiling themes.
- `ChandelierRadius`  
  Collision-check radius during placement. Larger decorations usually need a larger value.
- `ChandelierWallWeight` / `ChandelierCombatWeight`  
  Control how strongly candidates farther from walls or closer to combat-center-like positions are preferred.

### Practical tips
- In low-ceiling themes, increase `ChandelierMinCeilingHeight` to reduce collisions.
- If deeper floors should look more luxurious, create a separate mesh set that includes chandeliers.
- If room visuals and aisle visuals should differ, separate them by mesh set rather than trying to solve it at database level.

## Editing tips
- Managing room and aisle databases separately makes visual tuning much easier.
- Start with a single `Mesh Set` and add only the minimum parts first. Add additional sets only after that base set works.
- `Verify` catches cases where there is no floor, wall, or roof mesh at all.
- If you use `Custom Selector`, keep the logic lightweight and deterministic.

## Read Next
- [FDungeonMeshParts.en.md](./FDungeonMeshParts.en.md)  
  Review the per-mesh entry settings in detail.
- [CustomSelector.en.md](./CustomSelector.en.md)  
  Review how to replace selection rules with custom selectors.

## Related Pages
- [FDungeonMeshParts.en.md](./FDungeonMeshParts.en.md)
- [FDungeonRandomActorParts.en.md](./FDungeonRandomActorParts.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [CustomSelector.en.md](./CustomSelector.en.md)

