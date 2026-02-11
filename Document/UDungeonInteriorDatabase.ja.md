# UDungeonInteriorDatabase 説明書

UDungeonInteriorDatabase は、**部屋の内装パーツや植生パーツをタグで整理し、条件に合うものを自動抽選するデータベース**です。部屋のテーマに合わせて家具・装飾・植生を切り替えたいときに利用します。

## 主な使い方
- 部屋やゾーンに付けるタグ（例: `Library`, `Boss`, `Ruins` など）に対応した内装パーツを Parts に登録します。
- 植物や草などの植生パーツは VegetationParts にまとめておき、必要に応じて同じタグで管理します。
- 生成パイプラインで部屋に付与される `interiorTags` に基づき、Select が該当パーツを返します。プレハブ化したい場合は Build を実行しておくと安全です。

## UPROPERTY の意味
- **Parts (`TArray<FDungeonInteriorParts>`, EditAnywhere/BlueprintReadOnly)**  
  内装パーツの一覧。各パーツに「適用するタグ」を仕込んでおくと、Select で一致したものが選ばれます。家具セットやデカールなど、部屋の雰囲気を決める要素を登録してください。
- **VegetationParts (`TArray<FDungeonVegetationParts>`, EditAnywhere/BlueprintReadOnly)**  
  植生パーツの一覧。フォリッジや蔦など、環境系の装飾をまとめます。内装パーツとは別管理なので、非植生系だけを差し替えたい場合にも便利です。

## 編集のヒント
- Tags を活用して「この種類の部屋には必ずこの家具を置く」「ボス部屋では植生を減らす」といった制御ができます。
- パラメータを変更したら Build ボタンを押してデータを再生成してください（エディタ上で実行可能）。

