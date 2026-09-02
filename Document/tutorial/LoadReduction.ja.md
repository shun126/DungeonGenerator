# ランタイム処理負荷を軽減する

広いランタイム生成ダンジョンでは、敵、ライト、装飾、コリジョン付き Actor が大量に配置されます。
プレイヤーから遠い部屋まで常に動かし続けると、見えていない場所の処理でゲームが重くなることがあります。

Dungeon Generator には、そのための負荷制御機能があります。
`ADungeonMainLevelScriptActor` が生成済みダンジョンをパーティションに分け、`UDungeonComponentActivatorComponent` がプレイヤーから遠い区画の Actor を一時停止または復帰します。

![処理負荷軽減の概要](images/LoadReduction1.jpg)

## 何を軽くできるか
プレイヤーから遠い時に動いていなくてもよい Actor に使います。

- 敵 Actor や NPC
- Tick を持つ装飾 Actor
- 動く小物やエフェクト
- Point Light や Spot Light
- 近くにいない時は反応しなくてよいインタラクト Actor
- コリジョンが多い Actor

Player、ゲーム管理 Actor、全体 BGM 管理、セーブ管理、UI 関連など、常に動く必要がある Actor には使わないでください。

## メインレベルを設定する
ランタイムダンジョンを持つレベルの Level Script Actor として `ADungeonMainLevelScriptActor` を使います。

この Actor はレベル内のパーティションを管理します。
Play 中はプレイヤー位置を見て、どのパーティションを有効にするかを決め、登録された `UDungeonComponentActivatorComponent` を更新します。

重要な設定は次のとおりです。

- `bEnableLoadControl`
  PVS を使ったパーティション負荷制御を有効にします。広いランタイムダンジョンでは基本的に有効にします。
- `ActivationRangeScale`
  プレイヤー周辺を有効にする範囲の倍率です。Actor の表示や復帰が遅い場合、または近くで消える場合は値を上げます。
- `PartitionGridCountOverride`
  自動で決まるパーティション分割数を上書きします。細かく調整したい場合以外は各軸 `0` のままで構いません。
- `PrecomputedVisibilityDilationHopCount`
  PVS で見える可能性があるパーティションを隣接区画へ広げます。曲がり角、扉付近、上下移動でポップインが気になる場合は値を上げます。
- `MaxPartitionActivationsPerFrame`
  1 フレームで有効化するパーティション数の上限です。小さくすると一瞬の重さを抑えやすくなりますが、復帰が複数フレームに分散します。
- `MaxPartitionInactivationsPerFrame`
  1 フレームで無効化するパーティション数の上限です。大きなエリアから離れる時の負荷を分散できます。
- `MaxShadowCastingPointAndSpotLights`
  表示する影付き Point Light / Spot Light の最大数です。上限を超えたライトは影なしで表示し続けずフェードアウトするため、壁を越える光漏れを防げます。`0` なら無制限です。
- `ShowDebugInformation`
  エディタ上でパーティションのデバッグ情報を表示します。

![パーティション設定](images/LoadReduction2.jpg)

### 自動生成されるISM/HISM地形

ISMまたはHISMを使用する床、壁、屋根、柱は、ダンジョングリッド基準の固定された`8 × 8 × 2` XYZ空間グループに分けて生成されます。ダンジョンのパーティション構築後、各グループはメッシュ境界が交差するすべてのパーティションに関連付けられます。

関連するパーティションが1つでも有効な間はグループが表示され、すべて無効になった時だけ非表示になります。距離カリングも引き続き適用されるため、2つの仕組みが組み合わさって描画負荷を抑えます。パーティションによる地形制御は表示だけを変更し、CollisionやNavigationは無効にしません。

## Activator Component を追加する
プレイヤーから遠い時に休ませたい Actor Blueprint に `UDungeonComponentActivatorComponent` を追加します。

Actor ごとに、どの処理を制御するか選びます。

- `EnableOwnerActorTickControl`
  Actor の Tick 状態を保存し、パーティションが無効な間は Tick を止めます。
- `EnableOwnerActorAiControl`
  パーティションが無効な間は AI ロジックを止めます。遠くでも考え続ける必要がある Actor では無効にします。
- `EnableComponentActivationControl`
  Component の Activation 状態を保存し、あとで復元します。
- `EnableComponentVisibilityControl`
  無効な間は Component を非表示にします。
- `EnableCollisionEnableControl`
  無効な間はコリジョンを止めます。遠くからでも trace やブロックが必要な Actor では無効にします。

`OnPartitionActivate` と `OnPartitionInactivate` は任意の Blueprint イベントです。
パーティクルの再開、UI マーカーの更新、特殊なアニメーションの一時停止など、独自処理が必要な場合だけ使います。
多くの Actor が同時に復帰することがあるため、このイベント内で重い初期化を行わないでください。

![Activator Component](images/LoadReduction3.jpg)

## 実用的な設定例
- 敵
  Tick と AI control を有効にします。無効中にプレイヤーをブロックしなくてよいなら collision control も有効にします。遠距離行動が必要な敵では AI control を無効にします。
- 装飾 Actor
  Component activation と visibility control を有効にします。遠くで触れない小物なら collision control も有効です。
