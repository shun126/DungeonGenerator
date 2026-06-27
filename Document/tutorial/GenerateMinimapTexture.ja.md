# ミニマップを生成する

このページでは、生成したダンジョンからミニマップを作り、ゲーム中の UI に表示するまでの流れを説明します。

ミニマップは大きく分けて、次の 2 つの使い方があります。

- エディタでミニマップテクスチャを作り、アセットとして保存する
- ゲーム実行中にミニマップを生成し、Widget に表示する

はじめて使う場合は、まずエディタでテクスチャを作って見た目を確認してください。  
そのあとで、実行時表示、マスク、アイコン表示を追加すると原因を切り分けやすくなります。

## このページのゴール
- エディタでミニマップテクスチャを生成できる
- `UDungeonMiniMapWidget` でゲーム中にミニマップを表示できる
- プレイヤーや目的地のアイコンをミニマップに重ねられる
- アイコンサイズをマップの 1 グリッド幅に合わせられる

## 先に知っておくこと
- ミニマップは **直前に生成されたダンジョン情報** から作られます
- 階層があるダンジョンでは、階層ごとに別のミニマップテクスチャが作られます
- 実行時表示には `UDungeonMiniMapWidget` を使います
- メニュー画面などで全体マップを表示したい場合は `UDungeonMapWidget` を使います
- アイコン表示には、Widget Blueprint に `DungeonIconWidget` を配置します

## 前提
- [QuickStart.ja.md](./QuickStart.ja.md) が完了している
- ダンジョンを正常に生成できる
- レベルに `ADungeonGenerateActor` を配置している、またはエディタの `Window > DungeonGenerator` からプレビューできる

## A. エディタでミニマップテクスチャを作る

まずは、コンテンツブラウザに保存されるミニマップテクスチャを作ります。  
この手順は、ミニマップの色や階層ごとの見え方を確認するために便利です。

### 1. ダンジョンを生成する

ミニマップテクスチャは、最後に生成されたダンジョンをもとに作られます。  
まだダンジョンを生成していない場合は、先に `Generate dungeon` を実行してください。

### 2. 生成ボタンを確認する

有効なダンジョン生成結果がある場合、次のボタンを使えます。

- `Generate texture with size`
- `Generate texture with scale`

ボタンが押せない場合は、ダンジョン生成が失敗しているか、まだ一度も生成していない可能性があります。

![テクスチャ生成画面](images/MiniMap1.png)

### 3. 生成方法を選ぶ

目的に合わせて、どちらかの生成方法を選びます。

- `Generate texture with size`  
  出力テクスチャの一辺のピクセル数を固定したい場合に使います。UI デザイン側で「512 x 512 にしたい」など、最終サイズを先に決めたい時に向いています。
- `Generate texture with scale`  
  1 グリッドあたりのドット数を固定したい場合に使います。アイコンやマスク範囲をグリッド単位で合わせたい時に向いています。

ゲーム中のミニマップでは、まず `Generate texture with scale` と同じ考え方で調整すると分かりやすいです。  
例えば `DotScale` が `20` の場合、マップ上の 1 グリッドは 20 ドットになります。

### 4. 出力先を確認する

生成すると、テクスチャアセットがコンテンツブラウザの `ProceduralTextures` フォルダに作成されます。

![](images/MiniMap2.png)

### 5. 階層ごとの違いを確認する

ミニマップは階層ごとに別テクスチャとして生成されます。  
多階層ダンジョンでは、プレイヤーの高さに応じて表示するテクスチャを切り替えることで、現在いる階層を分かりやすく表示できます。

## B. 実行時にミニマップを表示する

ゲーム中にダンジョンを生成する場合は、テクスチャアセットとして保存せず、生成したミニマップをそのまま Widget に表示できます。

### 1. Widget Blueprint を用意する

ミニマップ用の Widget Blueprint を作成し、親クラスに `UDungeonMiniMapWidget` を指定します。  
Widget 内には、少なくとも次の要素を配置します。

- `OutImage`  
  ミニマップテクスチャを表示する `Image` です。名前を必ず `OutImage` にしてください。
- `OutIconWidget`  
  アイコンを表示したい場合だけ配置します。型は `DungeonIconWidget`、名前は `OutIconWidget` にしてください。

### 2. 表示用マテリアルを設定する

`UDungeonMiniMapWidget` の `OutMaterial` に、ミニマップ表示用のマテリアルを設定します。  
標準では、次のテクスチャパラメータ名が使われます。

- `BaseTexture`  
  ミニマップ本体のテクスチャです。
- `MaskTexture`  
  未探索エリアなどを隠すためのマスクテクスチャです。

マテリアル側のパラメータ名を変更している場合は、Widget 側の `BaseTextureName` と `MaskTextureName` も同じ名前にしてください。

### 3. ミニマップテクスチャの作り方を選ぶ

`UDungeonMiniMapWidget` では、実行時にミニマップテクスチャを生成するか、すでに生成済みの `DungeonMiniMapTextureLayer` を使うかを選べます。

