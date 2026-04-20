# カスタムセレクタガイド

`Custom Selector` は、既定の `Random` / `Identifier` / `Depth From Start` では足りないときに、`UDungeonPartsSelector` 派生アセットで選択ルールを差し替える仕組みです。

## いつ使うか
- 部屋の周囲形状や進行度に応じて、見た目を細かく変えたい
- 決定的な重み付き抽選で、サーバー / クライアントの結果を揃えたい
- 既定ポリシーでは表現しづらいテーマ切り替えをしたい

## 基本の流れ
1. `UDungeonPartsSelector` 派生の Blueprint または C++ クラスを作ります。
2. `SelectMeshSetIndex` または `SelectPartsIndex` を実装します。
3. 使いたい場所の `Selection Policy` を `Custom Selector` に変更します。
4. 対応する selector プロパティへ作成したアセットを割り当てます。

## メッシュセットを独自ルールで選ぶ
`UDungeonMeshSetDatabase` で次のように設定します。

1. `Mesh Set Selection Policy = Custom Selector`
2. `Custom Mesh Set Selector` に `UDungeonPartsSelector` 派生アセットを割り当てる

このとき `SelectMeshSetIndex` が呼ばれ、`FMeshSetQuery` と候補数を受け取って採用する `Mesh Set` の index を返します。

## 各パーツを独自ルールで選ぶ
床 / 壁 / 天井 / スロープ / キャットウォーク / シャンデリアや、柱 / 燭台 / ドアなども `Custom Selector` にできます。

- `FDungeonMeshSet` 側の床 / 壁 / 天井 / スロープ / キャットウォーク / シャンデリア  
  各 `*PartsSelectionPolicy` を `Custom Selector` にし、`Custom Mesh Parts Selector` を割り当てます。
- `UDungeonGenerateParameter` 側の柱 / 燭台 / ドアなど  
  各 `*SelectionPolicy` を `Custom Selector` にし、`Custom Dungeon Parts Selector` を割り当てます。

このとき `SelectPartsIndex` が呼ばれ、`FPartsQuery` と候補数を受け取って最終的なパーツ index を返します。

## サンプルセレクタを使う
サンプルとして `UDungeonSamplePartsSelector` が用意されています。

- `FMeshSetQuery`  
  `Mesh Set` を選ぶときの問い合わせです。
- `FPartsQuery`  
  個別パーツを選ぶときの問い合わせです。`NeighborMask6` や `SeedKey` を使った決定的選択の例があります。

まずはこのサンプルを複製して、必要な条件を少しずつ足していくと整理しやすいです。

## 実装時の注意
- selector は生成のホットパスをゲームスレッドで実行されます
- 重い処理、非同期処理、外部参照の多い処理は避けてください
- 非同期乱数や時刻依存の分岐は、サーバー / クライアント差分の原因になるので避けてください
- 返す index は `0` 以上 `NumCandidates - 1` 以下に収めてください

## 旧方式との違い
`UDungeonGenerateParameter::SelectMeshSetIndex` も残っていますが、現行構成では旧アセット互換寄りの経路です。  
新規構成では selector asset を使う方が、どこで何を切り替えているかを整理しやすくなります。

## 次に読む
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)  
  `Mesh Set` 側の選択ポリシーやシャンデリア設定をまとめて確認できます。
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)  
  応用設定をレベル内の生成アクターへ反映する流れを確認できます。

## 関連ページ
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)

