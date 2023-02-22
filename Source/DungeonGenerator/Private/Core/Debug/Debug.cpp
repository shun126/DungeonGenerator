/**
デバッグに関するソースファイル

\author		Shun Moriya
\copyright	2023 Shun Moriya
*/

#include "Debug.h"

// ログマクロ
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING > 0
DEFINE_LOG_CATEGORY(DungeonGenerator);
#else
#define NOMINMAX
#include <windows.h>
#endif

namespace dungeon
{
#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING == 0
#if defined(_WINDOWS) && (defined(_DEBUG) || defined(DEBUG))
	/**
	VisualStudioの出力ウィンドウに出力します
	ソースファイルからのみincludeされる前提なのでstatic関数で良い
	*/
	extern void OutputDebugStringWithArgument(const char* pszFormat, ...)
	{
		va_list	argp;
		char pszBuf[512];
		va_start(argp, pszFormat);
#if defined(_WINDOWS)
		vsprintf_s(pszBuf, pszFormat, argp);
#else
		vsprintf(pszBuf, pszFormat, argp);
#endif
		va_end(argp);
		::OutputDebugStringA(pszBuf);
	}
#endif
#endif

	/**
	BMPファイル生成
	*/
	namespace bmp
	{
		Canvas::Canvas() noexcept
			: mWidth(0)
			, mHeight(0)
		{
			memset(&mBmpHeader, 0, sizeof(BMPFILEHEADER));
			memset(&mBmpInfo, 0, sizeof(BMPINFOHEADER));
		};

		Canvas::Canvas(const uint32_t width, const uint32_t height) noexcept
		{
			Create(width, height);
		}

		void Canvas::Create(const uint32_t width, const uint32_t height) noexcept
		{
			mRgbImage = std::make_unique<RGBCOLOR[]>(width * height);
			memset(mRgbImage.get(), 0, sizeof(RGBCOLOR) * width * height);
			mHeight = height;
			mWidth = width;

			const size_t linedsize = (static_cast<size_t>(mWidth) + 3) / 4 * 4;

			// file header
			mBmpHeader.bfType[0] = 'B';
			mBmpHeader.bfType[1] = 'M';
			mBmpHeader.bfReserved1 = 0;
			mBmpHeader.bfReserved2 = 0;
			mBmpHeader.bfOffBits = sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);
			mBmpHeader.bfReserved1 = 0;

			// info header
			mBmpInfo.biSize = sizeof(BMPINFOHEADER);
			mBmpInfo.biWidth = mWidth;
			mBmpInfo.biHeight = mHeight;
			mBmpInfo.biPlanes = 1;
			mBmpInfo.biBitCount = 24;
			mBmpInfo.biCompression = 0;
			mBmpInfo.biSizeImage = linedsize * mHeight * 3;
			mBmpInfo.biXPelsPerMeter = 1;
			mBmpInfo.biYPelsPerMeter = 1;
			mBmpInfo.biClrUsed = 0;
			mBmpInfo.biClrImportant = 0;
		}

		int Canvas::Write(const std::string& filename) noexcept
		{
#if defined(_WINDOWS) || defined(_WIN32)
			FILE* fp;
			if (fopen_s(&fp, filename.c_str(), "wb") != 0)
#else
			FILE* fp = fopen(filename.c_str(), "wb");
			if (fp == NULL)
#endif
			{
				perror("Can't open the file.");
				return 0;
			}

			// ヘッダ部分を生成、書き込み
			fwrite(&mBmpHeader, sizeof(BMPFILEHEADER), 1, fp);
			fwrite(&mBmpInfo, sizeof(BMPINFOHEADER), 1, fp);

			// データ部分を一行ずつ書き込み
			for (int64_t i = static_cast<int64_t>(mHeight) - 1; i >= 0; --i)
			{
				fwrite(&mRgbImage.get()[i * mWidth], sizeof(RGBCOLOR), mWidth, fp);

				static const char buf[4] = { 0 };
				fwrite(buf, sizeof(char), mWidth % 4, fp);
			}

			fclose(fp);
			return 1;
		}

		void Canvas::Put(int32_t x, int32_t y, const RGBCOLOR color) noexcept
		{
			x = std::max(0, std::min(x, static_cast<int32_t>(mWidth - 1)));
			y = std::max(0, std::min(y, static_cast<int32_t>(mHeight - 1)));
			mRgbImage.get()[y * mWidth + x] = color;
		}

		void Canvas::Rectangle(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color) noexcept
		{
			left = std::max(0, std::min(left, static_cast<int32_t>(mWidth - 1)));
			top = std::max(0, std::min(top, static_cast<int32_t>(mHeight - 1)));
			right = std::max(0, std::min(right, static_cast<int32_t>(mWidth - 1)));
			bottom = std::max(0, std::min(bottom, static_cast<int32_t>(mHeight - 1)));
			if (left > right)
				std::swap(left, right);
			if (top > bottom)
				std::swap(top, bottom);

			for (int32_t y = top; y <= bottom; ++y)
			{
				for (int32_t x = left; x <= right; ++x)
				{
					mRgbImage.get()[y * mWidth + x] = color;
				}
			}
		}

		void Canvas::Frame(int32_t left, int32_t top, int32_t right, int32_t bottom, const RGBCOLOR color) noexcept
		{
			left = std::max(0, std::min(left, static_cast<int32_t>(mWidth - 1)));
			top = std::max(0, std::min(top, static_cast<int32_t>(mHeight - 1)));
			right = std::max(0, std::min(right, static_cast<int32_t>(mWidth - 1)));
			bottom = std::max(0, std::min(bottom, static_cast<int32_t>(mHeight - 1)));
			if (left > right)
				std::swap(left, right);
			if (top > bottom)
				std::swap(top, bottom);

			// top
			for (int32_t x = left; x <= right; ++x)
			{
				mRgbImage.get()[top * mWidth + x] = color;
			}

			// bottom
			for (int32_t x = left; x <= right; ++x)
			{
				mRgbImage.get()[bottom * mWidth + x] = color;
			}

			// left
			for (int32_t y = top; y <= bottom; ++y)
			{
				mRgbImage.get()[y * mWidth + left];
			}

			// right
			for (int32_t y = top; y <= bottom; ++y)
			{
				mRgbImage.get()[y * mWidth + right] = color;
			}
		}
	}
}