- `Generate Dungeon Mini Map Texture Layer` を有効にする  
  Widget が実行時にミニマップを生成します。最初はこちらが分かりやすいです。
- `Generate Dungeon Mini Map Texture Layer` を無効にする  
  `ADungeonGenerateActor` 側で生成済みのミニマップを使います。複数の Widget で同じミニマップを共有したい場合に向いています。

生成方法は次の設定で選びます。

- `Generate Minimap Texture With Size` を有効にする  
  `TextureWidth` でテクスチャサイズを指定します。
- `Generate Minimap Texture With Size` を無効にする  
  `DotScale` で 1 グリッドあたりのドット数を指定します。デフォルトは `20` です。

### 4. マスクを使うか選ぶ

`Create Mask Texture` を有効にすると、マスクテクスチャも同時に生成されます。  
プレイヤーの周囲だけを表示したい場合は有効にしてください。

`Player Visible Radius` は、プレイヤー周辺でマスクを解除する半径です。単位は Unreal の標準と同じセンチメートルです。

### 5. ミニマップの向きを選ぶ

`Is Map North Up` でミニマップの向きを選びます。

- 有効  
  北が常に上になります。地図として読みやすい表示です。
- 無効  
  プレイヤーまたはカメラの向きに合わせてマップが回転します。レーダーのような表示に向いています。

## C. アイコンを重ねて表示する

ミニマップにプレイヤー、目的地、鍵、扉などのアイコンを表示すると、探索中に状況を把握しやすくなります。

### 1. `OutIconWidget` を配置する

ミニマップ用 Widget Blueprint に `DungeonIconWidget` を配置し、名前を `OutIconWidget` にします。  
`OutIconWidget` は任意ですが、アイコンを表示する場合は必要です。

### 2. Brush にアイコン画像を設定する

`DungeonIconWidget` の `Brush` 配列に、表示したいアイコン画像を設定します。  
`IconIndex` は、この `Brush` 配列の番号です。

![](images/MiniMapIcon1.png)

![](images/MiniMapIcon2.png)

### 3. アイコンサイズを 1 グリッドに合わせる

アイコンの横幅をマップ上の 1 グリッドに合わせたい場合は、`Set Icon Brush Size To Grid` を呼びます。

- `IconIndex`  
  サイズを変更したい `Brush` の番号です。
- `GridUnitWidth`  
  何グリッド分の幅にするかを指定します。`1.0` なら 1 グリッド、`0.5` なら半グリッド、`2.0` なら 2 グリッドです。

例えば、`DotScale` が `20` で `GridUnitWidth` が `1.0` の場合、その Brush の `ImageSize` は `20 x 20` になります。  
`UDungeonMapWidget` で全体マップを拡大した場合も、`MapZoom` によってアイコンはマップと一緒に拡大されます。

### 4. アイコンを登録する

アイコンを表示または更新するには、`Icon Register Or Set` を使います。  
同じ Actor をもう一度登録すると、新しい位置に更新されます。

回転付きで表示したい場合は `Icon Register Or Set Rotated` を使います。  
プレイヤーや敵の向きを示すアイコンに向いています。

### 5. アイコンを解除する

不要になったアイコンは `Icon Unregister` で解除します。  
すべてのアイコンを消したい場合は `Icon Unregister All` を使います。

![](images/MiniMapIcon3.png)

## 確認する
- エディタでミニマップテクスチャを生成すると `ProceduralTextures` にアセットが作られる
- 実行時に `OutImage` へミニマップが表示される
- プレイヤーの高さに応じて表示階層が切り替わる
- マスクを使う場合、プレイヤー周辺が表示される
- `OutIconWidget` を使うと、登録したアイコンが期待した位置に表示される
- `Set Icon Brush Size To Grid` を使うと、アイコンの基準サイズがグリッド幅に合う

## よくある失敗
- `Generate texture with size` / `Generate texture with scale` が押せない  
  先にダンジョンを生成してください。ミニマップは直前の生成結果を使います。
- Widget に何も表示されない  
  `OutImage` という名前の `Image` があるか、`OutMaterial` とテクスチャパラメータ名が合っているか確認してください。
- 表示される階層がずれる  
  プレイヤーの高さ、ダンジョンの `Vertical Size`、階層設定を確認してください。
- アイコンが出ない  
  `OutIconWidget` の名前、`Brush` 配列、`IconIndex`、登録する Actor が有効か確認してください。
- アイコンのサイズが合わない  
  `Set Icon Brush Size To Grid` を、ミニマップテクスチャ生成後に呼んでください。`GridUnitWidth` が `1.0` なら 1 グリッド幅です。

## 補足
実際の組み込み例は、サンプルプロジェクトの `Content/Widget/WBP_SampleDungeonPlayGame` が参考になります。

## 次に読む
- [ADungeonGenerateActor.ja.md](./ADungeonGenerateActor.ja.md)
- [QuickStart.ja.md](./QuickStart.ja.md)
