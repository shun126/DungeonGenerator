# FDungeonMeshParts 説明書

FDungeonMeshParts は、**柱や壁など 1 枚のスタティックメッシュを「どこに・どの向きで」置くかを記録する入れ物**です。配置位置はベースクラスのオフセット情報（FDungeonPartsTransform）で決まり、見た目は `StaticMesh` の差し替えで変わります。レベルデザイナーやアーティストが自前のメッシュを登録し、テーマに合わせて柱・壁・天井などのパーツを切り替える用途を想定しています。

## 主な使い方
- 「この柱メッシュを 30cm 右に、90 度回して置いてほしい」といった要望を、`StaticMesh` とオフセットで表現します。
- メッシュセット（[UDungeonMeshSetDatabase](./UDungeonMeshSetDatabase.ja.md)）に複数の FDungeonMeshParts を並べると、生成時にランダムやルールに応じて選択されます。

## UPROPERTY の意味
- **StaticMesh (`UStaticMesh*`, EditAnywhere/BlueprintReadWrite)**  
  生成時にスポーンするスタティックメッシュを指定します。未設定だと何も置かれないので、必ず設定してください。メッシュの原点が配置基準になるため、メッシュ側でピボット調整を済ませておくと位置決めが楽になります。

## 編集と配置のヒント
- オフセットや回転はベースクラスの Transform で調整できます。グリッドサイズ（[UDungeonGenerateParameter](./UDungeonGenerateParameter.ja.md) の GridSize/VerticalGridSize）に合わせた値にするとスナップが揃います。
- 同じ役割のメッシュを複数登録しておくと、DB 側の選択ルールでバリエーションを出せます。

