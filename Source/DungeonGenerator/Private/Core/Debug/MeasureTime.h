/**
 * @author		Shun Moriya
 * @copyright	2023 - Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "Config.h"
#include "Debug.h"
#include "../Helper/Stopwatch.h"

#if defined(DEBUG_ENABLE_MEASURE_GENERATION_TIME)
/**
 * @brief 時間計測用の StopWatch オブジェクトを生成し、計測を開始するマクロ。
 *
 * このマクロを呼び出した時点で計測が開始され、生成された StopWatch オブジェクトは
 * MEASURE_TIME_LAP マクロと組み合わせて区間時間を取得するために使用される。
 *
 * @param OBJECT_NAME 生成される dungeon::Stopwatch オブジェクトの変数名。
 */
#define MEASURE_TIME_START(OBJECT_NAME) dungeon::Stopwatch OBJECT_NAME;

/**
 * @brief 前回の LAP からの経過時間を計測し、ログに出力するマクロ。
 *
 * dungeon::Stopwatch::Lap() により区間時間を取得し、
 * 指定したセクション名とともにログへ出力する。
 *
 * @param OBJECT_NAME MEASURE_TIME_START で生成した Stopwatch オブジェクトの変数名。
 * @param SECTION_NAME ログに表示する区間名（const TCHAR*）。
 */
#define MEASURE_TIME_LAP(OBJECT_NAME, SECTION_NAME) DUNGEON_GENERATOR_LOG(TEXT("%s: %lf seconds"), SECTION_NAME, OBJECT_NAME.Lap())

 /**
  * @brief 前回の LAP からの経過時間を計測し、変数に加算するマクロ。
  *
  * @param VARIABLE 加算先変数。
  * @param OBJECT_NAME MEASURE_TIME_START で生成した Stopwatch オブジェクトの変数名。
  */
#define MEASURE_TIME_ADD(OBJECT_NAME, VARIABLE)	VARIABLE += OBJECT_NAME.Lap()

#else
#define MEASURE_TIME_START(OBJECT_NAME) ((void)0)
#define MEASURE_TIME_LAP(OBJECT_NAME, SECTION_NAME) ((void)0)
#define MEASURE_TIME_ADD(OBJECT_NAME, VARIABLE) ((void)0)
#endif
