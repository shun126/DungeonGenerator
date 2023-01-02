/*!
デバッグに関するヘッダーファイル

他のWindowsマクロと衝突を防ぐため、このファイルをヘッダーからincludeしないで下さい。

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#pragma once
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>

// ログマクロ
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
#include <CoreMinimal.h>
DECLARE_LOG_CATEGORY_EXTERN(DungeonGenerator, Log, All);
#define DUNGEON_GENERATOR_ERROR(Format, ...)		UE_LOG(DungeonGenerator, Error, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		UE_LOG(DungeonGenerator, Warning, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		UE_LOG(DungeonGenerator, Display, Format, ##__VA_ARGS__);
#define DUNGEON_GENERATOR_LOG(Format, ...)			UE_LOG(DungeonGenerator, Log, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		UE_LOG(DungeonGenerator, Verbose, Format, ##__VA_ARGS__);
#else
#if defined(_WINDOWS) && (defined(_DEBUG) || defined(DEBUG))
#define DUNGEON_GENERATOR_ERROR(Format, ...)		dungeon::OutputDebugStringWithArgument(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		dungeon::OutputDebugStringWithArgument(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		dungeon::OutputDebugStringWithArgument(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_LOG(Format, ...)			dungeon::OutputDebugStringWithArgument(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		dungeon::OutputDebugStringWithArgument(Format, ##__VA_ARGS__)
#else
#define DUNGEON_GENERATOR_ERROR(Format, ...)		std::pritf(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		std::pritf(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		std::pritf(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_LOG(Format, ...)			std::pritf(Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		std::pritf(Format, ##__VA_ARGS__)
#endif
#endif
/*
#if !defined(TEXT)
#define TEXT(text)									text
#endif
*/
namespace dungeon
{
	/*!
	VisualStudioの出力ウィンドウに出力します
	ソースファイルからのみincludeされる前提なのでstatic関数で良い
	*/
	extern void OutputDebugStringWithArgument(const char* pszFormat, ...);

	namespace bmp
	{
#pragma pack(1)
		struct BMPFILEHEADER
		{
			char bfType[2];
			uint32_t bfSize;
			uint16_t bfReserved1;
			uint16_t bfReserved2;
			uint32_t bfOffBits;
		};

		struct BMPINFOHEADER
		{
			uint32_t biSize;
			int32_t biWidth;
			int32_t biHeight;
			uint16_t biPlanes;
			uint16_t biBitCount;
			uint32_t biCompression;
			uint32_t biSizeImage;
			int32_t biXPelsPerMeter;
			int32_t biYPelsPerMeter;
			uint32_t biClrUsed;
			uint32_t biClrImportant;
		};

		struct RGBCOLOR
		{
			uint8_t rgbBlue;
			uint8_t rgbGreen;
			uint8_t rgbRed;
		};
#pragma pack()

		/*!
		キャンバスクラス
		*/
		class Canvas
		{
		public:
			// コントラクタ
			Canvas();

			/*!
			コンストラクタ
			*/
			Canvas(const uint32_t width, const uint32_t height);

			// デストラクタ
			virtual ~Canvas() = default;

			/*!
			画像データの生成
			*/
			void Create(const uint32_t width, const uint32_t height);

			/*
			画像データの吐き出し
			*/
			int Write(const std::string& filename);

			/*!
			矩形の描画
			*/
			void Rectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color);

			/*!
			フレームの描画
			*/
			void Frame(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color);

		private:
			BMPFILEHEADER mBmpHeader;
			BMPINFOHEADER mBmpInfo;

			uint32_t mWidth;	// 横幅
			uint32_t mHeight;	// 縦幅

			std::unique_ptr<RGBCOLOR[]> mRgbImage;	// 画像データの本体
		};
	}
}
