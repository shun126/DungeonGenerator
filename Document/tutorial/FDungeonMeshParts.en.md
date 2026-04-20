# FDungeonMeshParts Reference

`FDungeonMeshParts` is the container that records where and in which direction a single static mesh should be placed, for example a pillar or a wall panel. Placement comes from the offset data in the base class (`FDungeonPartsTransform`), while the visual appearance is changed by swapping `StaticMesh`. This is intended for level designers and artists who want to register their own meshes and switch dungeon parts by theme.

## Main usage
- Express requests such as "place this pillar mesh 30 cm to the right and rotate it 90 degrees" through `StaticMesh` plus offsets.
- When multiple `FDungeonMeshParts` entries are listed in a mesh set ([UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)), generation can choose among them randomly or by rule.

## What the UPROPERTY means
- **StaticMesh (`UStaticMesh*`, EditAnywhere/BlueprintReadWrite)**  
  The static mesh spawned during generation. If this is unset, nothing is placed, so always assign it. The mesh pivot becomes the placement reference, so adjusting the pivot on the mesh asset itself makes positioning easier.

## Editing and placement tips
- You can adjust offset and rotation in the base-class transform settings. Values that match `GridSize` and `VerticalGridSize` in [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md) make snapping more consistent.
- Registering multiple meshes for the same role makes it easy to add variation through database-side selection rules.

