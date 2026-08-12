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
  Limits the number of visible point lights and spotlights that cast shadows. Overflow lights fade out instead of remaining visible without shadows, preventing light from leaking through walls. Set it to `0` for unlimited lights.
- `ShowDebugInformation`
  Shows partition debug information in the editor.

![Partition settings](images/LoadReduction2.jpg)

### Generated ISM/HISM Terrain

Generated floors, walls, roofs, and pillars that use ISM or HISM are divided into fixed `8 × 8 × 2` dungeon-grid XYZ spatial groups. After the dungeon partitions are built, each group is linked to every partition crossed by its mesh bounds.

The group remains visible while at least one linked partition is active and becomes hidden only after all linked partitions are inactive. Distance culling still applies, so both systems work together. Partition visibility does not disable terrain collision or remove it from navigation; it only reduces rendering work.

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
  Add an activator to non-static point and spot lights that should participate in runtime light control. Use `MaxShadowCastingPointAndSpotLights` to limit how many remain visible with shadows at once.
  Automatically generated aisle-slope base lights use dedicated actors with visibility control only. They do not participate in the shadow budget because they never cast shadows. Each actor registers with the partition containing its slope.
- Interactable actors
  Use Tick and visibility control carefully. Keep collision enabled if line traces or interaction checks must still work.

## Reducing Generation-Time Spikes
Load control helps after the dungeon exists.
For generation-time spikes, also check vegetation and mesh settings.

- `GenerationPerformance.ActorSpawn`
  Distributes Actor spawning, including Room Sensor enemies and keys, across frames. `MaxSpawnRequestsPerFrame` limits requests and `MaxSpawnTimeMs` limits processing time per frame. Use `0` for either unlimited value.
- `GenerationPerformance.Vegetation`
  Stores compact placement jobs instead of creating every candidate at generation start. Each frame materializes, traces, and places candidates near the player camera first. Candidate count, placement time, foliage tree count, and tree-build time have separate limits; `0` means unlimited for every limit. Inactive groups continue generating in the background.
- `bUseDeferredSpawn`
  Actor Spawn and Vegetation have independent switches. Disabling one completes that work synchronously. Static Editor Generate always completes synchronously regardless of these settings, while PIE uses the configured distribution.
- Instanced Mesh Cull Distance
  Use cull-distance settings for static visual detail that does not need to render far away.

## Troubleshooting
- A nearby actor disappears.
  Increase `ActivationRangeScale` or `PrecomputedVisibilityDilationHopCount`, then check whether the actor is registered in the expected partition.
- A room light disappears when viewed from outside the room.
  With a normally closed door, this is rarely a problem because the room lights become active by the time the door opens.
  However, when the room interior is visible from outside through an open doorway, grate, window, or similar opening, the room and aisle can have different Identifiers, allowing facing-angle culling to apply to the light.
  If the light disappears depending on its placement direction, increase `PointAndSpotLightTurnOnAngle` and `PointAndSpotLightTurnOffAngle`. To isolate the cause, temporarily set both values to `180` degrees.
  If the light still disappears, check `ActivationRangeScale` and `PrecomputedVisibilityDilationHopCount`.
- A point light or spot light fades out near other lights.
  It may be outside `MaxShadowCastingPointAndSpotLights`. Increase the limit, set it to `0` for unlimited lights, or reduce the number of shadow-casting point lights and spotlights.
- An aisle-slope light leaks into a nearby aisle or floor.
  Reduce `Theme.AisleSlopeBaseLight.AttenuationRadius`. Enable the Zone's `bOverrideAisleSlopeBaseLight` when different areas need different values.
- Enemy AI stops unexpectedly.
  Disable `EnableOwnerActorAiControl` for that enemy Blueprint.
- Moving between rooms causes a hitch.
  Lower `MaxPartitionActivationsPerFrame` and `MaxPartitionInactivationsPerFrame`. Also avoid heavy work in `OnPartitionActivate`.
- Far static meshes are still expensive.
  Combine this page with instanced mesh culling and `GenerationPerformance.Vegetation.bUseDeferredSpawn`.

## Useful Blueprint Hooks
`ADungeonMainLevelScriptActor` provides two Blueprint events around generation:

- `OnPreDungeonGeneration`
  Called before dungeon generation. Use this for loading screens or temporary UI.
- `OnPostDungeonGeneration`
  Called after dungeon generation. Use this to start gameplay, hide loading UI, or run lightweight setup.

Bind to the dungeon generator's `OnGenerationSuccess` event when gameplay may start after every gameplay-required deferred Actor has been processed. Bind to `OnGenerationComplete` when the loading screen must remain until all visual work, including distant vegetation and foliage trees, is complete. `IsGenerationComplete()` provides the same final state for polling.

Room Sensors queue their configured enemies, keys, and Unique Keys automatically. Custom Blueprint logic can call `RequestDeferredSpawnActorFromClass` to include another Actor in the same generation queue and receive it through the completion callback. The existing `SpawnActorFromClass` remains an immediate spawn helper for compatibility.

If your custom flow changes the traversable dungeon layout after generation, call `RebuildSparsePartitionGraphAndRefresh()` so the partition graph and activator registrations are updated.
