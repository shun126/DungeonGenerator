# Using the StaticMesh Fit Tool

This page explains how to use the `StaticMesh Fit` tool to prepare Static Mesh assets for the Dungeon Generator grid.  
Use it before registering your own floors, walls, roofs, slopes, and pillars in a `Mesh set database`.

## Goal
- Check whether selected Static Mesh assets match the dungeon grid size
- Generate fitted Static Mesh copies when needed
- Register the generated meshes in a `Mesh set database`

## What the tool does
The StaticMesh Fit tool checks the Bounds of selected Static Mesh assets against the `Grid Size` and `Vertical Grid Size` from a `DungeonGenerateParameter`.

The tool does not modify the original Static Mesh assets.  
When you run `Generate Fitted Meshes`, it creates new Static Mesh assets in the selected output folder.

Use it to check:

- Whether a floor or roof is one grid wide
- Whether a wall is one grid wide and one vertical grid high
- Whether a slope runs two grids forward and rises by one vertical grid
- Whether a pillar matches the vertical grid height
- Whether a 90-degree rotation would make the mesh fit better

## What you need first
- The Static Mesh assets you want to check
- A `DungeonGenerateParameter` asset
- A Content Browser folder for the generated assets

The `DungeonGenerateParameter` is required because the tool reads the grid dimensions from it.  
If you have not created one yet, follow [QuickStart.en.md](./QuickStart.en.md) first.

## 1. Select Static Mesh assets
In the Content Browser, select one or more Static Mesh assets.  
Multiple selected meshes can be checked with the same settings.

## 2. Open Fit Check
Right-click the selected Static Mesh assets and open:

`Dungeon Generator > Fit Check`

The `StaticMesh Fit Settings` dialog appears.

## 3. Review the settings
The dialog contains the following settings.

### `DungeonGenerateParameter`
Select the `DungeonGenerateParameter` used as the grid reference.  
Its `Grid Size` and `Vertical Grid Size` become the fit targets.

### `Output Directory`
The Content Browser folder where generated Static Mesh assets are saved.  
The default is `/Game/DungeonFittedMeshes`.

### `Enable Grid Fit`
When enabled, the tool applies the selected rotation and scale correction.  
When disabled, the tool only duplicates the selected meshes without rotation or scale correction.

### `Fit Mode`
- `Axis-wise`  
  Fits X/Y/Z independently. This is easier for exact grid fitting, but it can change the proportions of the mesh.
- `Uniform`  
  Uses one scale value for the whole mesh. This preserves proportions more easily, but some axes may remain slightly off the grid.

If you are unsure, start with `Axis-wise`.  
For decorative meshes where proportions matter more, also try `Uniform`.

### `Tolerance (cm)`
The remaining size error allowed for a `Pass` result.  
The default is `1 cm`. Smaller values make the check stricter.

### `Max Correction (%)`
The maximum allowed scale correction.  
The default is `50%`. Meshes that require more correction are marked as `Fail`.

### `Per-Mesh Fit Rules`
Choose how each mesh should be evaluated.

- `Auto`  
  Chooses the closest rule from the mesh Bounds.
- `Floor`  
  Fits X/Y to `Grid Size`.
- `Wall`  
  Fits X to `Grid Size` and Z to `Vertical Grid Size`.
- `Roof`  
  Fits X/Y to `Grid Size`.
- `Slope`  
  Fits X to `Grid Size`, Y to `Grid Size * 2`, and Z to `Vertical Grid Size`.
- `Pillar`  
  Fits Z to `Vertical Grid Size`.

`Auto` is designed to prefer `Floor` for flat slab-like meshes.  
If a mesh is meant to be a roof, manually select `Roof` when needed.

## 4. Run the check
After reviewing the settings, press `Start Fit Check`.  
The result dialog shows one row per mesh.
Use `Back` if the result is not what you expected. It returns to the settings dialog without generating meshes, so you can adjust the rules or tolerance and run the check again.

The main columns are:

- `Select`  
  Whether this row will be generated.
- `Mesh Name`  
  The checked Static Mesh name.
- `Rule`  
  The fit rule that was used. For `Auto`, the result is shown like `Auto -> Floor`.
- `Verdict`  
  `Pass`, `Review`, or `Fail`.
- `Rotation`  
  The recommended yaw rotation. It is chosen from 0, 90, 180, and 270 degrees.
- `Original Size -> Target Size`  
  The original Bounds size and the fitted target size.
- `Recommended Scale`  
  The scale applied when generating the fitted copy.
- `Reason`  
  Why the row received that verdict.

## Understanding verdicts
### `Pass`
The result is inside the tolerance.  
These rows are selected by default.

### `Review`
The correction is within the maximum correction value, but the remaining error is above `Tolerance (cm)`.  
Select these rows manually only after checking that the result is acceptable.

### `Fail`
The required correction is above `Max Correction (%)`.  
These rows cannot be generated. Review the original mesh size, orientation, or selected rule.

## 5. Generate fitted meshes
Check the `Select` boxes for the rows you want to generate, then press `Generate Fitted Meshes`.  
The generated Static Mesh assets are saved to the selected `Output Directory`.

Generated names are based on the source names:

- If the source name starts with `SM_`  
  Example: `SM_Wall` becomes `SM_Wall_DNG`
- If the source name does not start with `SM_`  
  Example: `Wall` becomes `SM_Wall_DNG`

If the name already exists, Unreal Engine creates a unique name.

## 6. Register the meshes in a Mesh Set Database
Register the generated Static Mesh assets in the room or aisle `Mesh set database`.

- Floors go in `Floor Parts`
- Walls go in `Wall Parts`
- Roofs go in `Roof Parts`
- Slopes go in `Slope Parts`
- Pillars go in a pillar setting such as `Pillar Parts` on `UDungeonGenerateParameter`

After registration, use `Window > DungeonGenerator`, run `Verify`, then `Generate dungeon` to confirm that floors, walls, roofs, and slopes connect cleanly.

## Notes
- Generated fitted meshes are recentered at the correct origin: floors, walls, slopes, and pillars use the bottom center, and roofs use the top center. The original mesh asset is not changed.
- Zero thickness is allowed only on axes that are not fitted by the selected rule. Required fit axes must have a size.
- This tool does not rebuild the visual design or UVs. Large scale corrections can change texture density or proportions.
- Always check collision after generation. Configure collision on the generated Static Mesh if needed.
- If you use a `Review` result, preview the dungeon and inspect the connections between floors and walls.

## Common mistakes
- `Start Fit Check` cannot be pressed because no `DungeonGenerateParameter` is selected
- The `Output Directory` is not under `/Game`
- A floor mesh is classified as a wall or pillar by `Auto`  
  Manually select `Floor` and run the check again.
- A roof mesh is shown as `Auto -> Floor`  
  Manually select `Roof` and run the check again.
- You are trying to generate a `Fail` row  
  `Fail` rows cannot be generated. Fix the source mesh or selected rule first.

## Read Next
- [PrepareMeshParts.en.md](./PrepareMeshParts.en.md)
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)
- [FDungeonMeshParts.en.md](./FDungeonMeshParts.en.md)
