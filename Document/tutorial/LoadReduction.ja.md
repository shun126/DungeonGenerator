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
  パーティションによる負荷制御を有効にします。広いランタイムダンジョンでは基本的に有効にします。
- `ActivationRangeScale`
  プレイヤー周辺を有効にする範囲の倍率です。Actor の表示や復帰が遅い場合、または近くで消える場合は値を上げます。
- `PartitionGridCountOverride`
  自動で決まるパーティション分割数を上書きします。細かく調整したい場合以外は各軸 `0` のままで構いません。
- `bUsePrecomputedPartitionVisibility`
  生成済みダンジョンの見通し情報を使い、見えない区画を有効にしすぎないようにします。通常は有効のまま使います。
- `PrecomputedVisibilityDilationHopCount`
  見える可能性があるパーティションを隣接区画へ広げます。曲がり角や扉付近でポップインが気になる場合は値を上げます。
- `MaxPartitionActivationsPerFrame`
  1 フレームで有効化するパーティション数の上限です。小さくすると一瞬の重さを抑えやすくなりますが、復帰が複数フレームに分散します。
- `MaxPartitionInactivationsPerFrame`
  1 フレームで無効化するパーティション数の上限です。大きなエリアから離れる時の負荷を分散できます。
- `MaxShadowCastingPointAndSpotLights`
  影を落とす Point Light / Spot Light の最大数です。ローカルライトが多いダンジョンで使います。
- `ShowDebugInformation`
  エディタ上でパーティションのデバッグ情報を表示します。

![パーティション設定](images/LoadReduction2.jpg)

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
- `EnableLightShadowControl`
  Point Light / Spot Light の Cast Shadow を制御します。BeginPlay 時点で Cast Shadow が有効なライトだけが対象です。
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
  Visibility と light shadow control を有効にします。`MaxShadowCastingPointAndSpotLights` で影付きライトが増えすぎないようにします。
- インタラクト Actor
  Tick と visibility control は慎重に使います。line trace やインタラクト判定が遠くから必要な場合は collision を残します。

## 生成時の重さも抑える
負荷制御は、ダンジョンが生成された後の処理を軽くする機能です。
生成中の一瞬の重さには、植生やメッシュの設定も合わせて確認してください。

- `Theme.bDeferredVegetationSpawn`
  植生を 1 フレームでまとめて置かず、複数フレームに分けて生成します。
- `Theme|VegetationPerformance`
  1 フレームあたりの植生候補数、foliage tree build 数、処理時間の目安を調整します。
- Instanced Mesh Cull Distance
  遠くに表示する必要がない静的な見た目には、カリング距離の設定を使います。

## 困った時
- 近くの Actor が消える
  `ActivationRangeScale` または `PrecomputedVisibilityDilationHopCount` を上げ、想定したパーティションに登録されているか確認します。
- ライトは見えるが影が消える
  `MaxShadowCastingPointAndSpotLights` を上げるか、影を落とす Point Light / Spot Light の数を減らします。
- 敵 AI が止まってしまう
  その敵 Blueprint の `EnableOwnerActorAiControl` を無効にします。
- 部屋移動時に一瞬重い
  `MaxPartitionActivationsPerFrame` と `MaxPartitionInactivationsPerFrame` を下げます。`OnPartitionActivate` 内で重い処理をしていないかも確認します。
- 遠くの Static Mesh がまだ重い
  Instanced Mesh の culling と `Theme.bDeferredVegetationSpawn` も併用します。

## 便利な Blueprint フック
`ADungeonMainLevelScriptActor` には、生成前後に呼ばれる Blueprint イベントがあります。

- `OnPreDungeonGeneration`
  ダンジョン生成前に呼ばれます。ロード画面や一時 UI の表示に使えます。
- `OnPostDungeonGeneration`
  ダンジョン生成後に呼ばれます。ゲーム開始、ロード UI の非表示、軽い初期化処理に使えます。

独自の流れで生成後に通行可能なレイアウトを変えた場合は、`RebuildSparsePartitionGraphAndRefresh()` を呼び、パーティショングラフと activator 登録を更新してください。
