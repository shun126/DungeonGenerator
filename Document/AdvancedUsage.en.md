# Dungeon Generator Advanced Usage Guide

This document explains how to configure the following requests:

- Adjustable aisle ceiling height
- Add chandeliers to DungeonMeshSetDatabase
- Custom selection of mesh sets and parts
- Generate dungeons connected to a lobby

## 1. Configure aisle ceiling height

Use **AisleCeilingHeightPolicy** in `UDungeonGenerateParameter` to control aisle ceiling height.

- `TwoGrids`: Always 2-grid ceiling
- `OneGrid`: Always 1-grid ceiling
- `Random`: Randomly choose 1-grid or 2-grid ceiling

### Steps
1. Open your `DungeonGenerateParameter` asset.
2. Set `AisleCeilingHeightPolicy` to the desired mode.
3. Regenerate the dungeon and verify aisle heights.

## 2. Add chandeliers to DungeonMeshSetDatabase

Chandelier entries are managed in each `FDungeonMeshSet`. During generation, a mesh set is selected first, then chandelier actor parts are selected from chandelier candidates.

### Steps
1. Open a `UDungeonMeshSetDatabase` asset.
2. Choose the target `FDungeonMeshSet` in `Parts`.
3. Add chandelier candidates (`FDungeonRandomActorParts`).
4. Tune chandelier selection policy/method as needed.

### Tips
- Prefer smaller chandeliers in low-ceiling areas to avoid overlap.
- Split mesh sets by depth to make deeper areas feel more luxurious.

## 3. Custom selection for mesh sets and parts

### 3-1. Custom mesh-set selection
Implement `SelectMeshSetIndex` (BlueprintNativeEvent) in `UDungeonGenerateParameter` to return a chosen mesh-set index from candidates.

- Called when `SelectionMethod = Custom`.
- Out-of-range return values fall back to built-in selection.

### 3-2. Custom parts selection
Use parts selectors (e.g., `UDungeonSamplePartsSelector`) and query context (`FPartsQuery` / `FMeshSetQuery`: depth, RoomId, neighbor mask, etc.) to build your own rules.

## 4. Generate dungeons connected to a lobby

To build a stable “Lobby -> Procedural Dungeon” flow, combine start-location policy and sublevel composition.

### Recommended setup
1. Place `PlayerStart` and transition trigger in the lobby.
2. Configure start policy (`StartLocationPolicy`) in `UDungeonGenerateParameter`.
3. If needed, prepare fixed connection rooms via Start/Goal settings in `UDungeonSubLevelDatabase`.
4. Run dungeon generation on lobby transition, then spawn/move players into the generated start room.

### Notes
- For multi-spawn patterns, use `UseMultiStart`.
- For guaranteed entrance staging, register a required sublevel room.
