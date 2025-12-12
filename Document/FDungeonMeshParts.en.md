# FDungeonMeshParts Guide

FDungeonMeshParts is a **container that records which single static mesh to place and how to offset/rotate it** for pillars, walls, ceilings, and similar pieces. The placement is driven by the base transform data (FDungeonPartsTransform), while the look comes from the `StaticMesh` you assign.

## Typical use
- Express requests like “place this pillar 30 cm to the right and rotate it 90°” with a `StaticMesh` plus its offset/rotation.
- When you list multiple FDungeonMeshParts inside a mesh set ([UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.en.md)), generation can pick one based on random choice or selection rules.

## Key UPROPERTY fields
- **StaticMesh (`UStaticMesh*`, EditAnywhere/BlueprintReadWrite)**  
  Specifies the static mesh that will be spawned. Leave it empty and nothing appears, so always assign a mesh. Because the mesh pivot becomes the placement origin, adjust the pivot in the asset if you want easier alignment.

## Editing and placement tips
- Use the base Transform offset/rotation to fine-tune the position. Matching the grid size values (GridSize/VerticalGridSize in [UDungeonGenerateParameter](./UDungeonGenerateParameter.en.md)) keeps everything snapped.
- Register several meshes for the same role so the database selection rules can add variety during generation.
