/**
@author		Shun Moriya
@copyright	2024- Shun Moriya
All Rights Reserved.
*/

#pragma once
#include "BuildInfomation.h"

#if WITH_EDITOR & JENKINS_FOR_DEVELOP

// 定義するとデバッグに便利なログを出力します
//#define DEBUG_ENABLE_SHOW_DEVELOP_LOG

// 定義すると通信のためのデバッグ情報を出力します
//#define DEBUG_ENABLE_INFOMATION_FOR_REPRECATION

// 定義すると生成時間を計測します
#define DEBUG_ENABLE_MEASURE_GENERATION_TIME

// 定義すると途中経過と最終結果をBMPで出力します
#define DEBUG_GENERATE_BITMAP_FILE

// 定義するとミッショングラフのデバッグファイル(PlantUML)を出力します
#define DEBUG_GENERATE_MISSION_GRAPH_FILE

#endif
