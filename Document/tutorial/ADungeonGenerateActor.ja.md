# ADungeonGenerateActor ガイド

`ADungeonGenerateActor` は、**レベル上に置いてダンジョン生成を実行する本番用アクター**です。  
エディタのプレビューだけなら `Window > DungeonGenerator` の方が速いですが、ゲーム中に生成したい場合や、レベルに固定で置きたい場合はこちらを使います。

## 役割
- `DungeonGenerateParameter` を読み込んでダンジョンを生成する
- 生成済みダンジョンを破棄する
- 必要に応じてミニマップやサブレベル連携を扱う

## 最小セットアップ
1. レベルに `ADungeonGenerateActor` を配置します。
2. `DungeonGenerateParameter` に `Generate parameter` アセットを指定します。
3. 必要に応じて `AutoGenerateAtStart` を設定します。
4. プレイ開始時または Blueprint から `GenerateDungeon` を呼びます。

## 主なプロパティ
- `DungeonGenerateParameter`  
  必須です。生成ルール、見た目、サブレベル、センサー設定はほぼこのアセット経由で決まります。
- `AutoGenerateAtStart`  
  レベル開始時に自動生成するかどうかです。まずは `true` のまま確認すると分かりやすいです。
- `Dungeon Floor Slope Mesh Generation Method`  
  床 / スロープ / キャットウォークの生成方式です。大量配置ならインスタンス系が有利です。
- `DungeonWallRoofPillarMeshGenerationMethod`  
  壁 / 天井 / 柱の生成方式です。大量の壁や柱を使う場合は `Hierarchical Instanced Static Mesh` が有効です。
- `StartRoomSubLevelScriptActor`  
  すでにロード済みのロビー用サブレベルを、スタート部屋として扱いたいときに使います。

## 生成の呼び出し方
- `GenerateDungeon`  
  すでに割り当て済みの `DungeonGenerateParameter` を使って生成します。
- `GenerateDungeonWithParameter`  
  呼び出し時に別の `DungeonGenerateParameter` を渡して生成します。
- `DestroyDungeon`  
  生成済みダンジョンを破棄します。

## ロビーと接続したい場合
ロード済みロビーをスタート部屋として使いたい場合は、`StartRoomSubLevelScriptActor` を利用します。  
このときは次の制約があります。

- `UDungeonGenerateParameter` 側で `MovePlayerStartToStartingPoint = false` を設定します
- `UDungeonGenerateParameter` 側で `StartLocationPolicy = NoAdjustment` を設定します
- `UseMultiStart` は併用できません
- 候補に指定できるのは、現在ロード済みの `ADungeonSubLevelScriptActor` を持つサブレベルです

ロビーを事前ロードせず、通常のサブレベル差し替えで固定スタート部屋を使いたい場合は [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md) を使ってください。  
使い分けは [LobbyConnection.ja.md](./LobbyConnection.ja.md) にまとめています。

## 運用のヒント
- まずはエディタの `Verify` で `DungeonGenerateParameter` を検証してから、このアクターに割り当てると手戻りが減ります。
- 見た目だけ確認したい段階では、プラグインウィンドウでのプレビューの方が速いです。
- ミニマップやサブレベルなどを追加するのは、基本生成が安定してからにするとデバッグしやすいです。

## 次に読む
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)  
  このアクターが参照する設定アセット全体を確認できます。
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)  
  既存ロビーや開始部屋サブレベルと接続したい場合の専用ガイドです。

## 関連ページ
- [QuickStart.ja.md](./QuickStart.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)

