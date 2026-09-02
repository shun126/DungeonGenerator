/**
 * @author      Shun Moriya
 * @copyright   2023- Shun Moriya
 * All Rights Reserved.
 */

/**
 * @file
 * Debug helpers kept out of public headers to avoid conflicts with Windows macros.
 * Windowsマクロとの競合を避けるため、公開ヘッダーから分離しているデバッグ補助機能です。
 */

#pragma once
#include <cstdio>
#include <cstdint>
#include <memory>
#include <string>

#if !defined(UE_BUILD_DEBUG)
#define UE_BUILD_DEBUG 0
#endif
#if !defined(UE_BUILD_DEVELOPMENT)
#define UE_BUILD_DEVELOPMENT 0
#endif
#if !defined(UE_BUILD_TEST)
#define UE_BUILD_TEST 0
#endif
#if !defined(UE_BUILD_SHIPPING)
#define UE_BUILD_SHIPPING 0
#endif

// platform
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
#define DUNGEON_GENERATOR_PLATFORM_UNREAL_ENGINE
#elif defined(_WINDOWS) && (defined(_DEBUG) || defined(DEBUG))
#define DUNGEON_GENERATOR_PLATFORM_WINDOWS
#else
#define DUNGEON_GENERATOR_PLATFORM_OTHER
#endif

// log macro
#if defined(DUNGEON_GENERATOR_PLATFORM_UNREAL_ENGINE)
#include <CoreMinimal.h>
DECLARE_LOG_CATEGORY_EXTERN(DungeonGeneratorLogger, Log, All);
#define DUNGEON_GENERATOR_ERROR(Format, ...)		UE_LOG(DungeonGeneratorLogger, Error, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		UE_LOG(DungeonGeneratorLogger, Warning, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		UE_LOG(DungeonGeneratorLogger, Display, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_LOG(Format, ...)			UE_LOG(DungeonGeneratorLogger, Log, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		UE_LOG(DungeonGeneratorLogger, Verbose, Format, ##__VA_ARGS__)

DECLARE_LOG_CATEGORY_EXTERN(DungeonGeneratorMeasure, Log, Log);
#elif defined(DUNGEON_GENERATOR_PLATFORM_WINDOWS)
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

#define DUNGEON_GENERATOR_MEASURE_SCOPE()			dungeon::FDungeonMeasureScope DUNGEON_GENERATOR_JOIN(dungeonMeasureScope, __COUNTER__)
#define DUNGEON_GENERATOR_JOIN_INNER(Left, Right)	Left##Right
#define DUNGEON_GENERATOR_JOIN(Left, Right)			DUNGEON_GENERATOR_JOIN_INNER(Left, Right)
#if defined(DUNGEON_GENERATOR_PLATFORM_UNREAL_ENGINE)
#define DUNGEON_GENERATOR_MEASURE(Format, ...)		dungeon::OutputMeasure(FString::Printf(Format, ##__VA_ARGS__))
#else
#define DUNGEON_GENERATOR_MEASURE(Format, ...)		dungeon::OutputMeasure(Format, ##__VA_ARGS__)
#endif

namespace dungeon
{
	/**
	 * Increases the thread-local measurement indentation and restores it on destruction.
	 * スレッド固有の計測インデントを増やし、破棄時に元へ戻します。
	 */
	class FDungeonMeasureScope final
	{
	public:
		/**
		 * Increases the current thread's indentation depth without logging a message.
		 * メッセージを出力せず、現在のスレッドのインデント深度を増やします。
		 */
		FDungeonMeasureScope() noexcept;

		/**
		 * Restores the indentation depth active before this scope was created.
		 * このスコープを作成する前のインデント深度へ復元します。
		 */
		~FDungeonMeasureScope() noexcept;

		FDungeonMeasureScope(const FDungeonMeasureScope&) = delete;
		FDungeonMeasureScope& operator=(const FDungeonMeasureScope&) = delete;
		FDungeonMeasureScope(FDungeonMeasureScope&&) = delete;
		FDungeonMeasureScope& operator=(FDungeonMeasureScope&&) = delete;

		/**
		 * Returns the indentation depth of the current thread.
		 * 現在のスレッドのインデント深度を返します。
		 */
		static int32_t GetDepth() noexcept;
	};

	/**
	 * Logs a measurement message without changing the current indentation depth.
	 * 現在のインデント深度を変更せずに計測メッセージを出力します。
	 */
#if defined(DUNGEON_GENERATOR_PLATFORM_UNREAL_ENGINE)
	extern void OutputMeasure(const FString& message);
#else
	extern void OutputMeasure(const char* format, ...);
#endif

	static constexpr auto BaseDirectoryName = TEXT("DungeonGenerator");

	//! 生成したダンジョンの経歴を残すディレクトリ名 / Directory name that keeps the history of generated dungeons
	static constexpr auto ArtifactDirectoryName = TEXT("artifact");

	static constexpr float ThinThickness = 1.f;
	static constexpr float BoldThickness = 5.f;

