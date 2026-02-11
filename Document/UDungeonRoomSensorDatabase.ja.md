# UDungeonRoomSensorDatabase 説明書

UDungeonRoomSensorDatabase は、**部屋への侵入を検知するセンサー（UDungeonRoomSensor 派生クラス）をまとめ、条件に応じて抽選・配置するデータベース**です。トラップの発動やイベントトリガーを部屋単位で切り替えたいときに利用します。

## 主な使い方
- 配置したいセンサー（`DungeonRoomSensorBase` を継承した Blueprint/C++ クラス）を `DungeonRoomSensorClass` に登録します。
- SelectionMethod で「どのセンサーをどの部屋に使うか」を決めます。深さで変える、ランダムで混ぜるなどの方式があります。
- ダンジョン生成が終わると OnEndGeneration が呼ばれ、`SpawnActorInAisle` に登録したアクターを通路に自動スポーンさせることもできます（センサー設置後の演出用）。

## UPROPERTY の意味
- **SelectionMethod (`EDungeonMeshSetSelectionMethod`, EditAnywhere)**  
  センサー選択ルール。スタートからの距離などを基準に、登録済みセンサーのどれを使うかを決めます。特定階層だけ強めのセンサーにする、といった使い分けに便利です。
- **DungeonRoomSensorClass (`TArray<UClass*>`, EditAnywhere, AllowedClasses=`DungeonRoomSensorBase`)**  
  使用可能なセンサークラスの一覧。ここに登録したものの中から SelectionMethod に従って抽選されます。未登録だとセンサーは置かれません。
- **SpawnActorInAisle (`TArray<FSoftObjectPath>`, EditAnywhere, AllowedClasses=`Engine.Blueprint`)**  
  通路にスポーンさせる追加アクターのリスト。生成完了後の演出（センサーの警報灯やフォグなど）を自動で配置したい場合に利用します。空なら何もスポーンしません。

## 編集のヒント
- センサーごとに難易度や挙動が異なる場合、SelectionMethod を「DepthFromStart」にして深い階層ほど強いセンサーを登録すると、探索進行に合わせて緊張感を上げられます。
- SpawnActorInAisle で指定する Blueprint は、配置先の通路サイズに合わせてメッシュやエフェクトのスケールを調整しておくと破綻しにくくなります。

