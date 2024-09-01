/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "BuildInfomation.h"

// エディタかつ開発ビルドのみ有効
#if WITH_EDITOR & JENKINS_FOR_DEVELOP

// 定義するとデバッグに便利なログを出力します
//#define DEBUG_ENABLE_SHOW_DEVELOP_LOG

// 定義すると通信のためのデバッグ情報を出力します
//#define DEBUG_ENABLE_INFORMATION_FOR_REPLICATION

// 定義すると生成失敗しても生成を継続します
#define DEBUG_FORCED_GENERATION_CONTINUES

// 定義すると生成時間を計測します
#define DEBUG_ENABLE_MEASURE_GENERATION_TIME

// 定義すると途中経過と最終結果をBMPで出力します
#define DEBUG_GENERATE_BITMAP_FILE

// 定義するとミッショングラフのデバッグファイル(PlantUML)を出力します
#define DEBUG_GENERATE_MISSION_GRAPH_FILE

#endif

// Unreal Engineなら定義されます
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
#define BUILD_TARGET_UNREAL_ENGINE
#endif
