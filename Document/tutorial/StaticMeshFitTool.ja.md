# メッシュフィットツールを使う

このページでは、`StaticMesh Fit` ツールを使って、手持ちの Static Mesh を Dungeon Generator のグリッド寸法に合わせる方法を説明します。  
自分で作った床、壁、天井、スロープ、柱を `Mesh set database` に登録する前に使うと、サイズ違いや向き違いに気づきやすくなります。

## このページのゴール
- 選択した Static Mesh がグリッド寸法に合うか確認する
- 必要に応じて、補正済みの Static Mesh コピーを生成する
- 生成したメッシュを `Mesh set database` に登録する

## メッシュフィットツールとは
メッシュフィットツールは、Content Browser で選んだ Static Mesh の Bounds を確認し、`DungeonGenerateParameter` の `Grid Size` と `Vertical Grid Size` に合うか判定するツールです。

ツールは元の Static Mesh を直接変更しません。  
`Generate Fitted Meshes` を実行すると、選択した出力先に新しい Static Mesh アセットを作ります。

主に次の確認に使います。

- 床や天井が 1 グリッド分の横幅になっているか
- 壁が 1 グリッド幅、1 グリッド高さになっているか
- スロープが 2 グリッド奥へ進んで 1 グリッド上がる寸法になっているか
- 柱の高さがグリッド高さに合っているか
- 90 度単位で回転すると、より自然にグリッドへ合うか

## 使う前に用意するもの
- 調整したい Static Mesh
- `DungeonGenerateParameter` アセット
- 出力先にする Content Browser 上のフォルダ

`DungeonGenerateParameter` は、ツールがグリッド寸法を読むために必要です。  
まだ作っていない場合は、先に [QuickStart.ja.md](./QuickStart.ja.md) の手順で作成してください。

## 1. Static Mesh を選択する
Content Browser で、確認したい Static Mesh を 1 つ以上選択します。  
複数選択した場合は、同じ設定でまとめてチェックできます。

## 2. Fit Check を開く
選択した Static Mesh を右クリックし、次のメニューを開きます。

`Dungeon Generator > Fit Check`

`StaticMesh Fit Settings` ダイアログが表示されます。

## 3. 設定を確認する
ダイアログでは、次の項目を設定します。

### `DungeonGenerateParameter`
グリッド寸法を読むための `DungeonGenerateParameter` を指定します。  
ここで指定した `Grid Size` と `Vertical Grid Size` が、フィット判定の基準になります。

### `Output Directory`
補正済み Static Mesh を保存する Content Browser 上のフォルダです。  
初期値は `/Game/DungeonFittedMeshes` です。

### `Enable Grid Fit`
オンのときは、ルールに合わせて回転とスケール補正を行います。  
オフにすると、回転やスケール補正を行わず、選択したメッシュのコピーだけを作ります。

### `Fit Mode`
- `Axis-wise`  
  X/Y/Z の各軸を個別に補正します。グリッドに正確に合わせやすい反面、形の比率が変わることがあります。
- `Uniform`  
  1 つのスケール値で全体を補正します。形の比率を保ちやすい反面、軸によってはグリッドから少しずれることがあります。

迷った場合は、まず `Axis-wise` で確認してください。  
見た目の比率を崩したくない装飾メッシュでは `Uniform` も検討します。

### `Tolerance (cm)`
`Pass` と判定するために許容する残り誤差です。  
初期値は `1 cm` です。値を小さくすると判定が厳しくなります。

### `Max Correction (%)`
許容する最大補正率です。  
初期値は `50%` です。この値を超える補正が必要なメッシュは `Fail` になります。

### `Per-Mesh Fit Rules`
メッシュごとに、どの種類のパーツとして判定するかを選びます。

- `Auto`  
  メッシュの Bounds から近いルールを自動で選びます。
- `Floor`  
  床用です。X/Y を `Grid Size` に合わせます。
- `Wall`  
  壁用です。X を `Grid Size`、Z を `Vertical Grid Size` に合わせます。
- `Roof`  
  天井用です。X/Y を `Grid Size` に合わせます。
- `Slope`  
  スロープ用です。X を `Grid Size`、Y を `Grid Size * 2`、Z を `Vertical Grid Size` に合わせます。
- `Pillar`  
  柱用です。Z を `Vertical Grid Size` に合わせます。

