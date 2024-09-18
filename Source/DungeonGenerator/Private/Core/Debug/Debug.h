/**
@brief	Debug function header files

To prevent conflicts with other Windows macros,
do not include this file from the header.

@file		Debug.h
@author		Shun Moriya
@copyright	2023- Shun Moriya
All Rights Reserved.
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

// log macro
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
#include <CoreMinimal.h>
DECLARE_LOG_CATEGORY_EXTERN(DungeonGeneratorLogger, Log, All);
#define DUNGEON_GENERATOR_ERROR(Format, ...)		UE_LOG(DungeonGeneratorLogger, Error, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_WARNING(Format, ...)		UE_LOG(DungeonGeneratorLogger, Warning, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_DISPLAY(Format, ...)		UE_LOG(DungeonGeneratorLogger, Display, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_LOG(Format, ...)			UE_LOG(DungeonGeneratorLogger, Log, Format, ##__VA_ARGS__)
#define DUNGEON_GENERATOR_VERBOSE(Format, ...)		UE_LOG(DungeonGeneratorLogger, Verbose, Format, ##__VA_ARGS__)
#elif defined(_WINDOWS) && (defined(_DEBUG) || defined(DEBUG))
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

namespace dungeon
{
	static constexpr auto BaseDirectoryName = TEXT("DungeonGenerator");

	/*!
	Output to VisualStudio output window
	Assumed to be included only from source files, so static functions are fine.
	*/
	extern void OutputDebugStringWithArgument(const char* pszFormat, ...);

	extern const FString& GetBaseDirectoryName();

	extern const FString& GetDebugDirectory();

	extern const std::string& GetDebugDirectoryString();

	extern void CreateDebugDirectory();

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

		//! Microsoft Windows Bitmap infomation header
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
		@brief Windows Bitmap canvas class
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
	static constexpr bmp::RGBCOLOR BaseDarkColor = { 95, 84, 62 };
	static constexpr bmp::RGBCOLOR BaseLightColor = { 173, 153, 112 };
	static constexpr bmp::RGBCOLOR StartColor = { 0, 128, 0 };
	static constexpr bmp::RGBCOLOR GoalColor = { 0, 0, 128 };
	static constexpr bmp::RGBCOLOR LeafColor = { 0, 128, 128 };
	static constexpr bmp::RGBCOLOR AisleColor = LeafColor;
	static constexpr uint8_t LightGridValue = 96;
	static constexpr uint8_t DarkGridValue = 48;
	static constexpr bmp::RGBCOLOR LightGridColor = { LightGridValue, LightGridValue, LightGridValue };
	static constexpr bmp::RGBCOLOR DarkGridColor = { DarkGridValue, DarkGridValue, DarkGridValue };
	static constexpr bmp::RGBCOLOR OriginXColor = { 0, 0, 255 };
	static constexpr bmp::RGBCOLOR OriginYColor = { 0, 255, 0 };
	static constexpr bmp::RGBCOLOR OriginZColor = { 255, 0, 0 };

	inline uint32_t Scale(const uint32_t value)
	{
		return static_cast<uint32_t>(value * ImageScale);
	}

}
