# MissionGraph を適用する

`Path.ProgressionPolicy = KeysAndLocks` は、スタートからゴールまでの経路へ鍵とロックの情報を配置します。生成された経路は検証され、鍵付き通路を生成形状から迂回できないようにします。

MissionGraph は現在 Beta 機能です。先に通常のダンジョンが安定して生成できる状態にしてください。

## 重要な制限: 配置はベストエフォート

鍵とロックは、生成レイアウトに適切な部屋とロック可能な経路がある場合だけ作られます。Unique Key を置ける有効な部屋がない場合、生成は失敗せず、鍵もロックもない到達可能なダンジョンとして完了します。このとき `ADungeonGenerateBase::GetLastGenerationIssues()` から `DG_GEN_KEYS_NOT_PLACED` 警告を取得できます。

これは経路破損ではなく、ロックなしのフォールバックです。毎回ロックが必要なゲームでは生成 Issue を確認し、別 Seed で再生成するか、条件を満たす Hall/Hanare 部屋を増やしてください。`RandomSeed` を固定した場合は、同じレイアウトを意図的に再現します。

## 最短の設定手順

### 1. 進行 Policy を有効にする

`UDungeonGenerateParameter` で `Path.ProgressionPolicy = KeysAndLocks` にします。

`Path.ExtraCorridorComplexity = 0` は仕組みを確認しやすい基準値ですが、必須ではありません。0 より大きい値は、ロックされていない通路へ交差や複雑さを追加します。鍵付き通路では交差と結合が常に無効になり、専用経路を保って迂回を防ぎます。現在の Details Panel では Keys And Locks へ切り替えた後にこの項目が読み取り専用になるため、0 以外を使う場合は Policy の切り替え前に設定します。Blueprint / C++ から設定することもできます。`Path.LoopRouteDensity` は、ロックを迂回しない範囲で有効です。

Keys And Locks では `Path.StartRoomPolicy` に `UseCentralPoint` と `UseMultiStart` を使用できません。

### 2. Door Actor を用意する

`ADungeonDoorBase` 派生 Blueprint を作り、`Theme.Fixtures.Door Parts` に追加します。最後のロックだけ見た目を変える場合は、`Unique Door Parts` も設定します。

![](./images/MissionGraph1.png)

Door Base は `EDungeonRoomProps` を受け取り、`IsKeyLockedDoor` や `IsUniqueKeyLockedDoor` などの判定を公開します。インベントリ確認、開閉アニメーション、鍵の消費は実装しません。`OnInitialize` で受け取った Props を使い、Door Blueprint 側にゲームプレイ処理を実装してください。

### 3. Key Actor を用意する

`Gameplay.DungeonRoomSensorClass` に指定した `ADungeonRoomSensorBase` 派生 Blueprint で、簡単な確認用に `SpawnKeyActor` と `SpawnUniqueKeyActor` を設定します。

![DungeonRoomSensor](./images/DungeonRoomSensor1.png)

![](./images/MissionGraph2.png)

これらの Helper は、MissionGraph で印が付いた部屋に設定 Actor をスポーンします。鍵 Actor の取得、インベントリ登録、消費はプラグイン側では実装しません。ゲーム側 Blueprint に処理を実装してください。宝箱や敵撃破から鍵を渡す場合は、`OnInitialize` に渡される部屋情報を使い、独自処理でスポーンまたは付与します。

## 配置に成功した場合に保証されること

- 各 Common Key は、対応する進行ロックより前に到達できます。
- Unique Key は Common Lock の進行後に到達可能になります。
- Unique Lock はゴール直前の最後のゲートになります。
- 鍵付き通路は、迂回を作る交差や Gate 共有を行いません。
- 配置された鍵とロックのグラフが解けない場合は、MissionGraph 検証によって生成が失敗します。

配置できる Common Lock は最大 16 個です。鍵を置く部屋は、別の予約 Item を持たない Hall または Hanare である必要があります。

## Play モードで確認する

1. 生成後に `GetLastGenerationIssues()` を確認します。`DG_GEN_KEYS_NOT_PLACED` があれば、その生成結果には意図的に鍵とロックがありません。
2. 配置成功時は、Common Key と Unique Key の Actor が対象部屋にスポーンしたか確認します。
3. Door Blueprint が通常扉、Common Lock、Unique Lock を区別できるか確認します。
4. 鍵の取得、開閉拒否、Common Key の消費、Unique Key の規則を実装したゲーム処理をテストします。
5. 必要なロックを閉じたままゴールへ到達できないことを確認します。

## トラブルシューティング

- 鍵も鍵付き扉も出ない: まず Generation Issue を確認してください。有効なレイアウトでも、説明どおりロックなしへフォールバックする場合があります。
- 扉は出るが鍵が出ない: `Gameplay.DungeonRoomSensorClass`、`SpawnKeyActor`、`SpawnUniqueKeyActor` を確認します。
- 鍵は出るが取得できない: 取得とインベントリ処理は、Helper の Actor スポーンには含まれないプロジェクト側のゲームプレイです。
- 扉の見た目はあるがプレイヤーを止めない: `ADungeonDoorBase` 派生 Blueprint に Collision、開閉、ロック判定を実装します。
- 通路複雑度を上げても鍵付き通路が変わらない: 意図した動作です。その場所では未施錠通路だけに値が適用されます。

## 関連ページ

- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
- [FDungeonDoorActorParts.ja.md](./FDungeonDoorActorParts.ja.md)
