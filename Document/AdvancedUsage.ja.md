# Dungeon Generator 応用ガイド

このドキュメントでは、以下の4つの要望に対応する設定方法をまとめます。

- 通路の天井の高さを指定可能
- シャンデリアを DungeonMeshSetDatabase に追加
- メッシュセットとパーツのカスタム選択
- ロビーに接続するダンジョン生成

## 1. 通路の天井の高さを指定する

`UDungeonGenerateParameter` の **AisleCeilingHeightPolicy** を使うことで、通路の天井高を制御できます。

- `TwoGrids`: 常に2グリッド天井
- `OneGrid`: 常に1グリッド天井
- `Random`: 1/2グリッドをランダム選択

### 手順
1. `DungeonGenerateParameter` アセットを開きます。
2. `AisleCeilingHeightPolicy` を目的に合わせて設定します。
3. ダンジョンを再生成し、通路高さを確認します。

## 2. シャンデリアを DungeonMeshSetDatabase に追加する

シャンデリアは各 `FDungeonMeshSet` のシャンデリア用配列に追加して管理します。生成時はメッシュセット選択後、シャンデリア候補からさらにパーツが選ばれます。

### 手順
1. `UDungeonMeshSetDatabase` を開きます。
2. `Parts` から編集対象の `FDungeonMeshSet` を選択します。
3. シャンデリア用の候補（`FDungeonRandomActorParts`）を追加します。
4. 必要に応じてシャンデリアの選択ポリシー/選択方式を調整します。

### 運用のコツ
- 天井が低いエリアでは、干渉しにくい小型シャンデリアを優先します。
- 深度（Depth）別にメッシュセットを分けると、進行に応じた豪華さを演出できます。

## 3. メッシュセットとパーツをカスタム選択する

### 3-1. メッシュセットのカスタム選択
`UDungeonGenerateParameter` の `SelectMeshSetIndex`（BlueprintNativeEvent）を実装すると、候補から任意のメッシュセットインデックスを返せます。

- `SelectionMethod = Custom` のとき呼ばれます。
- 返値が範囲外の場合は、標準選択にフォールバックされます。

#### 例
- ロビー付近は明るいセット
- ゴール付近は重厚なセット
- 特定 RoomId で専用セット

### 3-2. パーツのカスタム選択
`UDungeonSamplePartsSelector` などのパーツセレクタを利用し、`FPartsQuery` / `FMeshSetQuery`（深さ、RoomId、近傍情報など）に応じて選択ルールを作成します。

#### 例
- 角部屋では柱パーツ出現率を上げる
- 通路接続が多い壁では装飾を抑える
- ゴールに近いほど希少パーツを優先

## 4. ロビーに接続するダンジョンを生成する

「ロビー → 自動生成ダンジョン」の接続は、開始位置ポリシーとサブレベル構成を組み合わせると安定します。

### 推奨構成
1. ロビー側に `PlayerStart` と遷移トリガーを配置します。
2. `UDungeonGenerateParameter` で開始位置方針（`StartLocationPolicy`）を設定します。
3. 必要に応じて `UDungeonSubLevelDatabase` の Start/Goal 設定で固定接続用ルームを用意します。
4. ロビー遷移時にダンジョン生成を実行し、開始部屋へスポーン/移動させます。

### 実装メモ
- 複数 `PlayerStart` を使う場合、`UseMultiStart` でマルチスタート構成を作れます。
- 固定演出の入口部屋を使う場合は、サブレベルを「必ず生成」に登録しておくと、接続品質を担保しやすくなります。
