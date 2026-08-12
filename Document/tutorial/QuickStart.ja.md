# Dungeon Generator クイックスタート

このページでは、**最短でダンジョンを 1 つ生成して確認する手順**を説明します。
まずはエディタ上で見た目を確認し、その後でレベル配置用の `ADungeonGenerateActor` に繋げる流れをおすすめします。

## ゴール
- `Generate parameter` を 1 つ作る
- 部屋用 / 通路用の `Mesh set database` を作る
- `Verify` で不足設定を確認する
- `Generate dungeon` で生成結果を確認する

## 始める前に: プラグインを有効化する
初回のみ、先に `DungeonGenerator` プラグインを有効化します。
Unreal Editor のメニューから次の順で操作してください。

![enable-plugin-ja](./images/LoadPlugin.png)

1. `編集 > プラグイン` を開きます。
2. `DungeonGenerator` プラグインを見つけて有効化します。
3. 再起動を求められた場合は、エディタを再起動します。

有効化が終わると、Content Browser の `DungeonGenerator` カテゴリから必要なアセットを作成できるようになります。

## 最初に同梱サンプルを試す

Dungeon Generator には、次の 2 つのサンプルマップがプラグインコンテンツとして同梱されています。別のデモプロジェクトをダウンロードする必要はありません。

- `Content/Maps/Demonstration.umap`
  同梱コンテンツを使った標準的なランタイム生成、ゲームプレイ、ミニマップを確認できます。最初はこちらを開いてください。
- `Content/Maps/DemonstrationWithStartRoom.umap`
  制作者が用意した開始部屋のサブレベルと、生成されたダンジョンを接続する例を確認できます。

Content Browser の設定で `Show Plugin Content` を有効にし、`DungeonGenerator Content/Maps` を開いて、確認したいマップをダブルクリックします。Play を押すとサンプルを実行できます。

## 1. 必須アセットを作る
Content Browser の `DungeonGenerator` カテゴリから、次のアセットを作成します。

1. `Generate parameter`
2. `Mesh set database`
   部屋用
3. `Mesh set database`
   通路用

最初はこの 3 つだけで十分です。
`Interior database`、`Sub level database`、`Gameplay.DungeonRoomSensorClass` はあとから追加できます。

最初に見た目を確認したら、次は `Gameplay.DungeonRoomSensorClass` を追加し、`ADungeonRoomSensorBase` の Helper パラメータを試す流れがおすすめです。`DungeonGenerator|Helper` の設定を使うと、細かい Blueprint 制御に進む前に、敵や鍵アクターの簡単なスポーンをすばやく確認できます。

## 2. 部屋用 / 通路用メッシュを最低限登録する
部屋用と通路用の `Mesh set database` を開き、最初の `Mesh Set` に次のパーツを登録します。

- `Floor Parts`
- `Wall Parts`
- `Roof Parts`
- `Slope Parts`（高低差が生じるレイアウトでは推奨）

部屋用と通路用の各データベースでは、床 / 壁 / 天井メッシュが必須です。スロープ未設定はエラーではなく警告ですが、高低差が生成されても想定した移動面が表示されません。上下移動を作る Floor Mode やレイアウトを使う前に、スロープメッシュも登録してください。

## 3. `Generate parameter` にデータベースを割り当てる
`Generate parameter` を開き、最低限次の項目を設定します。

- `Theme.DungeonRoomMeshPartsDatabase`
  部屋用 `Mesh set database`
- `Theme.DungeonAisleMeshPartsDatabase`
  通路用 `Mesh set database`

最初の確認には、次の設定値が分かりやすいです。

- `RandomSeed = 0`
- `Structure.RoomCountRange = 10-10`
- `Path.StartRoomPolicy = UseSouthernMost`
  北・南・東・西・上下・中央からスタート部屋の位置を選べます。
- `Path.ExtraCorridorComplexity = 0`

最初の確認では経路調整値を `0` のままにして、まず `Path.ProgressionPolicy` の標準形状を確認してください。

## 4. エディタでプレビューする
`Window > DungeonGenerator` を開きます。
ウィンドウ内で `Generate parameter` を選択し、次の順で操作します。

1. `Verify`
2. エラーがあれば修正
3. `Generate dungeon`

`Verify` は、たとえば次の不足を検出します。

- 部屋用 / 通路用 DB が未設定
- 床 / 壁 / 天井メッシュが未設定
- ルーム数が少なすぎて失敗しやすい

問題を報告用にまとめたい場合は `Copy diagnostics` を使います。

## 5. レベルに組み込む
実際のレベルで使うときは、`ADungeonGenerateActor` を配置し、`DungeonGenerateParameter` に同じ `Generate parameter` を指定します。

- レベル開始時に自動生成したい
  `AutoGenerateAtStart = true`
- 任意タイミングで生成したい
  Blueprint などから `GenerateDungeon` または `GenerateDungeonWithParameter` を呼ぶ

詳細は [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md) を参照してください。

## よくある失敗
- 生成ボタンを押しても何も出ない
  `Verify` を実行し、床 / 壁 / 天井メッシュが入っているか確認してください。
- 高低差は生成されたが、階段やスロープが見えない
  部屋用と通路用の Mesh Set に `Slope Parts` を追加し、もう一度 `Verify` を実行してください。
- 見た目は出たがレベルに組み込めない
  `ADungeonGenerateActor` に `DungeonGenerateParameter` を割り当てているか確認してください。
- サブレベルを使ったらサイズがずれる
  `UDungeonSubLevelDatabase` の `Build` と、サブレベル側 `ADungeonSubLevelScriptActor` のグリッドサイズ確認が必要です。

## 次に読む
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)
