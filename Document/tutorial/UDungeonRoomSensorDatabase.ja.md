# UDungeonRoomSensorDatabase ガイド

`UDungeonRoomSensorDatabase` は、**どの部屋にどの `ADungeonRoomSensorBase` 派生クラスを使うかを決めるデータベース**です。  
部屋侵入イベント、BGM 切り替え、敵スポーン、トラップ発動などを部屋ごとに変えたいときに使います。

## 使い方の流れ
1. `ADungeonRoomSensorBase` を親クラスにした Blueprint を作ります。
2. その Blueprint を `DungeonRoomSensorClass` に登録します。
3. `SelectionMethod` で、どのルールでセンサーを選ぶかを決めます。
4. 必要に応じて `SpawnActorInAisle` に通路演出用 Blueprint を登録します。
5. `UDungeonGenerateParameter` の `DungeonRoomSensorDatabase` にこの Database を指定します。

## 主な項目
- `SelectionMethod`
  - `Random`  
    毎回ランダムに選びます。
  - `Identifier`  
    部屋識別子に応じて決定的に選びます。
  - `Depth From Start`  
    スタートからの進行度に応じて選びます。
- `DungeonRoomSensorClass`  
  配置候補となる `ADungeonRoomSensorBase` 派生クラスの一覧です。
- `SpawnActorInAisle`  
  生成完了後に通路へ追加配置する Blueprint 群です。

## `SpawnActorInAisle` について
これは**部屋の中**に置くアクターではなく、**通路側へ追加演出を置く設定**です。  
部屋の中の敵や宝箱は、`ADungeonRoomSensorBase` 派生 Blueprint 側で扱う方が分かりやすいです。

## 編集のヒント
- 深い階層ほど危険な部屋にしたい場合は、`Depth From Start` が使いやすいです。
- センサー選択は「どのクラスを置くか」を決めるだけなので、部屋に入ったあと何をするかは各センサー Blueprint に実装します。
- `Custom` 相当の高度な選択は、この Database 単体では主導線ではありません。まずは `Random` / `Identifier` / `Depth From Start` で構成するのがおすすめです。

## 次に読む
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)  
  センサー Blueprint 側で実装するイベントとプロパティを確認できます。
- [UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md)  
  センサーから参照する内装タグや問い合わせ先を整理できます。

## 関連ページ
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [UDungeonInteriorDatabase.ja.md](./UDungeonInteriorDatabase.ja.md)

