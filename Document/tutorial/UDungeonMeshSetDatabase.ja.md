# UDungeonMeshSetDatabase ガイド

`UDungeonMeshSetDatabase` は、**床・壁・天井・スロープ・キャットウォーク・シャンデリアをまとめた「見た目のテーマ集」**です。  
部屋用と通路用で別々の Database を用意し、`UDungeonGenerateParameter` から参照します。

## 役割
- どの `FDungeonMeshSet` を使うかを決める
- 選ばれた `FDungeonMeshSet` の中から、床 / 壁 / 天井などの個別パーツを選ぶ

言い換えると、**Phase 1 は Database が「どのテーマを使うか」を決め、Phase 2 は Mesh Set が「そのテーマ内のどのパーツを使うか」を決めます。**

## 最低限の使い方
1. `Mesh set database` アセットを作ります。
2. `Mesh Set` を 1 つ追加します。
3. その中に最低限次のパーツを入れます。  
   `Floor Parts` / `Wall Parts` / `Roof Parts`
4. それを `UDungeonGenerateParameter` の部屋用または通路用 DB に割り当てます。

## 主な項目
- `Mesh Set Selection Policy` (`SelectionPolicy`)  
  この Database 内の複数 `Mesh Set` から、どれを使うかを決めます。
- `Custom Mesh Set Selector`  
  `Mesh Set Selection Policy = Custom Selector` のときだけ使う独自 selector です。
- `Mesh Set` (`Parts`)  
  実際のテーマ一覧です。各要素が `FDungeonMeshSet` です。

## `Mesh Set Selection Policy` の考え方
- `Random`  
  同じテーマ帯の中でばらつきを出したいとき向けです。
- `Identifier`  
  部屋識別子ベースで決定的に選びます。
- `Depth From Start`  
  スタートからの進行度に応じてテーマを変えたいときに便利です。
- `Custom Selector`  
  独自ロジックで `Mesh Set` を選びたいときに使います。

新規構成では `SelectionMethod` ではなく、**`SelectionPolicy` を見る**ようにしてください。  
`SelectionMethod` は旧アセット互換のために保持されている項目です。

## `FDungeonMeshSet` 側で決めること
各 `FDungeonMeshSet` には、次のような配列と選択ポリシーがあります。

- `Floor Parts`
- `Wall Parts`
- `Roof Parts`
- `Slope Parts`
- `Catwalk Parts`
- `Chandelier Parts`

シャンデリアは Database 全体ではなく、**各 Mesh Set の中**で管理します。  
深度ごとに豪華さを変えたい場合は、Mesh Set を分けるのが分かりやすいです。

## シャンデリアを設定する
シャンデリアは `UDungeonMeshSetDatabase` の共通設定ではなく、**各 `FDungeonMeshSet` の装飾設定**として調整します。

### 手順
1. `UDungeonMeshSetDatabase` を開きます。
2. 対象の `Mesh Set` を選びます。
3. `Chandelier Parts` に `FDungeonRandomActorParts` を追加します。
4. 必要に応じて次の項目を調整します。
   - `ChandelierPartsSelectionPolicy`
   - `ChandelierMinSpacing`
   - `ChandelierMinCeilingHeight`
   - `ChandelierRadius`
   - `ChandelierWallWeight`
   - `ChandelierCombatWeight`

### 主な項目の意味
- `Chandelier Parts`  
  実際に配置するシャンデリア候補です。`ActorClass` やスポーン条件は `FDungeonRandomActorParts` 側で設定します。
- `ChandelierPartsSelectionPolicy`  
  複数候補があるときに、どのシャンデリアを選ぶかを決めます。
- `ChandelierMinSpacing`  
  シャンデリア同士の間隔です。密集しすぎる場合は大きくします。
- `ChandelierMinCeilingHeight`  
  配置に必要な最小天井高です。低い天井テーマでは干渉防止に重要です。
- `ChandelierRadius`  
  配置時の衝突確認半径です。大きな装飾ほど広めに取ります。
- `ChandelierWallWeight` / `ChandelierCombatWeight`  
  壁から離れた位置や、戦闘の中心寄りの位置をどれだけ優先するかを決めます。

### 運用のコツ
- 天井が低いテーマでは `ChandelierMinCeilingHeight` を高めにして、干渉する場所を減らします。
- 深い階層ほど豪華にしたい場合は、シャンデリア込みの `Mesh Set` を別テーマとして分けると管理しやすいです。
- 通路用と部屋用で演出を分けたい場合は、Database ではなく各 `Mesh Set` 側で出し分けます。

## 編集のヒント
- 部屋用 DB と通路用 DB は分けて管理すると、見た目の調整がしやすくなります。
- まずは `Mesh Set` を 1 つだけ作り、そこに最低限のパーツを揃えてから、2 つ目以降を追加すると破綻しにくいです。
- `Verify` は、床 / 壁 / 天井メッシュが 1 つもない状態を検出します。
- `Custom Selector` を使う場合は、重い処理を避け、決定的に動くようにしてください。

## 次に読む
- [FDungeonMeshParts.ja.md](./FDungeonMeshParts.ja.md)  
  各メッシュ枠に入れる設定項目を個別に確認できます。
- [CustomSelector.ja.md](./CustomSelector.ja.md)  
  `Custom Selector` を使って選択ルールを差し替える流れを確認できます。

## 関連ページ
- [FDungeonMeshParts.ja.md](./FDungeonMeshParts.ja.md)
- [FDungeonRandomActorParts.ja.md](./FDungeonRandomActorParts.ja.md)
- [UDungeonGenerateParameter.ja.md](./UDungeonGenerateParameter.ja.md)
- [CustomSelector.ja.md](./CustomSelector.ja.md)

