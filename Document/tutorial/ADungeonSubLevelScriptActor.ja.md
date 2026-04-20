# ADungeonSubLevelScriptActor ガイド

`ADungeonSubLevelScriptActor` は、**特殊部屋用サブレベルを Dungeon Generator に読み取らせるための親クラス**です。  
`UDungeonSubLevelDatabase` に登録する各レベルは、この親クラスを使ってサイズやグリッド情報を持たせます。

## いつ使うか
- ボス部屋を固定デザインにしたい
- スタート部屋やゴール部屋を手作りしたい
- 「たまに出る隠し部屋」のような特別ルームを差し込みたい

## 作成手順
1. 特殊部屋用のレベルを作成します。
2. そのレベルの Level Blueprint の親クラスを `ADungeonSubLevelScriptActor` に変更します。
3. `Horizontal Size` / `Vertical Size` を本体の `UDungeonGenerateParameter` と同じ値に設定します。
4. `Width` / `Depth` / `Height` に、部屋サイズを**グリッド数**で入力します。
5. 必要なら `ShowGrid` を有効にして、メッシュをグリッドに合わせて配置します。
6. そのレベルを [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md) に登録し、`Build` を実行します。

## 重要なポイント
- `Width` / `Depth` / `Height` はセンチメートルではなく**グリッド数**です。
- `Horizontal Size` / `Vertical Size` は、本体側の `GridSize` / `VerticalGridSize` と一致している必要があります。
- `UDungeonSubLevelDatabase` の `Build` は、このクラスからサイズとグリッド情報を読み取ってデータベースへコピーします。

## 編集時の注意
- サブレベルの入口位置と壁抜け位置は、あとで自動生成部屋と接続される前提で配置してください。
- `Build` 時にはレベルを一度ロードして解析するため、**現在編集中のそのレベル自身**はビルド対象にしない方が安全です。
- グリッドサイズが一致しない場合、`UDungeonSubLevelDatabase` のビルド時にエラーになります。

## 次に読む
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)  
  このアクターをどの部屋に使うかを登録するデータベースです。
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)  
  開始部屋サブレベルをどう接続するかを用途別に確認できます。

## 関連ページ
- [UDungeonSubLevelDatabase.ja.md](./UDungeonSubLevelDatabase.ja.md)
- [LobbyConnection.ja.md](./LobbyConnection.ja.md)
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)

