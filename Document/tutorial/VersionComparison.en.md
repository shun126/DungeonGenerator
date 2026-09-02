# Dungeon Generator v1.x and v2.0.0 Comparison

Dungeon Generator v2.0.0 is a major update designed to create more than randomly connected rooms and corridors. It focuses on **playable progression, visual variety across the dungeon, and settings that are easier to tune**.

> **Warning — no v1.x migration support:** v2.0.0 contains breaking changes and does not support upgrading or converting v1.x projects or assets. Keep your v1.x project and plugin version intact. To use v2.0.0, create and configure the Dungeon Generator setup again for v2 in a separate project or branch.

Some source content, such as compatible meshes, may be reusable after manual review, but v1.x Dungeon Generator assets, parameters, Blueprints, and C++ integrations are not migrated automatically. The generation algorithm and parameter structure have changed, so the same seed and similar settings are not expected to reproduce a v1.x layout.

## Main differences

| Area | v1.x | v2.0.0 | What v2 provides |
| --- | --- | --- | --- |
| Level design | Individually tune room counts, sizes, floors, corridor complexity, and related values | Start with a `Progression Policy` that coordinates the main route, branches, loops, start, and goal | Choose a layout based on how it should play, such as free exploration, keys and locks, or a route toward a boss |
| Layout candidates | Generate a layout from the selected conditions | Evaluate multiple candidates and select the better-scoring layout | Balance candidate count against generation cost while making the result easier to steer |
| Room purpose | Use route flags and sensor data to classify rooms in project logic | Assign Gameplay Roles such as `Combat`, `Treasure`, `Puzzle`, `Rest`, `Boss`, and `Secret` | Connect enemies, rewards, rest points, and events to an explicit room purpose |
| Zones | Primarily use shared generation settings and databases | Select Zones by progress and floor, then override visual and Gameplay settings | Give entrances, deeper areas, and upper floors distinct themes |
| Enemy placement | Configure behavior mainly through Room Sensors and their database | Read generated room information, Gameplay Role, Structural Role, and Zone from a Room Sensor | Tune enemy counts and events according to a room's purpose and route position |
| Interiors and vegetation | Place furniture, decoration, and vegetation through an Interior Database | Override Interiors and Fixtures by Role or Zone; add deferred vegetation generation and clustering | Create interiors suited to combat rooms, treasure rooms, and secrets while tuning generation cost |
| Fixtures | Select pillars, torches, doors, and similar fixtures from shared settings | Add Role／Zone Fixture Overrides and doors intended for Unique Locks | Change fixed decoration and doors to match the location and gameplay purpose |
| Mesh／Parts selection | Use Random, Identifier, Depth From Start, and the legacy custom selection path | Unify selection through Mesh Set Selectors and Parts Selectors, extendable in Blueprint or C++ | Make selection rules easier to understand and reuse |
| Minimap and map | Provide minimap textures, masks, and helper widgets | Expand the HUD minimap with a map screen, icons, rotation, and control of unexplored content | Build an in-game map that communicates exploration state |
| Runtime load control | Provide partition-based visibility and load reduction | Expand control with Activator Components, a sparse partition graph, per-frame activation limits, and options for AI, collision, Tick, and light shadows | Choose the load-reduction detail appropriate for larger dungeons and different actor types |
| Parameter editing | Place many settings at the top level of `UDungeonGenerateParameter` | Organize settings into `Theme`, `Structure`, `Path`, `Zones`, and `Gameplay` | Separate essential setup from later fine-tuning more clearly |

## Choose a Progression Policy

In v2.0.0, first choose the dungeon's basic play style with `Path.ProgressionPolicy`. You can then fine-tune main-route emphasis, loops, and extra corridors.

| Policy | Resulting dungeon | Suitable uses |
| --- | --- | --- |
| `Free Exploration` | An open layout with loops, shortcuts, and optional side rooms | Exploration, collection, and roguelites |
| `Start To Goal` | A layout with a readable main route from the start to the goal | Games that need a clear route with some branches |
| `Keys And Locks` | A progression route where players collect keys and cannot bypass the corresponding locked doors | Key searches, escape games, and staged progression |
| `Boss Route` | A route that builds through encounters and rest toward a boss room near the end | Action games and boss-focused progression |
| `Hub Quest` | An early hub that connects to several objective-like branches | Quest structures and hub-based exploration |

![Comparison of the five Progression Policies](images/ProgressionPolicyStyles.png)

In the image, `S` marks the start, `G` marks the goal, and the bright line shows a representative progression route. The positions of the Key/Lock, Boss, and Hub make it easier to compare how different Policies can produce different player experiences from a similar number of rooms. This is a conceptual example; it does not prescribe the exact room shapes or decoration that will be generated.

`Keys And Locks` prevents unsafe loops around locks. Extra corridor complexity still applies to unlocked aisles, while locked aisles disable intersections and merging so players cannot bypass the door. When rebuilding a v1.x design manually, `Keys And Locks` is the closest v2 Policy to the old `UseMissionGraph` behavior. For a regular start-to-goal v1.x design, begin with `Start To Goal` and tune the new settings for the intended result.

## Communicate room purpose with Gameplay Roles