`Auto` は水平な板状メッシュを `Floor` として扱いやすいように作られています。  
天井として使うメッシュは、必要に応じて `Roof` を手動で選んでください。

## 4. チェックを実行する
設定が終わったら、`Start Fit Check` を押します。  
結果ダイアログに、メッシュごとの判定が表示されます。
結果が想定と違う場合は `Back` を押します。メッシュを生成せずに設定ダイアログへ戻るので、ルールや許容値を直してもう一度チェックできます。

主な列の意味は次の通りです。

- `Select`  
  生成対象にするかどうかです。
- `Mesh Name`  
  チェックした Static Mesh 名です。
- `Rule`  
  使用されたフィットルールです。`Auto` の場合は `Auto -> Floor` のように解決結果が表示されます。
- `Verdict`  
  `Pass` / `Review` / `Fail` の判定です。
- `Rotation`  
  推奨される Yaw 回転です。0 / 90 / 180 / 270 度から選ばれます。
- `Original Size -> Target Size`  
  元の Bounds サイズと、補正後の目標サイズです。
- `Recommended Scale`  
  生成時に適用されるスケールです。
- `Reason`  
  判定理由です。

## 判定の読み方
### `Pass`
許容誤差内に収まっています。  
初期状態で `Select` がオンになります。

### `Review`
最大補正率の範囲内ですが、`Tolerance (cm)` を超える誤差が残っています。  
結果を見て問題ないと判断できる場合だけ、手動で `Select` をオンにしてください。

### `Fail`
必要な補正率が `Max Correction (%)` を超えています。  
この結果は生成できません。元メッシュのサイズ、向き、または選択したルールを見直してください。

## 5. 補正済みメッシュを生成する
生成したい行の `Select` を確認し、`Generate Fitted Meshes` を押します。  
生成される Static Mesh は、選択した `Output Directory` に保存されます。

生成名は、元の名前をもとに作られます。

- 元の名前が `SM_` で始まる場合  
  例: `SM_Wall` から `SM_Wall_DNG`
- 元の名前が `SM_` で始まらない場合  
  例: `Wall` から `SM_Wall_DNG`

同じ名前が既にある場合は、Unreal Engine が重複しない名前を付けます。

## 6. Mesh Set Database に登録する
生成した Static Mesh を、部屋用または通路用の `Mesh set database` に登録します。

- 床は `Floor Parts`
- 壁は `Wall Parts`
- 天井は `Roof Parts`
- スロープは `Slope Parts`
- 柱は `UDungeonGenerateParameter` の `Pillar Parts` など、柱用の設定

登録後、`Window > DungeonGenerator` で `Verify` と `Generate dungeon` を実行し、床、壁、天井、スロープが自然につながるか確認してください。

## 注意点
- 生成される補正済みメッシュは、正しい原点に合わせられます。床、壁、スロープ、柱は底面中央、天井は天面中央が原点になります。元のメッシュアセットは変更されません。
- 厚み0は、選択したルールで補正対象ではない軸だけ許可されます。補正対象の軸にはサイズが必要です。
- このツールは、見た目や UV を作り直すものではありません。大きなスケール補正を行うと、模様の密度や見た目の比率が変わることがあります。
- 生成後のコリジョンは必ず確認してください。必要に応じて、生成された Static Mesh 側でコリジョンを設定し直します。
- `Review` の結果をそのまま使う場合は、プレビューで床や壁のつなぎ目を確認してください。

## よくある失敗
- `DungeonGenerateParameter` を選んでいないため、`Start Fit Check` が押せない
- `Output Directory` が `/Game` 配下ではないため、出力先として使えない
- 床にしたいメッシュが `Auto` で壁や柱に判定される  
  `Floor` を手動で選んで再チェックしてください。
- 天井にしたいメッシュが `Auto -> Floor` と表示される  
  `Roof` を手動で選んで再チェックしてください。
- `Fail` になったメッシュを生成しようとしている  
  `Fail` は生成対象にできません。元メッシュまたはルールを見直してください。

## 次に読む
- [PrepareMeshParts.ja.md](./PrepareMeshParts.ja.md)
- [UDungeonMeshSetDatabase.ja.md](./UDungeonMeshSetDatabase.ja.md)
- [FDungeonMeshParts.ja.md](./FDungeonMeshParts.ja.md)
