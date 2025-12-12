# UDungeonMeshSetDatabase 説明書

UDungeonMeshSetDatabase は、床・壁・天井などの**メッシュセットをまとめて管理し、生成時にどのセットを使うかを抽選するデータベース**です。セットごとに FDungeonMeshParts を束ねた「テーマ」を作り、ダンジョンの深さや識別子に応じて出し分けできます。

## 主な使い方
- テーマや階層ごとに異なるメッシュ構成（例: 1F は石造り、B1F は錆びた金属）を Parts に登録します。
- SelectionMethod で、どのルールでセットを選ぶかを決めます。深さに応じて変える、完全ランダムにする、といった切り替えが可能です。
- [UDungeonGenerateParameter](./UDungeonGenerateParameter.ja.md) の「部屋用/通路用メッシュパーツ DB」にこのアセットを指定すると、生成時に参照されます。

## UPROPERTY の意味
- **SelectionMethod (`EDungeonMeshSetSelectionMethod`, EditAnywhere/BlueprintReadOnly)**  
  メッシュセットの選び方。深さに応じた重み付けや単純ランダムなど、用意された方式から選びます。迷ったらデフォルトの「DepthFromStart」でスタート地点からの距離に応じてテーマを変えるのがおすすめです。
- **Parts (`TArray<FDungeonMeshSet>`, EditAnywhere/BlueprintReadOnly)**
  実際のメッシュセットを並べる配列です。各 FDungeonMeshSet に床・壁・天井などの [FDungeonMeshParts](./FDungeonMeshParts.ja.md) を含めておくと、抽選されたセット内でさらに部位ごとのパーツが使われます。

## 編集のヒント
- セットの並び順は SelectionMethod によって解釈が変わる場合があります。深さで使い分けたいときは「DepthFromStart」で、バリエーションだけ欲しいときはランダム系を選ぶとシンプルです。
- Parts を複数登録し、各セットの中にも複数のパーツを入れておくと、同じテーマでも細かな違いを出せます。

