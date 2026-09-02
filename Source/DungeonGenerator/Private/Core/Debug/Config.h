/**
 * @author      Shun Moriya
 * @copyright   2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "BuildInformation.h"

// エディタかつ開発ビルドのみ有効
#if WITH_EDITOR & JENKINS_FOR_DEVELOP

// 定義するとデバッグに便利なログを出力します
//#define DEBUG_ENABLE_SHOW_DEVELOP_LOG

// 定義すると通信のためのデバッグ情報を出力します
//#define DEBUG_ENABLE_INFORMATION_FOR_REPLICATION

// 定義すると生成時間を計測します
#define DEBUG_ENABLE_MEASURE_GENERATION_TIME

// 定義すると途中経過と最終結果をBMPで出力します
#define DEBUG_GENERATE_BITMAP_FILE

/*
 * 定義すると生成に成功したダンジョンの最終結果(BMPと構造図)を成果物ディレクトリへ残します。
 * 成果物は生成の経歴なので生成の度に削除されず、ディスク容量を圧迫します。
 * 経歴を確認したい時だけ定義してください。
 */
#define DEBUG_GENERATE_ARTIFACT_FILE

// 成果物の出力は途中経過のBMP出力と同じ描画処理を利用します
#if defined(DEBUG_GENERATE_ARTIFACT_FILE) && !defined(DEBUG_GENERATE_BITMAP_FILE)
#error DEBUG_GENERATE_ARTIFACT_FILE requires DEBUG_GENERATE_BITMAP_FILE
#endif

// 定義するとミッショングラフのデバッグファイル(Markdown)を出力します
#define DEBUG_GENERATE_MISSION_GRAPH_FILE

#endif

// Unreal Engineなら定義されます
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
#define BUILD_TARGET_UNREAL_ENGINE
#endif
