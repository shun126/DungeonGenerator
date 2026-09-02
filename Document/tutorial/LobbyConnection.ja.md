# ロビー接続ガイド

既存ロビーとダンジョンを繋げたい場合は、`ADungeonGenerateActor` の `StartRoomSubLevelScriptActor` を使う方法と、`UDungeonSubLevelDatabase` の `StartRoom` を使う方法があります。

## いつ使うか
- すでにレベル内にあるロビーから、そのままダンジョンへ繋げたい
- スタート部屋を手作りサブレベルで管理したい
- パッケージ版でも開始部屋の見た目とサイズを固定したい

## 方法 A: 配置済みロビーを開始部屋として使う
`ADungeonGenerateActor` の `StartRoomSubLevelScriptActor` に、レベル内の `ADungeonSubLevelScriptActor` を指定します。

### 向いているケース
- すでにロビーがレベル内に存在する
- ロビーからダンジョンへ直接繋げたい
- 生成後に開始部屋の位置を既存ロビーへ揃えたい

### 手順
1. ロビー側レベルに `ADungeonSubLevelScriptActor` を配置します。
2. `ADungeonGenerateActor` の `StartRoomSubLevelScriptActor` にそのアクターを割り当てます。
3. `UDungeonGenerateParameter` で `Path.bMovePlayerStartToStartRoom = false` を設定します。
4. `UDungeonGenerateParameter` で `Path.StartRoomPolicy = UseCentralPoint` を設定します。
5. `UseMultiStart` は使わず、通常の 1 開始地点で生成します。

### 現行実装で重要な注意
- `UseMultiStart` は未対応で、指定するとエラーになります
- 選択したプリロード済みスタート部屋のメタデータは `ADungeonGenerateActor` に保存されます
- `StartRoomSubLevelScriptActor` が設定されている間、`Gameplay.DungeonSubLevelDatabase.StartRoom` は開始部屋には使われません
- `Gameplay.DungeonSubLevelDatabase.StartRoom` が別レベルを指していても、警告を表示して生成は続行します

## 方法 B: スタート部屋サブレベルを生成側から差し込む
`UDungeonSubLevelDatabase` の `StartRoom` を使って、開始部屋を通常の生成フローに含めます。

### 向いているケース
- 開始部屋も生成設定アセット側で管理したい
- 既存ロビーを常駐させず、開始部屋をサブレベルとして扱いたい
- エディタとパッケージ版で同じメタデータを明示的に管理したい

### 手順
1. 開始部屋用サブレベルに `ADungeonSubLevelScriptActor` を配置します。
2. グリッドサイズと部屋サイズを `UDungeonGenerateParameter` に合わせます。
3. [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md) の `StartRoom` にそのサブレベルを登録します。
4. `Build` を実行してメタデータを更新します。
5. `UDungeonGenerateParameter` の `Gameplay.DungeonSubLevelDatabase` にそのアセットを割り当てます。

## どちらを選ぶか
- 既存ロビーにそのまま繋ぎたい  
  方法 A
- 開始部屋も生成アセット側で完結させたい  
  方法 B

## 次に読む
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)  
  `StartRoomSubLevelScriptActor` を含む生成アクター側の設定を確認できます。
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)  
  `StartRoom` の登録や `Build` 手順を詳しく確認できます。

## 関連ページ
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)
- [ADungeonSubLevelScriptActor.ja.md](./ADungeonSubLevelScriptActor.ja.md)
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)

