# UDungeonGenerateParameter ガイド

`UDungeonGenerateParameter` は、**ダンジョン生成の中心になる設定アセット**です。  
レイアウト、開始位置、見た目、サブレベル、センサー、内装まで、多くの設定をここから参照します。

## まず最初に設定する項目
- `DungeonRoomMeshPartsDatabase`  
  部屋用の `Mesh set database` です。未設定だと生成できません。
- `DungeonAisleMeshPartsDatabase`  
  通路用の `Mesh set database` です。未設定だと生成できません。
- `GridSize` / `VerticalGridSize`  
  メッシュやサブレベルが合わせる基準サイズです。
- `RandomSeed`  
  `0` なら毎回ランダム、固定値なら再現用です。

最初の確認では、まず上の 4 つだけ押さえておけば十分です。

## レイアウトを決める項目
- `RoomWidth` / `RoomDepth` / `RoomHeight`  
  部屋サイズの最小 / 最大です。大きくすると広場寄り、小さくすると迷路寄りになります。
- `NumberOfCandidateRooms`  
  生成の試行に使う候補部屋数です。少なすぎると生成失敗しやすくなります。
- `RoomMargin` / `VerticalRoomMargin`  
  部屋同士の余白です。密度や上下の詰まり方を調整します。
- `MergeRooms`  
  隣接部屋を結合して大部屋を作りやすくします。有効にすると一部の余白設定は無効になります。
- `ExpansionPolicy`  
  横に広げるか、縦に広げるか、両方を許可するかを決めます。  
  単層にしたい場合は `Flat` ではなく `ExpansionPolicy = Flat` を使うのが新しい書き方です。
- `NumberOfCandidateFloors`  
  多層構造を試行するための候補数です。`ExpansionPolicy = Flat` のときは使いません。

## 開始位置と進行の項目
- `StartLocationPolicy`  
  スタート部屋の選び方です。通常は `UseSouthernMost` か `UseCentralPoint` から始めると分かりやすいです。
- `MovePlayerStartToStartingPoint`  
  旧設定です。新規設定では `StartLocationPolicy` を優先してください。
- `UseMissionGraph`  
  鍵付きルートやミッション進行を使いたい場合に有効化します。  
  実運用では `MergeRooms = false` かつ `AisleComplexity = 0` と合わせて使うのが安全です。
- `AisleComplexity`  
  通路の複雑さです。MissionGraph を使わない通常のダンジョンでは、`1` 以上で調整してください。
- `AisleCeilingHeightPolicy`  
  通路の天井高を `1 Grid` / `2 Grids` / `Random` で切り替えます。見た目の印象だけでなく、通路側装飾の余白にも影響します。

`AisleCeilingHeightPolicy` の選び方:
- `OneGrid`  
  低い天井で圧迫感を出したいときに向いています。
- `TwoGrids`  
  開放感を優先したいときや、背の高い装飾を通路側で使いたいときに向いています。
- `Random`  
  低い通路と高い通路を混ぜて変化を出したいときに向いています。

通路の見た目は `DungeonAisleMeshPartsDatabase` 側の Mesh Set と合わせて調整してください。  
シャンデリアなど天井側の演出を強くしたい場合は、[UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md) の設定も一緒に見直すと整理しやすいです。

## 部屋の雰囲気を変える項目
- `GenerateSlopeInRoom`  
  部屋内スロープの生成を許可します。
- `GenerateStructuralColumn`  
  部屋内の構造柱生成を許可します。
- `SkylightChancePercent`  
  部屋内にスカイライト用ボクセルを作る確率です。

## パーツ・フィクスチャ設定
- `PillarPartsSelectionPolicy` / `PillarParts`
- `TorchPartsSelectionPolicy` / `TorchParts`
- `DoorPartsSelectionPolicy` / `DoorParts`

ここでは、柱・燭台・ドアの候補と選択ルールを設定します。  
現在の編集対象は `SelectionMethod` ではなく **`SelectionPolicy`** です。

- `Random`  
  ランダムに選びます。
- `Grid Index` / `Direction` / `Identifier` / `Depth From Start`  
  グリッドや進行度に応じて決定的に選びます。
- `Custom Selector`  
  独自セレクタを使います。

`Custom Selector` を使う場合は、`Custom Dungeon Parts Selector` に `UDungeonPartsSelector` 派生オブジェクトを指定します。  
サンプル実装として `UDungeonSamplePartsSelector` が用意されています。

## 参照するデータベース
- `DungeonRoomMeshPartsDatabase`  
  部屋の床 / 壁 / 天井 / シャンデリアなどを決める DB
- `DungeonAisleMeshPartsDatabase`  
  通路の床 / 壁 / 天井などを決める DB
- `DungeonInteriorDatabase`  
  家具や植生をタグで出し分ける DB
- `DungeonSubLevelDatabase`  
  スタート / ゴール / 特殊部屋サブレベルを登録する DB
- `DungeonRoomSensorDatabase`  
  部屋侵入センサーや通路演出を登録する DB
- `DungeonRoomSensorClass`  
  旧設定です。新規設定では `DungeonRoomSensorDatabase` を使ってください。

## 検証時に見られるポイント
エディタの `Window > DungeonGenerator` にある `Verify` は、主に次の問題を確認します。

- 部屋用 / 通路用 DB が未設定
- 部屋用 / 通路用 DB に床 / 壁 / 天井メッシュがない
- 部屋候補数が少なすぎる
- 参照アセットのパスが壊れている

生成前に `Verify` を通しておくと、初心者が詰まりやすいポイントをかなり減らせます。

## 迷ったときの初期設定
- `RandomSeed = 0`
- `NumberOfCandidateRooms = 10`
- `ExpansionPolicy = ExpandHorizontally`
- `StartLocationPolicy = UseSouthernMost`
- `UseMissionGraph = false`
- `AisleComplexity = 5`

## 補足
- `PluginVersion` はサポートや不具合報告時の確認用です。
- `Flat` は旧設定です。新規構成では `ExpansionPolicy = Flat` を使ってください。

## 次に読む
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)  
  見た目を変えるための Mesh Set の組み方を確認できます。
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)  
  特殊部屋や開始部屋サブレベルを使いたい場合の次の設定先です。

## 関連ページ
- [QuickStart.ja.md](./QuickStart.ja.md)
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)
- [UDungeonRoomSensorDatabase.ja.md](./UDungeonRoomSensorDatabase.ja.md)

