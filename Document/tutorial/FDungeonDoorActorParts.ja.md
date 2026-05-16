# FDungeonDoorActorParts 説明書

FDungeonDoorActorParts は、**部屋や通路の入口にスポーンさせるドア用アクターを登録するための入れ物**です。位置・向きのオフセット情報（FDungeonPartsTransform）に加えて、ドアとして扱うアクタークラスを指定します。

## 主な使い方
- 既存の Blueprint ドア（`DungeonDoorBase` を継承）を `ActorClass` に設定すると、生成時にそのドアが配置されます。
- メッシュやアニメーションを変えた複数のドアを作成し、メッシュセットやフロアごとに差し替えることで、同じダンジョンでも雰囲気を変えられます。

## UPROPERTY の意味
- **ActorClass (`UClass*`, EditAnywhere/BlueprintReadOnly, AllowedClasses=`DungeonDoorBase`)**  
  生成時にスポーンするドアアクターを指定します。`DungeonDoorBase` を継承した Blueprint や C++ クラスのみ選択可能です。未設定のままだとドアは置かれません。

## 編集のヒント
- 開閉方向や位置合わせはベースクラスの Transform で調整できます。壁厚に合わせて奥行きを微調整すると自然に見えます。
- ドアの機能（鍵付き・破壊可能など）はドアクラス側で作り込んでください。ここでは「どのドアを置くか」だけを選びます。

