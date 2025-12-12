/**
 * @author		Shun Moriya
 * @copyright	2024- Shun Moriya
 * All Rights Reserved.
 */

#pragma once
#include "NonCopyable.h"
#include <functional>

namespace dungeon
{
	/**
	 * スコープ終了時に自動的にクリーンアップ処理を実行するクラス
	 * @tparam Callable  呼び出し可能オブジェクト（関数ポインタ、ラムダ、関数オブジェクト等）
	 *
	 * このクラスは RAII の原則に従い、以下の特性を持ちます。
	 *  - コンストラクタで渡された function が保持され、デストラクタで 1 回だけ呼び出される。
	 *  - コピーは 禁止（二重実行を防止）。ムーブは許可し、所有権を安全に移譲できる。
	 *  - 例外が送出されてもデストラクタは確実に呼び出される（例外安全）。
	 *
	 * @note  C++ の標準ライブラリにも `std::unique_ptr` のデリータや
	 *        `std::lock_guard` のような RAII 用ユーティリティが多数ありますが、
	 *        任意の任務をシンプルにラップしたいときに `Finalizer` が便利です。
	 */
	class Finalizer final : NonCopyable
	{
	public:
		/**
		 * コンストラクタ
		 */
		explicit Finalizer(const std::function<void()>& function)
			: mFunction(function)
		{
		}

		/**
		 * デストラクタ
		 */
		~Finalizer()
		{
			mFunction();
		}

	private:
		std::function<void()> mFunction;
	};
}