![Room concepts for each Gameplay Role](images/RoomGameplayRoleStyles.png)

Gameplay Roles give generated rooms a purpose: `None`, `Combat`, `Treasure`, `Puzzle`, `Rest`, `Boss`, or `Secret`. The image visualizes one possible use for each Role. Assigning a Role does not automatically place enemies, treasure chests, or puzzles. Connect the Role to Room Sensor Blueprint logic and Role-specific mesh, Interior, and Fixture Overrides. This lets v2.0.0 keep layout intent and gameplay-content tuning separate.

## When v2.0.0 is a good fit

- You want individual dungeons to feel different through exploration, keys and locks, or boss progression
- You want to connect combat, treasure, rest, and secrets to explicit room purposes
- You want meshes, interiors, vegetation, and doors to change by progress, floor, or room role
- You want designers to tune related settings as purpose-based groups
- You need detailed control over Actor, AI, collision, Tick, and lighting cost in a large dungeon
- You want a map screen with icons and exploration state in addition to a HUD minimap

## When keeping v1.x may be practical

Keeping the current production workflow can be reasonable in the following situations:

- Level tuning and game balance already depend on the current v1.x generation results
- The project is close to release and cannot budget time to revalidate layouts, Blueprints, and C++ code
- The project does not need the new Progression Policies, Room Roles, Zone Overrides, or map features
- Existing results, including fixed-seed layouts, must not change

Because there is no supported v1.x-to-v2 migration path, adopting v2 means rebuilding the Dungeon Generator configuration and integrations. Compare the value of the new controls with the time required to recreate, retune, and test the project.

## Rebuild a v1.x design in v2.0.0

There is no supported in-place upgrade or automatic asset conversion. Treat v2.0.0 as a new implementation:

1. **Keep a restorable v1.x project.**
   Preserve a source-control branch or copy that retains the v1.x plugin and can still open the original assets.
2. **Create a separate v2 workspace.**
   Do not overwrite the working v1.x project or assume that opening its Dungeon Generator assets in v2 will convert them.
3. **Recreate the generation settings.**
   Build new v2 assets and configure `Theme`, `Structure`, `Path`, `Zones`, and `Gameplay`. Use the comparison table as a guide, not as a one-to-one parameter map.
4. **Reconnect project integrations manually.**
   Update Blueprint and C++ code for renamed or removed APIs, and recreate Room Sensor, selector, Interior, SubLevel, and minimap connections where required.
5. **Retest the result as a new dungeon design.**
   Validate editor and runtime generation, visuals, progression, gameplay, replication, and performance. Fixed v1.x seeds and layouts are not compatibility targets in v2.

## Notes for Blueprint and C++ users

In v2.0.0, the standard API for generated room information is `FDungeonGeneratedRoomInfo` together with `ADungeonRoomSensorBase::GetGeneratedRoomInfo()`. Rebuild integrations against this API rather than relying on v1 Room Sensor fields.

`EnableLightShadowControl`, `IsEnableLightCastShadowControl()`, and `SetEnableLightCastShadowControl()` have been removed in v2.0.0. If a v1 Blueprint or C++ class references them, remove those references manually before compiling with v2.0.0. Point Light and Spot Light shadows are now managed automatically by the runtime light-control system.

`GetInquireInteriorTags()` and `InquireInteriorTags` have been replaced by `GetProvidedContextTags()` and `ProvidedContextTags`. This rename is not redirected automatically: recreate Room Sensor Blueprint implementations under the new event and re-enter saved Interior Location Component values.

`EDungeonInteriorPlacementAnchor`, `PlacementAnchors`, and the placement-query `Anchor` member have been removed. v2 Interior Actor placement no longer creates an automatic room-center candidate. Explicit nested spawn positions authored with `UDungeonInteriorLocationComponent` continue to work.

Nested Interior Location spawning now requires the target Part to declare at least one `RequiredContextTags` entry. When recreating an untagged nested placement, add matching `ProvidedContextTags` to the Interior Location. Recursive branches also skip Actor Classes already present in their ancestry.

Mesh Set and individual-part selection are organized around Mesh Set Selectors and Parts Selectors. If the project has custom Blueprint or C++ selection logic, verify not only that it compiles, but also that it selects the intended candidates under representative conditions.

## v2 rebuild checklist

- A restorable v1.x project with the v1.x plugin is preserved
- v2 work is taking place in a separate project or source-control branch
- New v2 `UDungeonGenerateParameter`, Mesh Set, Interior, SubLevel, and Room Sensor assets have been created as required
- Both editor generation and runtime generation have been tested
- Blueprint／C++ integration for Room Sensors, selectors, and the minimap has been tested
- Visual appearance and gameplay progression have been validated as a new v2 design

## Related pages

- [UDungeonGenerateParameter Guide](./UDungeonGenerateParameter.en.md)
- [ADungeonRoomSensorBase Guide](./ADungeonRoomSensorBase.en.md)
- [Custom Selector Guide](./CustomSelector.en.md)
- [Generate Minimap Textures](./GenerateMinimapTexture.en.md)
- [Reduce Runtime Processing Cost](./LoadReduction.en.md)
