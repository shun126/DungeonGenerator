# カスタムセレクターガイド

セレクターは、生成時にどの見た目候補を使うか決める機能です。Version 2 では、メッシュセットを選ぶセレクターと、その中のパーツを選ぶセレクターを直接設定します。

## 正しいセレクターの種類を選ぶ

- `UDungeonMeshSetDatabase` から `FDungeonMeshSet` を選ぶ場合は、`UDungeonBlueprintMeshSetSelector` 派生 Blueprint を作り、`Select Mesh Set Index` を実装します。
- 床、壁、天井、スロープ、キャットウォーク、シャンデリア、柱、松明、扉の候補を選ぶ場合は、`UDungeonBlueprintPartsSelector` 派生 Blueprint を作り、`Select Parts Index` を実装します。

`UDungeonPartsSelector` は C++ で複数のセレクター型をまとめて読み込むためのヘッダーであり、Blueprint の親クラスではありません。

## セレクターを設定する場所

Mesh Set 全体を選ぶ場合は、`Mesh set database` の `Mesh Set Selector` に設定します。

`FDungeonMeshSet` 内のパーツを選ぶ場合は、対象に対応するプロパティを使います。

- `Floor Parts Selector`
- `Wall Parts Selector`
- `Roof Parts Selector`
- `Slope Parts Selector`
- `Catwalk Parts Selector`
- `Chandelier Parts Selector`

共通 Fixture は、`UDungeonGenerateParameter` の `Theme.Fixtures.Pillar Parts Selector`、`Torch Parts Selector`、`Door Parts Selector`、`Unique Door Parts Selector` に設定します。

これらはプロパティ内に保持されるインラインのセレクターオブジェクトです。空の場合は Uniform Random が自動的に割り当てられるため、別の Selection Policy を変更する必要はありません。

## 組み込みセレクター

Mesh Set 用には Uniform Random、Identifier、Depth From Start があります。パーツ用には Uniform Random、Grid Index、Direction があります。必要な規則を表せる場合は組み込みセレクターを使い、プロジェクト固有の条件が必要な場合だけ Blueprint セレクターを作ると管理しやすくなります。

## Blueprint での作成手順

1. `UDungeonBlueprintMeshSetSelector` または `UDungeonBlueprintPartsSelector` 派生 Blueprint を作ります。
2. `Select Mesh Set Index` または `Select Parts Index` をオーバーライドします。
3. `Query` を調べ、`0` から `NumCandidates - 1` のインデックスを返します。
4. 上記の対応するインラインセレクタープロパティで、その Blueprint セレクターを選びます。
5. 固定 Seed を複数試し、結果が安定していることを確認します。

範囲外の値を返すと警告が出て Uniform Random にフォールバックするため、生成自体は続行できます。ただし、フォールバックを通常動作として使わず、セレクターの不具合として修正してください。

## Query の値

Mesh Set 選択には `FDungeonMeshSetQuery`、個別パーツ選択には `FDungeonPartsQuery` が渡されます。

どちらにも `GridX`、`GridY`、`GridZ` などのダンジョン内ローカルグリッド座標と、決定的な選択に使える `SeedKey` が含まれます。Unreal のワールド座標ではありません。Parts Query には対象の種類、回転、周囲セルなども含まれるため、周辺形状に応じた規則を作れます。

決定的なバリエーションが必要な場合は `Query.SeedKey` を使ってください。時刻、同期されない Random ノード、非同期処理、外部参照は避けます。セレクターは生成中に何度も呼ばれ、同じ入力から同じ結果を返す必要があります。

## サンプル

プラグインには `UDungeonSampleMeshSetSelector` と `UDungeonSamplePartsSelector`、Blueprint サンプルの `BP_SampleDungeonMeshSetSelector` と `BP_SampleDungeonPartsSelector` が含まれます。決定的な重み付き選択や Query を使った規則を確認し、必要な部分から少しずつ条件を追加してください。

## Version 1 に関する注意

実装上、非表示の旧 Policy や旧セレクターフィールドが保存データに残る場合がありますが、Version 2 で使用する設定方法ではなく、Version 1 から Version 2 への移行をサポートするものでもありません。表示されている Version 2 のセレクタープロパティを使って手動で設定を作り直してください。

## 関連ページ

- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)
