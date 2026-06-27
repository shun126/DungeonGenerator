# アクターの負荷を分散



# PVS による負荷制御

PVS (Potentially Visible Set) は、生成済み dungeon の traversable grid から、各 Partition が見える可能性のある Partition 集合を事前計算する仕組みです。
Runtime では Player/Pawn の現在 Partition を起点に PVS を参照し、必要な Partition だけ `Mark()` して `UDungeonComponentActivatorComponent` を復帰します。

PVS は厳密な遮蔽判定ではなく、見えているものを消さないために保守的に広める設計です。
壁、床、天井で明確に遮られている場合は除外しますが、見える可能性が残る場合は対象 Partition を有効化側に含めます。

スロープ、吹き抜け、上下階をつなぐ空間では、床中心だけの直線判定だと上下方向の見通しを取りこぼしやすくなります。
そのため、`Slope` / `Stairwell` / `DownSpace` / `UpSpace` では入口側、出口側、上下空間を表す追加サンプルを使い、PVS を安全側に広げます。

PVS 構築に失敗した場合は、見え消えを避けるため全 Partition をアクティブに倒します。
負荷削減よりも、プレイヤーから見えているメッシュやライトを突然消さないことを優先します。

# クラス図
```mermaid
classDiagram

class DungeonComponentActivationSaver {
	+ Stash(const AActor* actor, std::function<std::pair<bool, T>(UActorComponent*)> function) void
	+ Pop(std::function<void(UActorComponent*, const T)> function) void
}

class UDungeonComponentActivatorComponent {
    -OnPartiationActivate() void
    -OnPartiationInactivate() void
}

class UDungeonPartiation {
    -mComponetActivator : UDungeonComponentActivatorComponent
    -mBounds : FBox
    -mMarked : bool

    -Begin() void
    -Mark(FBox) void
    -End() void
}

class ADungeonMainLevelScriptActor {
    -Initialize(FBox) void
    -Add(UActorComponent) bool

    -Begin() void
    -Mark(FBox) void
    -End() void
}

UActorComponent --o DungeonComponentActivationSaver

UActorComponent <|-- UDungeonComponentActivatorComponent
ADungeonMainLevelScriptActor --o UDungeonComponentActivatorComponent
UDungeonPartiation --o UDungeonComponentActivatorComponent
DungeonComponentActivationSaver --* UDungeonComponentActivatorComponent

UObject <|-- UDungeonPartiation
UDungeonComponentActivatorComponent --* UDungeonPartiation

ALevelScriptActor <|-- ADungeonMainLevelScriptActor
UDungeonPartiation --* ADungeonMainLevelScriptActor
```

# パーティション構築シーケンス

`PreInitializeComponents()` または `RebuildSparsePartitionGraphAndRefresh()` から `ExecutePartitionBuild()` を呼び、現在の生成済み dungeon から runtime 用の Partition graph と PVS を作ります。

```mermaid
sequenceDiagram
    participant LevelScript as ADungeonMainLevelScriptActor
    participant Partition as UDungeonPartition
    participant PVS

    LevelScript ->> LevelScript : ExecutePartitionBuild()
    LevelScript ->> LevelScript : CollectPartitionBuildActors()
    LevelScript ->> LevelScript : ComputePartitionBuildMetrics()
    LevelScript ->> Partition : BuildSparsePartitionGraph()
    Partition ->> Partition : SetBounds()
    Partition ->> Partition : AddNeighborIndex()
    LevelScript ->> PVS : BuildPartitionVisibilitySamples()
    LevelScript ->> PVS : BuildPartitionConnectedComponents()
    LevelScript ->> PVS : BuildPrecomputedPartitionVisibility()
    LevelScript ->> LevelScript : ResetPartitionTransitionQueue()
```

# Runtime 更新シーケンス

Play 中は `Tick()` で Player/Pawn の現在位置を調べ、PVS から必要な Partition を `Mark()` します。
`End(deltaSeconds)` で marked/unmarked を目的の activation state に変換し、非アクティブ化猶予 timer と遷移キューを処理します。

```mermaid
sequenceDiagram
    participant LevelScript as ADungeonMainLevelScriptActor
    participant Player as PlayerController/Pawn
    participant PVS
    participant Partition as UDungeonPartition
    participant Activator as UDungeonComponentActivatorComponent

    LevelScript ->> LevelScript : Tick(deltaSeconds)
    LevelScript ->> Partition : Begin()
    Partition ->> Partition : Unmark()
    LevelScript ->> Player : GetPawn()->GetActorLocation()
    LevelScript ->> LevelScript : FindPartitionIndex(playerLocation)
    LevelScript ->> PVS : MarkPrecomputedPartitionVisibility(sourcePartitionIndex)
    PVS ->> Partition : Mark()
    LevelScript ->> Partition : End(deltaSeconds)
    Partition ->> Partition : ResetPartitionInactivateRemainTimer() / UpdatePartitionInactivateRemainTimer()
    LevelScript ->> LevelScript : EnqueuePartitionTransition()
    LevelScript ->> LevelScript : ProcessPartitionTransitionQueue()
    LevelScript ->> Partition : CallPartitionActivate() / CallPartitionInactivate()
    Partition ->> Activator : CallPartitionActivate() / CallPartitionInactivate()
    LevelScript ->> LevelScript : UpdateShadowCastingPointAndSpotLights()
```

# Runtime 再構築シーケンス

生成後に traversable layout が変わった場合は `RebuildSparsePartitionGraphAndRefresh()` を呼び、Partition graph と PVS を作り直します。
再構築後は全 `UDungeonComponentActivatorComponent` を現在位置の Partition に登録し直し、現在の Player/Pawn 位置から activation state を即時同期します。

```mermaid
sequenceDiagram
    participant LevelScript as ADungeonMainLevelScriptActor
    participant Actor_ as AActor
    participant Activator as UDungeonComponentActivatorComponent
    participant Partition as UDungeonPartition
    participant PVS

    LevelScript ->> LevelScript : RebuildSparsePartitionGraphAndRefresh()
    LevelScript ->> LevelScript : ExecutePartitionBuild(RuntimeRebuild)
    LevelScript ->> Partition : BuildSparsePartitionGraph()
    LevelScript ->> PVS : BuildPartitionVisibilitySamples()
    LevelScript ->> PVS : BuildPrecomputedPartitionVisibility()
    LevelScript ->> Actor_ : TActorIterator<AActor>
    Actor_ ->> Activator : GetComponents()
    LevelScript ->> Activator : RefreshPartitionRegistration(this)
    Activator ->> Partition : UnregisterActivatorComponent()
    Activator ->> Partition : RegisterActivatorComponent()
    LevelScript ->> LevelScript : ApplyCurrentPartitionActivationState()
```
