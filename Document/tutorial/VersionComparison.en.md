# Dungeon Generator v1.x and v2.0.0 Comparison

Dungeon Generator v2.0.0 is a major update designed to create more than randomly connected rooms and corridors. It focuses on **playable progression, visual variety across the dungeon, and settings that are easier to tune**.

> **Release status:** v2.0.0 is currently in development and has not been released. This page reflects the current implementation, and details may change before the final release.

You can continue using meshes, interiors, sublevels, and other content created for v1.x while gaining clearer control over progression, room purposes, and theme changes. However, the generation algorithm and parameter structure have changed, so the same seed and settings may not reproduce the exact v1.x layout.

## Main differences

| Area | v1.x | v2.0.0 | What the upgrade provides |
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

`Keys And Locks` restricts unsafe loops and extra corridors so players cannot bypass locked doors. A v1.x asset with `UseMissionGraph` enabled migrates to this Policy. A regular v1.x asset that did not use MissionGraph migrates to `Start To Goal`.

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

Migration carries many existing values forward, but generated results still need to be reviewed. Compare the value of the new controls with the time required for retuning and testing.

## Migrate v1.x assets to v2.0.0

1. **Back up the project.**
   Create a source-control branch or retain a copy of the project and assets that can still be opened with v1.x.
2. **Install v2.0.0 and open the project in Unreal Editor.**
   When a Dungeon Generator asset is loaded, legacy values are converted in memory into the v2.0.0 setting groups.
3. **Review the converted settings.**
   Inspect `Theme`, `Structure`, `Path`, and `Gameplay` in `UDungeonGenerateParameter`. Generate the dungeon again, including with representative fixed seeds, and compare the result.
4. **Check the `DungeonGenerator Migration` log.**
   Review information, warnings, errors, and suggested fixes in Unreal Editor's Message Log. A v1.x asset without a Custom Version is treated as a v1.x asset and produces a warning that requests review before saving.
5. **Check Blueprint and C++ references.**
   Pay particular attention to legacy room-information fields on Room Sensors, Mesh／Parts selection logic, and classes whose names or locations changed.
6. **Save the assets after they pass review.**
   Saving writes the v2.0.0 format. There is no documented downgrade workflow back to v1.x, so do not overwrite the backup until validation is complete.

### Main settings migrated automatically

- Grid sizes, room count, room dimensions, room margins, and floor mode
- Layout candidate count, corridor complexity, and corridor ceiling height
- Start／goal selection, PlayerStart movement, and MissionGraph usage
- Room and corridor Mesh Set Databases
- Pillars, torches, doors, and their legacy selection methods
- Interior Database, SubLevel Database, and Room Sensor references

Automatic migration maps existing values to their new storage locations. It does not automatically redesign the game around the new Roles, Zones, Fixture Overrides, or detailed Progression Policy settings.

## Notes for Blueprint and C++ users

In v2.0.0, the standard API for generated room information is `FDungeonGeneratedRoomInfo` together with `ADungeonRoomSensorBase::GetGeneratedRoomInfo()`. Legacy fields on the Room Sensor remain for migration, but new implementations should use the standard API.

Mesh Set and individual-part selection are organized around Mesh Set Selectors and Parts Selectors. If the project has custom Blueprint or C++ selection logic, verify not only that it compiles, but also that it selects the intended candidates under representative conditions.

## Pre-upgrade checklist

- A restorable backup of the v1.x project and assets exists
- Representative fixed seeds and expected generation results have been recorded
- The project has identified its `UDungeonGenerateParameter`, Mesh Set, Interior, SubLevel, and Room Sensor assets
- Migration warnings and errors have been reviewed
- Both editor generation and runtime generation have been tested
- Blueprint／C++ integration for Room Sensors, selectors, and the minimap has been tested
- Visual appearance and gameplay progression have been checked before saving migrated assets

## Related pages

- [UDungeonGenerateParameter Guide](./UDungeonGenerateParameter.en.md)
- [ADungeonRoomSensorBase Guide](./ADungeonRoomSensorBase.en.md)
- [Custom Selector Guide](./CustomSelector.en.md)
- [Generate Minimap Textures](./GenerateMinimapTexture.en.md)
- [Reduce Runtime Processing Cost](./LoadReduction.en.md)
