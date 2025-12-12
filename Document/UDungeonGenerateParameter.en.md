# UDungeonGenerateParameter Guide

UDungeonGenerateParameter collects the settings for dungeon generation. This guide explains **the intent, default behavior, and edit conditions** for the Blueprint-exposed UPROPERTY fields in a way that non-engineers can tune the generator for their desired look and feel. Focus on how each option affects the player experience instead of code locations.

## Basics (reproducibility and debugging)
- **RandomSeed (`int32`)**: Seed for generation. Fix it to reproduce a layout; use 0 to randomize every run. `EditAnywhere`, `ClampMin=0`.
- **GeneratedRandomSeed (`int32`)**: The seed actually used in the last generation. Read-only reference when you want to recreate a found layout.
- **GeneratedDungeonCRC32 (`int32`)**: CRC32 of the last generated dungeon. Useful for version-to-version comparisons (read-only).

## Room size and spacing (exploration feel and visibility)
- **RoomWidth (`FInt32Interval`)**: Min/max room width. Smaller for corridor-like areas, larger for combat arenas. UI clamps to 1 or more.
- **RoomDepth (`FInt32Interval`)**: Min/max room depth. Raise it to create more elongated rooms. UI clamps to 1 or more.
- **RoomHeight (`FInt32Interval`)**: Min/max room height. Increase for vertical combat or sightlines. UI clamps to 1 or more.
- **RoomMargin (`uint8`)**: Horizontal gap between rooms. Wider margins reduce claustrophobic corridors; narrow margins increase density. Editable only when `MergeRooms` is off; min 1.
- **VerticalRoomMargin (`uint8`)**: Vertical gap between rooms. Helps avoid overlap in multi-layer layouts. Editable only when `MergeRooms` and `Flat` are off; min 0.

## Room/floor candidates and layout policy (overall map shape)
- **NumberOfCandidateRooms (`uint8`)**: Initial number of room candidates. More candidates mean longer generation but greater variety. Range 3–100.
- **MergeRooms (`bool`)**: Whether to merge adjacent rooms. Useful for large halls, but disables Margin and ExpansionPolicy edits when on.
- **ExpansionPolicy (`EDungeonExpansionPolicy`)**: Directional growth policy (horizontal, vertical, any). Enable vertical growth for multi-level mazes. Editable only when `MergeRooms` and `Flat` are off.
- **NumberOfCandidateFloors (`uint8`)**: Number of candidate floors to attempt. Raise to encourage multi-layer structures. Range 0–5; editable only when `Flat` is off.
- **Flat (`bool`)**: Force a single-floor layout. Handy for top-down prototypes; suppresses floor-related and vertical margin options.

## Player start and missions (core gameplay flow)
- **MovePlayerStartToStartingPoint (`bool`)**: Automatically moves PlayerStart to the chosen start room after generation for quick playtests.
- **StartLocationPolicy (`EDungeonStartLocationPolicy`)**: How the start room is picked (southernmost, highest, central, multi-start, etc.). Shapes the opening flow. `UseCentralPoint` and `UseMultiStart` require `UseMissionGraph` to be disabled. When using `UseMultiStart`, the generator creates as many start rooms as there are `PlayerStart` actors.
- **UseMissionGraph (`bool`)**: Enables key/mission generation. Leave off for free exploration; turn on for key hunts and controlled routes. Tends to be stable when `MergeRooms` is off and `AisleComplexity` is 0 (per comments).
- **AisleComplexity (`uint8`)**: Corridor complexity. Increase for maze-like feel. Editable only when both `MergeRooms` and `UseMissionGraph` are off; range 0–10. When `UseMissionGraph` is on, the getter forces this to 0.

## In-room structures (personality of each space)
- **GenerateSlopeInRoom (`bool`)**: Add slopes inside rooms for vertical accents and sight breaks (may become always-on in future).
- **GenerateStructuralColumn (`bool`)**: Add structural columns for cover and silhouette variety (may become always-on in future).

## Grid size (snap and movement baseline)
- **GridSize (`float`)**: Horizontal voxel size. Align to your mesh dimensions for clean snapping. `ClampMin=1`.
- **VerticalGridSize (`float`)**: Vertical voxel size. Match your jump height or stair slope for consistent feel. `ClampMin=1`.