	/**
	 * Output to VisualStudio output window
	 * Assumed to be included only from source files, so static functions are fine.
	 * OutputDebugStringWithArgument を表します。
	 */
	extern void OutputDebugStringWithArgument(const char* pszFormat, ...);
	extern const FString& GetBaseDirectoryName();
	extern const FString& GetDebugDirectory();
	extern const std::string& GetDebugDirectoryString();
	extern void CreateDebugDirectory();

	/**
	 * Returns the directory that keeps generated artifacts.
	 * Unlike the debug directory, its contents are never deleted.
	 * 成果物を残すディレクトリを返します。
	 * デバッグディレクトリと違い、中身は削除されません。
	 */
	extern const FString& GetArtifactDirectory();

	/**
	 * Creates the artifact directory if it does not exist.
	 * 成果物ディレクトリが無ければ作成します。
	 */
	extern void CreateArtifactDirectory();

	/**
	 * Creates the artifact directory and returns a "<date>_<time>_<randomSeed>" path without an extension.
	 * The time has millisecond resolution so files generated in a row do not overwrite each other,
	 * and the seed tells which dungeon the files describe.
	 * Append the extension at the call site so that files describing the same dungeon share one name.
	 * 成果物ディレクトリを作成し、拡張子を除いた "<日付>_<時間>_<乱数の種>" のパスを返します。
	 * 時間はミリ秒まで含むため続けて生成しても上書きされず、種はどのダンジョンかを示します。
	 * 同じダンジョンを表すファイルの名前を揃えられるよう、拡張子は呼び出し側で付けてください。
	 * @param[in]	randomSeed	ダンジョンの生成に使った乱数の種
	 * @return		拡張子を除いた成果物のパス
	 */
	extern std::string CreateArtifactBasePath(const uint32_t randomSeed);

	//! Microsoft Windows Bitmap Image Implementation
	namespace bmp
	{
#pragma pack(1)
		//! Microsoft Windows Bitmap file header
		struct BMPFILEHEADER
		{
			char bfType[2];
			uint32_t bfSize;
			uint16_t bfReserved1;
			uint16_t bfReserved2;
			uint32_t bfOffBits;
		};

		//! Microsoft Windows Bitmap information header
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

		//! Microsoft Windows Bitmap pixel
		struct RGBCOLOR
		{
			uint8_t rgbBlue;
			uint8_t rgbGreen;
			uint8_t rgbRed;
		};
#pragma pack()

		/**
		 * @brief Windows Bitmap canvas class
		 * vas かどうかを返します。
		 */
		class Canvas final
		{
		public:
			//! Create Canvas class
			Canvas() noexcept;

			//! Create Canvas class
			Canvas(const uint32_t width, const uint32_t height) noexcept;

			//! Delete Canvas class
			~Canvas() = default;

			//! Generate image data
			void Create(const uint32_t width, const uint32_t height) noexcept;

			//! Saves image data to a file
			int Write(const std::string& filename) const noexcept;

			//! Draw point
			void Put(int32_t x, int32_t y, const RGBCOLOR color) const noexcept;

			//! Draw line
			void HorizontalLine(int32_t startX, int32_t endX, int32_t y, const RGBCOLOR color) const noexcept;

			//! Draw line
			void VerticalLine(int32_t x, int32_t startY, int32_t endY, const RGBCOLOR color) const noexcept;

			//! Draw rectangle
			void Rectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color) const noexcept;

			//! Draw frame
			void Frame(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color) const noexcept;

		private:
			BMPFILEHEADER mBmpHeader;
			BMPINFOHEADER mBmpInfo;

			uint32_t mWidth;
			uint32_t mHeight;

			std::unique_ptr<RGBCOLOR[]> mRgbImage;
		};
	}

	static constexpr float ImageScale = 10.0f;
	static constexpr bmp::RGBCOLOR BaseDarkColor = { 45, 34, 12 };
	static constexpr bmp::RGBCOLOR BaseLightColor = { 233, 213, 172 };
	static constexpr bmp::RGBCOLOR StartColor = { 0, 128, 0 };
	static constexpr bmp::RGBCOLOR GoalColor = { 0, 0, 128 };
	static constexpr bmp::RGBCOLOR LeafColor = { 0, 128, 128 };
	static constexpr bmp::RGBCOLOR AisleColor = LeafColor;
	//!< Endpoints of an aisle that could not be generated / 生成できなかった通路の両端
	static constexpr bmp::RGBCOLOR FailedAisleColor = { 255, 0, 255 };
	static constexpr uint8_t LightGridValue = 96;
	static constexpr uint8_t DarkGridValue = 48;
	static constexpr bmp::RGBCOLOR LightGridColor = { LightGridValue, LightGridValue, LightGridValue };
	static constexpr bmp::RGBCOLOR DarkGridColor = { DarkGridValue, DarkGridValue, DarkGridValue };
	static constexpr bmp::RGBCOLOR OriginXColor = { 0, 0, 255 };
	static constexpr bmp::RGBCOLOR OriginYColor = { 0, 255, 0 };
	static constexpr bmp::RGBCOLOR OriginZColor = { 255, 0, 0 };
	static constexpr bmp::RGBCOLOR FloorLevelColor = { 0, 215, 255 };

	inline uint32_t Scale(const uint32_t value)
	{
		return static_cast<uint32_t>(value * ImageScale);
	}

}
