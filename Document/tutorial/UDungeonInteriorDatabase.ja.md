# UDungeonInteriorDatabase ガイド

`UDungeonInteriorDatabase` は、**家具・装飾・植生をタグで出し分けるためのデータベース**です。  
部屋ごとに雰囲気を変えたいときや、特定タグの部屋だけ家具や草木を増やしたいときに使います。

## 何を管理するか
- `Interior Parts`  
  家具や装飾など、アクターとして置く内装パーツ
- `VegetationParts`  
  草、蔦、フォリッジなど、植生系の装飾

## タグの流れ
内装の選択は、部屋側が返すタグと Database 側のタグが一致したときに行われます。  
主な入力元は次の 2 つです。

- `ADungeonRoomSensorBase::GetInquireInteriorTags`
- `UDungeonInteriorLocationComponent::InquireInteriorTags`

たとえば部屋側が `library` を返し、Database 側の内装パーツが `library` を持っていれば、その家具が候補になります。

## 主な項目
- `Interior Parts`
  - 家具や装飾のアクタークラス
  - システムタグ
  - 追加タグ
  - 出現頻度
  - 重なりチェック
  - スポーン方法
- `VegetationParts`
  - 植生用メッシュ
  - タグ
  - 密度
  - 傾斜条件
  - カリング距離

## `Build` が必要な理由
`Interior Parts` は、`Build` 時にアクターのバウンディングボックス情報を事前計算します。  
家具のサイズやクラスを変えたあとに `Build` をしておかないと、配置判定が古い情報のままになる可能性があります。

## 使い方の流れ
1. 家具や装飾の Blueprint / C++ アクターを `Interior Parts` に登録します。
2. 必要なら植生を `VegetationParts` に登録します。
3. 部屋側または `DungeonInteriorLocationComponent` 側で、出し分けたいタグを返します。
4. `Build` を実行します。
5. `UDungeonGenerateParameter` の `DungeonInteriorDatabase` に割り当てます。

## 編集のヒント
- 最初は `start`、`goal`、`hall` のような大きな分類タグから始めると管理しやすいです。
- `kitchen`、`library` などの追加タグは、その後で拡張すると整理しやすくなります。
- 家具だけ差し替えたい場合は `Interior Parts`、草木だけ変えたい場合は `VegetationParts` を編集します。

## 次に読む
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)  
  このデータベースのタグを部屋イベント側から使う方法を確認できます。
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)  
  生成設定へこのデータベースを割り当てる場所を確認できます。

## 関連ページ
- [ADungeonRoomSensorBase.ja.md](./ADungeonRoomSensorBase.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)

