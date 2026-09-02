# Apply MissionGraph

`Path.ProgressionPolicy = KeysAndLocks` asks the generator to place key and lock metadata on a route from the start to the goal. The generated route is validated so a locked corridor cannot be bypassed through generated geometry.

MissionGraph is currently a beta feature. First make sure a normal dungeon generates reliably.

## Important limitation: placement is best effort

Keys and locks are created only when the generated layout contains suitable rooms and lockable route edges. If no valid room is available for the unique key, generation succeeds as a complete, reachable dungeon without any keys or locks. The generator reports the `DG_GEN_KEYS_NOT_PLACED` warning through `ADungeonGenerateBase::GetLastGenerationIssues()`.

This is not a broken route. It is a lock-free fallback. If every generated dungeon must contain locks, inspect generation issues and retry with another seed, or provide more eligible Hall/Hanare rooms. A fixed `RandomSeed` intentionally reproduces the same layout.

## Fastest setup

### 1. Enable the progression policy

Set `Path.ProgressionPolicy = KeysAndLocks` in `UDungeonGenerateParameter`.

`Path.ExtraCorridorComplexity = 0` is the simplest baseline, but it is not required. Values above zero add intersections and complexity to unlocked aisles. Locked aisles always keep intersections and merging disabled so they remain private and cannot be bypassed. In the current Details panel, set a nonzero value before switching the Policy to Keys And Locks, because the field becomes read-only afterward; Blueprint/C++ can also set it. `Path.LoopRouteDensity` remains effective only where a loop does not bypass a lock.

`Path.StartRoomPolicy` cannot use `UseCentralPoint` or `UseMultiStart` with Keys And Locks.

### 2. Prepare door actors

Create a Blueprint derived from `ADungeonDoorBase` and add it to `Theme.Fixtures.Door Parts`. Add a separate `Unique Door Parts` entry if the final lock needs a different appearance.

![](./images/MissionGraph1.png)

The base door receives `EDungeonRoomProps` and exposes helpers such as `IsKeyLockedDoor` and `IsUniqueKeyLockedDoor`. It does not implement inventory checks, opening animation, or key consumption. Implement that gameplay in the door Blueprint from the properties received by `OnInitialize`.

### 3. Prepare key actors

In the `ADungeonRoomSensorBase` Blueprint assigned to `Gameplay.DungeonRoomSensorClass`, set `SpawnKeyActor` and `SpawnUniqueKeyActor` for a quick test.

![DungeonRoomSensor](./images/DungeonRoomSensor1.png)

![](./images/MissionGraph2.png)

These helper properties spawn the configured actors in rooms marked by the mission graph. The plugin does not implement pickup, inventory, or consumption for your key actor. Implement those rules in your gameplay Blueprints. If keys should come from a chest or defeated enemy, use the room information passed to `OnInitialize` and spawn them through your own logic instead.

## What is guaranteed when placement succeeds

- Every common key is reachable before its corresponding progression lock.
- The unique key becomes reachable after the common-lock progression.
- The unique lock is the final gate before the goal.
- Locked corridors do not accept intersections or shared gates that would create a bypass.
- Mission graph validation aborts generation if the placed key-and-lock graph is unsolvable.

Placement supports at most 16 common locks. Key rooms must be eligible Hall or Hanare rooms without another reserved item.

## Verify in Play mode

1. Check `GetLastGenerationIssues()` after generation. If `DG_GEN_KEYS_NOT_PLACED` is present, this run intentionally has no key-and-lock content.
2. When placement succeeded, confirm that common and unique key actors spawned in the marked rooms.
3. Confirm the door Blueprint distinguishes ordinary, common-lock, and unique-lock doors.
4. Test the gameplay code that grants keys, blocks opening, consumes common keys, and preserves the unique-key rule.
5. Confirm the goal cannot be reached while the required lock is closed.

## Troubleshooting

- No keys or locked doors appear: inspect generation issues first. A valid layout may still use the documented lock-free fallback.
- Doors appear but keys do not: check `Gameplay.DungeonRoomSensorClass`, `SpawnKeyActor`, and `SpawnUniqueKeyActor`.
- Keys spawn but cannot be collected: pickup and inventory are project gameplay, not behavior supplied by the helper actor spawn.
- A door looks correct but does not block the player: implement collision, opening, and lock checks in the `ADungeonDoorBase` Blueprint.
- Higher corridor complexity does not affect a locked corridor: this is intentional; the value applies only to unlocked aisles there.

## Related Pages

- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
- [FDungeonDoorActorParts.en.md](./FDungeonDoorActorParts.en.md)