## Mesh parts (art style and density)
- **DungeonRoomMeshPartsDatabase (`[UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.en.md)*`)**: Mesh parts DB for rooms. Swap to change the art theme.
- **DungeonAisleMeshPartsDatabase (`[UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.en.md)*`)**: Mesh parts DB for corridors. Use a separate set if you want distinct corridor style.
- **PillarPartsSelectionMethod (`EDungeonPartsSelectionMethod`)**: How pillar parts are picked (random, depth-based, etc.). Adjust to bias pillar variety.
- **PillarParts (`TArray<[FDungeonMeshParts](./FDungeonMeshParts.en.md)>`)**: Pillar mesh list. Swap to change silhouettes or cover options.

### Mesh parts and databases in context
- **[FDungeonMeshParts](./FDungeonMeshParts.en.md)**: Holds a single static mesh plus transform offset (inherits `FDungeonPartsTransform`). Simple for art teams to request exact placement/orientation for pillars or similar pieces.
- **[UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.en.md)**: Groups floor/wall/ceiling mesh sets with selectable policies (depth-based, random, etc.). Swap here to change the art theme per room or corridor.
- **[FDungeonRandomActorParts](./FDungeonRandomActorParts.en.md)**: Registers actors (e.g., a torch Blueprint) with spawn probabilities via `Frequency` (0.0–1.0). Great for occasional rare decorations.
- **[FDungeonDoorActorParts](./FDungeonDoorActorParts.en.md)**: Door actor parts referencing `DungeonDoorBase`-derived Blueprint classes plus offsets. Use to mix keyed doors or visual variants.
- **TorchPartsSelectionMethod (`EDungeonPartsSelectionMethod`)**: How torch parts are chosen. Tune to control wall decoration and visibility rhythm.
- **FrequencyOfTorchlightGeneration (`EFrequencyOfGeneration`)**: Torch spawn frequency. Lower for darker mood, higher for bright dungeons. Default `Rarely`.
- **TorchParts (`TArray<[FDungeonRandomActorParts](./FDungeonRandomActorParts.en.md)>`)**: Torch/lamplight actor list. Change flame color or shape to alter atmosphere.
- **DoorPartsSelectionMethod (`EDungeonPartsSelectionMethod`)**: Door parts selection method. Useful for highlighting important rooms.
- **DoorParts (`TArray<[FDungeonDoorActorParts](./FDungeonDoorActorParts.en.md)>`)**: Door variants to mix keyed doors or different looks.

## Product-only databases (`PRODUCT_ONLY`) (final polish)
- **DungeonInteriorDatabase** ([UDungeonInteriorDatabase](./UDungeonInteriorDatabase.en.md)): Interior placement DB. Swap to change furniture/decoration themes.
- **DungeonSubLevelDatabase** ([UDungeonSubLevelDatabase](./UDungeonSubLevelDatabase.en.md)): Start/goal and special sublevel DB. Use to inject event-specific levels.

## Room sensors (triggers and presentation)
- **DungeonRoomSensorClass (`UClass*`)**: Direct sensor class reference. Marked `DeprecatedProperty`; prefer the database for new setups.
- **DungeonRoomSensorDatabase ([UDungeonRoomSensorDatabase](./UDungeonRoomSensorDatabase.en.md))**: Database controlling room sensors and their effects (BGM, buffs, spawns, etc.).

### How to use the product-only databases
- [**UDungeonInteriorDatabase**](./UDungeonInteriorDatabase.en.md): Manages interior presets by tags and regenerates voxel placement data on Build. Swap themes quickly to test different moods.
- [**UDungeonSubLevelDatabase**](./UDungeonSubLevelDatabase.en.md): Registers sublevels for start/goal rooms or special events. Must match this parameter’s GridSize. Supports direct start/goal assignments plus capped random injections for secret rooms or scripted boss arenas.
- [**UDungeonRoomSensorDatabase**](./UDungeonRoomSensorDatabase.en.md): Draws from `DungeonRoomSensorBase`-derived classes with conditions by depth or identifiers. Includes an event to spawn extra corridor actors after generation, letting you centralize BGM changes, traps, or other room-entry effects.

## Extra info (troubleshooting)
- **PluginVersion (`uint8`)**: Plugin version, read-only. Useful for support and cross-environment checks.

### Edit-condition reminders
- Enabling `MergeRooms` disables editing of `RoomMargin`, `VerticalRoomMargin`, and `ExpansionPolicy`.
- Enabling `Flat` disables floor-related options (`NumberOfCandidateFloors`) and vertical margins.
- When `UseMissionGraph` is enabled, `GetAisleComplexity` returns 0, effectively ignoring the complexity setting.

Use this guide when adjusting dungeon generation parameters in Blueprint to understand how each option shapes the resulting layout and play experience.
