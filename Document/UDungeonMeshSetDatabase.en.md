# UDungeonMeshSetDatabase Guide

UDungeonMeshSetDatabase **manages collections of mesh sets (themes) and chooses which set to use during generation**. Group FDungeonMeshParts into themed sets—like stone for 1F and rusty metal for B1F—and let the database select them based on depth or randomness.

## Typical use
- Build several mesh themes in Parts (e.g., stone floors/walls/ceilings for early levels, metal variants for deeper floors).
- Use SelectionMethod to decide how a set is chosen: by depth weighting, or simple randomness.
- Assign this asset to the “Room/Corridor Mesh Parts DB” fields in [UDungeonGenerateParameter](./UDungeonGenerateParameter.en.md) so generation references it.

## Key UPROPERTY fields
- **SelectionMethod (`EDungeonMeshSetSelectionMethod`, EditAnywhere/BlueprintReadOnly)**  
  How to choose a mesh set. Options include depth-aware weighting or pure random. If unsure, start with “DepthFromStart” to swap themes the farther you go.
- **Parts (`TArray<FDungeonMeshSet>`, EditAnywhere/BlueprintReadOnly)**
  The list of actual mesh sets. Each FDungeonMeshSet contains [FDungeonMeshParts](./FDungeonMeshParts.en.md) for floor, wall, ceiling, and other slots; once a set is chosen, those parts supply the meshes.

## Editing tips
- The meaning of order can change with the SelectionMethod. Use “DepthFromStart” for progression-based themes, or a random option if you just want variety.
- Add multiple sets and multiple parts within each set to keep rooms visually varied even inside the same theme.
