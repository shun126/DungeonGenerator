# Runtime Load Reduction

Large runtime dungeons can contain many enemies, lights, decorations, and collision components.
If every actor stays fully active all the time, the game can become heavy even when the player is far away from most rooms.

Dungeon Generator provides a load-control system for this case.
`ADungeonMainLevelScriptActor` divides the generated dungeon into partitions, and `UDungeonComponentActivatorComponent` pauses or restores selected actors when their partition moves away from the player.

![Load reduction overview](images/LoadReduction1.jpg)

## What This System Can Reduce
Use load control for actors that are expensive when they are far from the player.

- Enemy actors and NPCs
- Decorative actors with Tick
- Moving props and effects
- Point lights and spotlights
- Interactable actors that do not need to work outside the nearby area
- Collision-heavy actors

Do not use it on actors that must always work, such as the player, game managers, global audio managers, save systems, or UI-related actors.

## Set Up the Main Level
Use `ADungeonMainLevelScriptActor` as the Level Script Actor for the level that owns the runtime dungeon.

This actor manages the partitions for the level. During play, it checks the player position, decides which partitions should be active, and updates registered `UDungeonComponentActivatorComponent` instances.

The most important settings are:

- `bEnableLoadControl`
  Enables the PVS-based partition load-control system. Keep this enabled for large runtime dungeons.
- `ActivationRangeScale`
  Scales the active range around the player. Increase this value if actors appear too late or disappear too close to the player.
- `PartitionGridCountOverride`
  Overrides the automatic partition size. Leave each axis at `0` unless you need advanced tuning.
- `PrecomputedVisibilityDilationHopCount`
  Expands PVS-visible partitions through neighboring partitions. Increase this if you see pop-in near corners, doors, or vertical transitions.
- `MaxPartitionActivationsPerFrame`
  Limits how many partitions can become active in one frame. Lower values reduce frame spikes but make activation spread over more frames.
- `MaxPartitionInactivationsPerFrame`
  Limits how many partitions can become inactive in one frame. Lower values reduce frame spikes when leaving a large area.
- `MaxShadowCastingPointAndSpotLights`
  Limits the number of point lights and spotlights that cast shadows. Use this when many local lights are placed in the dungeon.
- `ShowDebugInformation`
  Shows partition debug information in the editor.

![Partition settings](images/LoadReduction2.jpg)

## Add Activator Components
Add `UDungeonComponentActivatorComponent` to actor Blueprints that should sleep when they are far from the player.

For each actor, choose which parts can be controlled:

- `EnableOwnerActorTickControl`
  Saves the actor Tick state and disables Tick while the partition is inactive.
- `EnableOwnerActorAiControl`
  Stops AI logic while the partition is inactive. Disable this for actors that must keep thinking from far away.
- `EnableComponentActivationControl`
  Disables component activation states and restores them later.
- `EnableComponentVisibilityControl`
  Hides components while inactive.
- `EnableLightShadowControl`
  Controls Cast Shadow for point lights and spotlights. Only lights that had Cast Shadow enabled at BeginPlay are controlled.
- `EnableCollisionEnableControl`
  Disables collision while inactive. Disable this for actors that must block or receive traces even when they are far away.

`OnPartitionActivate` and `OnPartitionInactivate` are optional Blueprint events.
Use them only when you need custom behavior, such as restarting a particle effect, refreshing a UI marker, or pausing a special animation.
Avoid heavy initialization inside these events, because it can create a spike when many actors become active.

![Activator component](images/LoadReduction3.jpg)

## Practical Setup Examples
- Enemies
  Enable Tick and AI control. Keep collision control enabled if enemies should not block the player while inactive. Disable AI control if the enemy must continue long-range behavior.
- Decorative props
  Enable component activation and visibility control. Collision control is useful for props the player cannot reach while far away.
- Lights
  Enable visibility and light shadow control. Use `MaxShadowCastingPointAndSpotLights` to prevent too many local lights from casting shadows at once.
- Interactable actors
  Use Tick and visibility control carefully. Keep collision enabled if line traces or interaction checks must still work.

## Reducing Generation-Time Spikes
Load control helps after the dungeon exists.
For generation-time spikes, also check vegetation and mesh settings.

- `Theme.bDeferredVegetationSpawn`
  Spawns vegetation over multiple frames instead of placing all vegetation in one frame.
- `Theme|VegetationPerformance`
  Tune the number of vegetation candidates, foliage tree builds, and time budget processed per frame.
- Instanced Mesh Cull Distance
  Use cull-distance settings for static visual detail that does not need to render far away.

## Troubleshooting
- A nearby actor disappears.
  Increase `ActivationRangeScale` or `PrecomputedVisibilityDilationHopCount`, then check whether the actor is registered in the expected partition.
- A light is visible, but its shadow disappears.
  Increase `MaxShadowCastingPointAndSpotLights`, or reduce the number of shadow-casting point lights and spotlights.
- Enemy AI stops unexpectedly.
  Disable `EnableOwnerActorAiControl` for that enemy Blueprint.
- Moving between rooms causes a hitch.
  Lower `MaxPartitionActivationsPerFrame` and `MaxPartitionInactivationsPerFrame`. Also avoid heavy work in `OnPartitionActivate`.
- Far static meshes are still expensive.
  Combine this page with instanced mesh culling and `Theme.bDeferredVegetationSpawn`.

## Useful Blueprint Hooks
`ADungeonMainLevelScriptActor` provides two Blueprint events around generation:

- `OnPreDungeonGeneration`
  Called before dungeon generation. Use this for loading screens or temporary UI.
- `OnPostDungeonGeneration`
  Called after dungeon generation. Use this to start gameplay, hide loading UI, or run lightweight setup.

If your custom flow changes the traversable dungeon layout after generation, call `RebuildSparsePartitionGraphAndRefresh()` so the partition graph and activator registrations are updated.