- ライト
  実行時ライト制御の対象にする非Staticの Point Light / Spot Light に Activator を追加します。`MaxShadowCastingPointAndSpotLights` で、同時に表示する影付きライト数を制限します。
  自動生成される通路スロープ用ベースライトは専用 Actor を持ち、Visibility control のみを使います。影なしライトなので影予算の対象にはなりません。各 Actor は対応するスロープ位置の Partition に登録されます。
- インタラクト Actor
  Tick と visibility control は慎重に使います。line trace やインタラクト判定が遠くから必要な場合は collision を残します。

## 生成時の重さも抑える
負荷制御は、ダンジョンが生成された後の処理を軽くする機能です。
生成中の一瞬の重さには、植生やメッシュの設定も合わせて確認してください。

- `GenerationPerformance.ActorSpawn`
  Room Sensorの敵やキーを含むActor生成を複数フレームへ分散します。`MaxSpawnRequestsPerFrame`は1フレームの要求数、`MaxSpawnTimeMs`は処理時間の上限です。どちらも`0`で無制限になります。
- `GenerationPerformance.Vegetation`
  生成開始時にすべての候補を作らず、コンパクトな配置Jobとして保持します。各フレームでPlayer Cameraに近いJobから候補作成、Trace、配置を行います。植生候補数、配置時間、Foliage Tree構築数と構築時間を個別に調整でき、各上限の`0`は無制限です。
- `bUseDeferredSpawn`
  Actor SpawnとVegetationに個別の切替があります。無効にすると対応する処理を同期完了します。Editorの静的Generateは設定に関係なく常に同期完了しますが、PIEでは設定が適用されます。
- Instanced Mesh Cull Distance
  遠くに表示する必要がない静的な見た目には、カリング距離の設定を使います。

## 困った時
- 近くの Actor が消える
  `ActivationRangeScale` または `PrecomputedVisibilityDilationHopCount` を上げ、想定したパーティションに登録されているか確認します。
- 部屋の外から見えるライトが消える
  通常の閉じたドアでは、ドアが開く段階で部屋のライトが有効になるため、問題になることはほとんどありません。
  ただし、開いたドア、格子、窓などを通して部屋の外から内部が見えるデザインでは、部屋と通路の Identifier が異なるため、ライトの正面角度カリングが適用されることがあります。
  ライトの配置方向によって消える場合は、`PointAndSpotLightTurnOnAngle` と `PointAndSpotLightTurnOffAngle` を大きくしてください。原因を切り分ける時は、両方を一時的に `180` 度へ設定します。
  それでも消える場合は、`ActivationRangeScale` と `PrecomputedVisibilityDilationHopCount` を確認してください。
- 他のライトに近づくと Point Light / Spot Light がフェードアウトする
  `MaxShadowCastingPointAndSpotLights` の上限外になっている可能性があります。上限を上げるか、`0` で無制限にするか、影を落とす Point Light / Spot Light の数を減らします。
- 通路スロープの光が隣の通路や階へ漏れる
  `Theme.AisleSlopeBaseLight.AttenuationRadius` を下げます。Zoneごとに異なる値が必要なら、Zoneの `bOverrideAisleSlopeBaseLight` を有効にしてください。
- 敵 AI が止まってしまう
  その敵 Blueprint の `EnableOwnerActorAiControl` を無効にします。
- 部屋移動時に一瞬重い
  `MaxPartitionActivationsPerFrame` と `MaxPartitionInactivationsPerFrame` を下げます。`OnPartitionActivate` 内で重い処理をしていないかも確認します。
- 遠くの Static Mesh がまだ重い
  Instanced Meshのcullingと`GenerationPerformance.Vegetation.bUseDeferredSpawn`も併用します。

## 便利な Blueprint フック
`ADungeonMainLevelScriptActor` には、生成前後に呼ばれる Blueprint イベントがあります。

- `OnPreDungeonGeneration`
  ダンジョン生成前に呼ばれます。ロード画面や一時 UI の表示に使えます。
- `OnPostDungeonGeneration`
  ダンジョン生成後に呼ばれます。ゲーム開始、ロード UI の非表示、軽い初期化処理に使えます。

ゲームプレイに必要な遅延Actorがすべて処理された時点で開始する場合は、Dungeon Generatorの`OnGenerationSuccess`イベントを使用してください。遠方の植生やFoliage Treeを含むすべての視覚要素が完成するまでロード画面を残す場合は、`OnGenerationComplete`を使用します。ポーリングする場合は`IsGenerationComplete()`で同じ最終状態を取得できます。

Room Sensorに設定した敵、キー、Unique Keyは自動的に遅延生成されます。Blueprintで独自のActorも同じ生成キューへ含める場合は`RequestDeferredSpawnActorFromClass`を呼び、完了コールバックから生成済みActorを受け取ります。既存の`SpawnActorFromClass`は、互換性のため即時生成関数として維持されています。

独自の流れで生成後に通行可能なレイアウトを変えた場合は、`RebuildSparsePartitionGraphAndRefresh()` を呼び、パーティショングラフと activator 登録を更新してください。
