# UDungeonSubLevelDatabase 説明書

UDungeonSubLevelDatabase は、**特定の部屋を別レベルに差し替える「サブレベル」を管理し、スタート・ゴール・ランダム枠に配置するためのデータベース**です。大部屋やボス部屋などを事前に作り込んだレベルアセットで置き換えたいときに使います。

## 主な使い方
- 事前に用意したサブレベル（LevelPath を持つ FDungeonRoomRegister）を登録しておき、スタートやゴール用、優先配置用、ランダム抽選用に振り分けます。
- GridSize/VerticalGridSize は参照用の表示フィールドです。実際の値はサブレベル内の `DungeonSubLevelScriptActor` で設定し、Build を実行して同期させます。
- ランダム枠の抽選回数を `DungeonRoomLocatorMaxDrawCount` で制御し、出現率を調整します。

## UPROPERTY の意味
- **GridSize / VerticalGridSize (`float`, VisibleAnywhere/BlueprintReadOnly)**  
  サブレベルが想定するグリッド寸法の表示欄です。SubLevelScriptActor の `GridSize` と合わせることで、メッシュやコリジョンが本体のダンジョンとズレないようにします。
- **StartRoom (`FDungeonRoomRegister`, EditAnywhere)**  
  スタート部屋として強制配置するサブレベル。LevelPath が空なら自動生成の部屋が使われます。チュートリアルや安全地帯を決めたいときに指定します。
- **GoalRoom (`FDungeonRoomRegister`, EditAnywhere)**  
  ゴール部屋として強制配置するサブレベル。出口やボス部屋を明示したい場合に設定します。こちらも LevelPath 未設定なら自動生成にフォールバックします。
- **DungeonRoomRegister (`TArray<FDungeonRoomRegister>`, EditAnywhere, DisplayName="Preferred Sublevel")**  
  置きたいサブレベルの優先リスト。登録した順や抽選ルールに従って配置されます。必ず置きたい特別部屋を並べるのに適しています。
- **DungeonRoomLocator (`TArray<FDungeonRoomLocator>`, EditAnywhere, DisplayName="Random Sublevel")**  
  ランダムに抽選して配置するサブレベル群。各要素には識別子や重み付けを設定できます。多すぎる場合は `DungeonRoomLocatorMaxDrawCount` で抽選回数を制限してください。
- **DungeonRoomLocatorMaxDrawCount (`int32`, EditAnywhere, ClampMin=0, DisplayName="Random Sublevel Draw Count")**  
  ランダム枠から抽選する最大回数。0 ならすべてのランダム候補を抽選します。値を絞れば「たまに出る特別部屋」を演出しやすくなります。

## 編集のヒント
- サブレベルのサイズや入口位置が本体のグリッドに合っているか、Build 後にプレビューで確認してください。
- Start/Goal を空にすると自動生成の部屋に置き換わるため、まずは空のままテストしてから必要なサブレベルを順次差し込むとデバッグしやすいです。

