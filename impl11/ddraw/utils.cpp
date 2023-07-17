// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt
// Extended for VR by Leo Reyes (c) 2019

#include "common.h"
#include "utils.h"

#include <sstream>
#include <iomanip>
#include <memory>
#include <gdiplus.h>
#include <shellapi.h>

// SteamVR
#include <openvr.h>

#pragma comment(lib, "Gdiplus")

using namespace Gdiplus;

void DisplayTimedMessage(uint32_t seconds, int row, char* msg);

std::string int_to_hex(int i)
{
	std::stringstream stream;
	stream << std::setfill('0') << std::setw(8) << std::hex << i;
	return stream.str();
}

std::string wchar_tostring(LPCWSTR text)
{
	std::wstring wstr(text);
	return std::string(wstr.begin(), wstr.end());
}

std::wstring string_towstring(const std::string& text)
{
	std::wstringstream path;
	path << text.c_str();
	return path.str();
}

std::wstring string_towstring(const char* text)
{
	std::wstringstream path;
	path << text;
	return path.str();
}

void toupper(char* string)
{
	int i = 0;
	while (string[i]) {
		string[i] = (char)toupper(string[i]);
		i++;
	}
}

#if LOGGER

std::string tostr_IID(REFIID iid)
{
	LPOLESTR lpsz;

	if (FAILED(StringFromIID(iid, &lpsz)))
	{
		return std::string();
	}

	std::wstring wstr(lpsz);
	std::string str(wstr.begin(), wstr.end());

	CoTaskMemFree(lpsz);

	return str;
}

std::string tostr_DDSURFACEDESC(LPDDSURFACEDESC lpDDSurfaceDesc)
{
	if (lpDDSurfaceDesc == nullptr)
	{
		return " NULL";
	}

	std::stringstream str;

	DWORD dwFlags = lpDDSurfaceDesc->dwFlags;

	if (dwFlags & DDSD_CAPS)
	{
		DWORD dwCaps = lpDDSurfaceDesc->ddsCaps.dwCaps;

		if (dwCaps & DDSCAPS_ALPHA)
		{
			str << " ALPHA";
		}

		if (dwCaps & DDSCAPS_BACKBUFFER)
		{
			str << " BACKBUFFER";
		}

		if (dwCaps & DDSCAPS_COMPLEX)
		{
			str << " COMPLEX";
		}

		if (dwCaps & DDSCAPS_FLIP)
		{
			str << " FLIP";
		}

		if (dwCaps & DDSCAPS_FRONTBUFFER)
		{
			str << " FRONTBUFFER";
		}

		if (dwCaps & DDSCAPS_OFFSCREENPLAIN)
		{
			str << " OFFSCREENPLAIN";
		}

		if (dwCaps & DDSCAPS_OVERLAY)
		{
			str << " OVERLAY";
		}

		if (dwCaps & DDSCAPS_PALETTE)
		{
			str << " PALETTE";
		}

		if (dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			str << " PRIMARYSURFACE";
		}

		if (dwCaps & DDSCAPS_SYSTEMMEMORY)
		{
			str << " SYSTEMMEMORY";
		}

		if (dwCaps & DDSCAPS_TEXTURE)
		{
			str << " TEXTURE";
		}

		if (dwCaps & DDSCAPS_3DDEVICE)
		{
			str << " 3DDEVICE";
		}

		if (dwCaps & DDSCAPS_VIDEOMEMORY)
		{
			str << " VIDEOMEMORY";
		}

		if (dwCaps & DDSCAPS_VISIBLE)
		{
			str << " VISIBLE";
		}

		if (dwCaps & DDSCAPS_WRITEONLY)
		{
			str << " WRITEONLY";
		}

		if (dwCaps & DDSCAPS_ZBUFFER)
		{
			str << " ZBUFFER";
		}

		if (dwCaps & DDSCAPS_OWNDC)
		{
			str << " OWNDC";
		}

		if (dwCaps & DDSCAPS_LIVEVIDEO)
		{
			str << " LIVEVIDEO";
		}

		if (dwCaps & DDSCAPS_HWCODEC)
		{
			str << " HWCODEC";
		}

		if (dwCaps & DDSCAPS_MODEX)
		{
			str << " MODEX";
		}

		if (dwCaps & DDSCAPS_MIPMAP)
		{
			str << " MIPMAP";
		}

		if (dwCaps & DDSCAPS_ALLOCONLOAD)
		{
			str << " ALLOCONLOAD";
		}

		if (dwCaps & DDSCAPS_VIDEOPORT)
		{
			str << " VIDEOPORT";
		}

		if (dwCaps & DDSCAPS_LOCALVIDMEM)
		{
			str << " LOCALVIDMEM";
		}

		if (dwCaps & DDSCAPS_NONLOCALVIDMEM)
		{
			str << " NONLOCALVIDMEM";
		}

		if (dwCaps & DDSCAPS_STANDARDVGAMODE)
		{
			str << " STANDARDVGAMODE";
		}

		if (dwCaps & DDSCAPS_OPTIMIZED)
		{
			str << " OPTIMIZED";
		}
	}

	if (dwFlags & (DDSD_HEIGHT | DDSD_WIDTH))
	{
		str << " " << lpDDSurfaceDesc->dwWidth << "x" << lpDDSurfaceDesc->dwHeight;
	}

	if (dwFlags & DDSD_PITCH)
	{

	}

	if (dwFlags & DDSD_BACKBUFFERCOUNT)
	{
		str << " " << lpDDSurfaceDesc->dwBackBufferCount << " back buffers";
	}

	if (dwFlags & DDSD_ZBUFFERBITDEPTH)
	{

	}

	if (dwFlags & DDSD_ALPHABITDEPTH)
	{

	}

	if (dwFlags & DDSD_LPSURFACE)
	{

	}

	if (dwFlags & DDSD_PIXELFORMAT)
	{
		DDPIXELFORMAT& ddpf = lpDDSurfaceDesc->ddpfPixelFormat;

		if (ddpf.dwFlags & DDPF_RGB)
		{
			str << " " << ddpf.dwRGBBitCount << " bpp";
		}
	}

	if (dwFlags & DDSD_CKDESTOVERLAY)
	{

	}

	if (dwFlags & DDSD_CKDESTBLT)
	{

	}

	if (dwFlags & DDSD_CKSRCOVERLAY)
	{

	}

	if (dwFlags & DDSD_CKSRCBLT)
	{

	}

	if (dwFlags & DDSD_MIPMAPCOUNT)
	{
		str << " " << lpDDSurfaceDesc->dwMipMapCount << " mipmaps";
	}

	if (dwFlags & DDSD_REFRESHRATE)
	{
		if (lpDDSurfaceDesc->dwRefreshRate == 0)
		{
			str << " default Hz";
		}
		else
		{
			str << " " << lpDDSurfaceDesc->dwRefreshRate << " Hz";
		}
	}

	if (dwFlags & DDSD_LINEARSIZE)
	{

	}

	return str.str();
}

std::string tostr_RECT(LPRECT lpRect)
{
	if (lpRect == nullptr)
	{
		return " NULL";
	}

	std::stringstream str;

	str << " " << lpRect->left;
	str << " " << lpRect->top;
	str << " " << lpRect->right;
	str << " " << lpRect->bottom;

	return str.str();
}

#endif

void copySurface(char* dest, DWORD destWidth, DWORD destHeight, DWORD destBpp, char* src, DWORD srcWidth, DWORD srcHeight, DWORD srcBpp, DWORD dwX, DWORD dwY, LPRECT lpSrcRect, bool useColorKey)
{
	RECT rc;

	if (lpSrcRect != nullptr)
	{
		rc = *lpSrcRect;
	}
	else
	{
		rc = { 0, 0, static_cast<LONG>(srcWidth), static_cast<LONG>(srcHeight) };
	}

	int h = rc.bottom - rc.top;
	int w = rc.right - rc.left;

	if (destBpp == 2)
	{
		if (srcBpp == 2)
		{
			unsigned short* srcBuffer = (unsigned short*)src + rc.top * srcWidth + rc.left;
			unsigned short* destBuffer = (unsigned short*)dest + dwY * destWidth + dwX;

			if (useColorKey)
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned short color16 = srcBuffer[y * srcWidth + x];

						if (color16 == 0x2000)
							continue;

						destBuffer[y * destWidth + x] = color16;
					}
				}
			}
			else
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned short color16 = srcBuffer[y * srcWidth + x];

						destBuffer[y * destWidth + x] = color16;
					}
				}
			}
		}
		else
		{
			unsigned int* srcBuffer = (unsigned int*)src + rc.top * srcWidth + rc.left;
			unsigned short* destBuffer = (unsigned short*)dest + dwY * destWidth + dwX;

			if (useColorKey)
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned int color32 = srcBuffer[y * srcWidth + x];

						if (color32 == 0x200000)
							continue;

						destBuffer[y * destWidth + x] = convertColorB8G8R8X8toB5G6R5(color32);
					}
				}
			}
			else
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned int color32 = srcBuffer[y * srcWidth + x];

						destBuffer[y * destWidth + x] = convertColorB8G8R8X8toB5G6R5(color32);
					}
				}
			}
		}
	}
	else
	{
		if (srcBpp == 2)
		{
			unsigned short* srcBuffer = (unsigned short*)src + rc.top * srcWidth + rc.left;
			unsigned int* destBuffer = (unsigned int*)dest + dwY * destWidth + dwX;

			if (useColorKey)
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned short color16 = srcBuffer[y * srcWidth + x];

						if (color16 == 0x2000)
							continue;

						destBuffer[y * destWidth + x] = convertColorB5G6R5toB8G8R8X8(color16);
					}
				}
			}
			else
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned short color16 = srcBuffer[y * srcWidth + x];

						destBuffer[y * destWidth + x] = convertColorB5G6R5toB8G8R8X8(color16);
					}
				}
			}
		}
		else
		{
			unsigned int* srcBuffer = (unsigned int*)src + rc.top * srcWidth + rc.left;
			unsigned int* destBuffer = (unsigned int*)dest + dwY * destWidth + dwX;

			if (useColorKey)
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned int color32 = srcBuffer[y * srcWidth + x];

						if (color32 == 0x200000)
							continue;

						destBuffer[y * destWidth + x] = color32 & 0xffffff;
					}
				}
			}
			else
			{
				for (int y = 0; y < h; ++y)
				{
					for (int x = 0; x < w; ++x)
					{
						unsigned int color32 = srcBuffer[y * srcWidth + x];

						destBuffer[y * destWidth + x] = color32 & 0xffffff;
					}
				}
			}
		}
	}
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;
	UINT size = 0;

	ImageCodecInfo* pImageCodecInfo = nullptr;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == nullptr)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

class GdiInitializer
{
public:
	GdiInitializer()
	{
		this->status = GdiplusStartup(&token, &gdiplusStartupInput, nullptr);

		GetEncoderClsid(L"image/png", &this->pngClsid);
		GetEncoderClsid(L"image/jpeg", &this->jpgClsid);
	}

	~GdiInitializer()
	{
		if (this->status == 0)
		{
			GdiplusShutdown(token);
		}
	}

	bool hasError()
	{
		return this->status != 0;
	}

	ULONG_PTR token;
	GdiplusStartupInput gdiplusStartupInput;

	Status status;
	CLSID pngClsid;
	CLSID jpgClsid;
};

static GdiInitializer g_gdiInitializer;

void scaleSurface(char* dest, DWORD destWidth, DWORD destHeight, DWORD destBpp, char* src, DWORD srcWidth, DWORD srcHeight, DWORD srcBpp)
{
	if (g_gdiInitializer.hasError())
		return;

	std::unique_ptr<Bitmap> bitmap(new Bitmap(destWidth, destHeight, destBpp == 2 ? PixelFormat16bppRGB565 : PixelFormat32bppRGB));
	std::unique_ptr<Bitmap> bitmapSrc(new Bitmap(srcWidth, srcHeight, srcWidth * srcBpp, srcBpp == 2 ? PixelFormat16bppRGB565 : PixelFormat32bppRGB, (BYTE*)src));

	{
		std::unique_ptr<Graphics> graphics(new Graphics(bitmap.get()));
		graphics->SetInterpolationMode(InterpolationModeNearestNeighbor);

		Rect rc(0, 0, destWidth, destHeight);

		Rect srcRc;

		if (g_config.AspectRatioPreserved)
		{
			if (srcHeight * destWidth <= srcWidth * destHeight)
			{
				srcRc.Width = srcHeight * destWidth / destHeight;
				srcRc.Height = srcHeight;
			}
			else
			{
				srcRc.Width = srcWidth;
				srcRc.Height = srcWidth * destHeight / destWidth;
			}
		}
		else
		{
			srcRc.Width = srcWidth;
			srcRc.Height = srcHeight;
		}

		srcRc.X = (srcWidth - srcRc.Width) / 2;
		srcRc.Y = (srcHeight - srcRc.Height) / 2;

		if (graphics->DrawImage(bitmapSrc.get(), rc, srcRc.X, srcRc.Y, srcRc.Width, srcRc.Height, UnitPixel) == 0)
		{
			BitmapData data;

			if (bitmap->LockBits(&rc, ImageLockModeRead, bitmap->GetPixelFormat(), &data) == 0)
			{
				int rowLength = destWidth * destBpp;

				if (rowLength == data.Stride)
				{
					memcpy(dest, data.Scan0, destHeight * rowLength);
				}
				else
				{
					char* srcBuffer = (char*)data.Scan0;
					char* destBuffer = dest;

					for (DWORD y = 0; y < destHeight; y++)
					{
						memcpy(destBuffer, srcBuffer, rowLength);

						srcBuffer += data.Stride;
						destBuffer += rowLength;
					}
				}

				bitmap->UnlockBits(&data);
			}
		}
	}
}

ColorConverterTables::ColorConverterTables()
{
	// X8toX5, X8toX6
	for (unsigned int c = 0; c < 0x100; c++)
	{
		this->X8toX5[c] = (c * (0x1F * 2) + 0xFF) / (0xFF * 2);
		this->X8toX6[c] = (c * (0x3F * 2) + 0xFF) / (0xFF * 2);
	}

	// B5G6R5toB8G8R8X8
	for (unsigned int color16 = 0; color16 < 0x10000; color16++)
	{
		unsigned int red = (color16 & 0xF800) >> 11;
		unsigned int green = (color16 & 0x7E0) >> 5;
		unsigned int blue = color16 & 0x1F;

		red = (red * (0xFF * 2) + 0x1F) / (0x1F * 2);
		green = (green * (0xFF * 2) + 0x3F) / (0x3F * 2);
		blue = (blue * (0xFF * 2) + 0x1F) / (0x1F * 2);

		unsigned int color32 = (red << 16) | (green << 8) | blue;

		this->B5G6R5toB8G8R8X8[color16] = color32;
	}

	// B4G4R4A4toB8G8R8A8
	for (unsigned int color16 = 0; color16 < 0x10000; color16++)
	{
		unsigned int red = (color16 & 0xF00) >> 8;
		unsigned int green = (color16 & 0xF0) >> 4;
		unsigned int blue = color16 & 0x0F;
		unsigned int alpha = (color16 & 0xF000) >> 12;

		red = (red * (0xFF * 2) + 0x0F) / (0x0F * 2);
		green = (green * (0xFF * 2) + 0x0F) / (0x0F * 2);
		blue = (blue * (0xFF * 2) + 0x0F) / (0x0F * 2);
		alpha = (alpha * (0xFF * 2) + 0x0F) / (0x0F * 2);

		unsigned int color32 = (alpha << 24) | (red << 16) | (green << 8) | blue;

		this->B4G4R4A4toB8G8R8A8[color16] = color32;
	}

	// B5G5R5A1toB8G8R8A8
	for (unsigned int color16 = 0; color16 < 0x10000; color16++)
	{
		unsigned int red = (color16 & 0x7C00) >> 10;
		unsigned int green = (color16 & 0x3E0) >> 5;
		unsigned int blue = color16 & 0x1F;
		unsigned int alpha = (color16 & 0x8000) ? 0xFF : 0;

		red = (red * (0xFF * 2) + 0x1F) / (0x1F * 2);
		green = (green * (0xFF * 2) + 0x1F) / (0x1F * 2);
		blue = (blue * (0xFF * 2) + 0x1F) / (0x1F * 2);

		unsigned int color32 = (alpha << 24) | (red << 16) | (green << 8) | blue;

		this->B5G5R5A1toB8G8R8A8[color16] = color32;
	}

	// B5G6R5toB8G8R8A8
	for (unsigned int color16 = 0; color16 < 0x10000; color16++)
	{
		unsigned int red = (color16 & 0xF800) >> 11;
		unsigned int green = (color16 & 0x7E0) >> 5;
		unsigned int blue = color16 & 0x1F;
		unsigned int alpha = 0xFF;

		red = (red * (0xFF * 2) + 0x1F) / (0x1F * 2);
		green = (green * (0xFF * 2) + 0x3F) / (0x3F * 2);
		blue = (blue * (0xFF * 2) + 0x1F) / (0x1F * 2);

		unsigned int color32 = (alpha << 24) | (red << 16) | (green << 8) | blue;

		this->B5G6R5toB8G8R8A8[color16] = color32;
	}
}

ColorConverterTables g_colorConverterTables;

void saveScreenshot(const std::wstring& filename, char* buffer, DWORD width, DWORD height, DWORD bpp)
{
	if (g_gdiInitializer.hasError())
		return;

	char* image;

	if (bpp == 2)
	{
		image = new char[width * height * 4];
		copySurface(image, width, height, 4, buffer, width, height, bpp, 0, 0, nullptr, false);
	}
	else
	{
		image = buffer;
	}

	EncoderParameters encoderParameters;
	ULONG quality = 100;
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &quality;

	std::unique_ptr<Bitmap> bitmap(new Bitmap(width, height, width * 4, PixelFormat32bppRGB, (BYTE*)image));

	bitmap->Save(filename.c_str(), &g_gdiInitializer.jpgClsid, &encoderParameters);

	if (bpp == 2)
	{
		delete[] image;
	}
}

void saveSurface(const std::wstring& name, char* buffer, DWORD width, DWORD height, DWORD bpp)
{
	if (g_gdiInitializer.hasError())
		return;

	static int index = 0;

	if (index == 0)
	{
		SHFILEOPSTRUCT file_op = {
			NULL,
			FO_DELETE,
			"_screenshots",
			"",
			FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
			false,
			0,
			"" };

		SHFileOperation(&file_op);

		CreateDirectory("_screenshots", nullptr);
	}

	//std::wstring filename = L"_screenshots\\" + std::to_wstring(index) + L"_" + name + L".png";
	std::wstring filename = L"_screenshots\\" + std::to_wstring(index) + L"_" + name + L"_" + std::to_wstring(width) + L"x" + std::to_wstring(height) + L"x" + std::to_wstring(bpp) + L".png";
	index++;

	char* image;

	if (bpp == 2)
	{
		image = new char[width * height * 4];
		copySurface(image, width, height, 4, buffer, width, height, bpp, 0, 0, nullptr, false);
	}
	else
	{
		image = buffer;
	}

	std::unique_ptr<Bitmap> bitmap(new Bitmap(width, height, width * 4, PixelFormat32bppRGB, (BYTE*)image));

	bitmap->Save(filename.c_str(), &g_gdiInitializer.pngClsid, nullptr);

	if (bpp == 2)
	{
		delete[] image;
	}
}

void log_debug(const char *format, ...)
{
	char buf[512];

	va_list args;
	va_start(args, format);

	vsprintf_s(buf, 512, format, args);
	OutputDebugString(buf);
	OutputDebugString("\n");
	va_end(args);
}

void DisplayTimedMessageV(uint32_t seconds, int row, const char *format, ...)
{
	char buf[128];

	va_list args;
	va_start(args, format);

	vsprintf_s(buf, 128, format, args);
	DisplayTimedMessage(seconds, row, buf);

	va_end(args);
}

void log_file(const char *format, ...)
{
	char buf[256];
	static FILE *file = NULL;
	int error = 0;
	
	if (file == NULL) {
		error = fopen_s(&file, "C:\\Temp\\_debug_data.txt", "wt");
		if (error != 0) {
			log_debug("[DBG] Error: %d when creating _debug_data.txt", error);
			return;
		}
	}

	if (file == NULL)
		return;

	va_list args;
	va_start(args, format);

	vsprintf_s(buf, 256, format, args);
	fprintf(file, buf);
	fflush(file);

	va_end(args);
}

// From: https://stackoverflow.com/questions/27303062/strstr-function-like-that-ignores-upper-or-lower-case
char* stristr(const char* str1, const char* str2)
{
	const char* p1 = str1;
	const char* p2 = str2;
	const char* r = *p2 == 0 ? str1 : 0;

	while (*p1 != 0 && *p2 != 0)
	{
		if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
		{
			if (r == 0)
			{
				r = p1;
			}

			p2++;
		}
		else
		{
			p2 = str2;
			if (r != 0)
			{
				p1 = r + 1;
			}

			if (tolower((unsigned char)*p1) == tolower((unsigned char)*p2))
			{
				r = p1;
				p2++;
			}
			else
			{
				r = 0;
			}
		}

		p1++;
	}

	return *p2 == 0 ? (char*)r : 0;
}

void ShowMatrix3(const Matrix3& mat, char* name)
{
	log_debug("[DBG] -----------------------------------------");
	if (name != NULL)
		log_debug("[DBG] %s", name);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f", mat[0], mat[3], mat[6]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f", mat[1], mat[4], mat[7]);
	log_debug("[DBG] %0.6f, %0.6f, %0.6f", mat[2], mat[5], mat[8]);
	log_debug("[DBG] =========================================");
}

// ----------------------------------------------------------------------------
// Numerical diagonalization of 3x3 matrices
// Copyright (C) 2006  Joachim Kopp
// ----------------------------------------------------------------------------
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
// ----------------------------------------------------------------------------

// Constants
#define M_SQRT3 1.732050f  // sqrt(3)

// Macros
#define SQR(x) ((x) * (x))  // x^2

// ----------------------------------------------------------------------------
int EigenVals(float3x3& A, Vector3& w)
// ----------------------------------------------------------------------------
// Calculates the eigenvalues of a symmetric 3x3 matrix A using Cardano's
// analytical algorithm.
// Only the diagonal and upper triangular parts of A are accessed. The access
// is read-only.
// ----------------------------------------------------------------------------
// Parameters:
//   A: The symmetric input matrix
//   w: Storage buffer for eigenvalues
// ----------------------------------------------------------------------------
// Return value:
//   0: Success
//  -1: Error
// ----------------------------------------------------------------------------
{
	float m, c1, c0;

	// Determine coefficients of characteristic poynomial. We write
	//       | a   d   f |
	//  A =  | d*  b   e |
	//       | f*  e*  c |
	float de = A[0][1] * A[1][2];   // d * e
	float dd = SQR(A[0][1]);        // d^2
	float ee = SQR(A[1][2]);        // e^2
	float ff = SQR(A[0][2]);        // f^2
	m = A[0][0] + A[1][1] + A[2][2];
	c1 = (A[0][0] * A[1][1] + A[0][0] * A[2][2] +
		A[1][1] * A[2][2])  // a*b + a*c + b*c - d^2 - e^2 - f^2
		- (dd + ee + ff);
	c0 = A[2][2] * dd + A[0][0] * ee + A[1][1] * ff - A[0][0] * A[1][1] * A[2][2] -
		2.0f * A[0][2] * de;  // c*d^2 + a*e^2 + b*f^2 - a*b*c - 2*f*d*e)

	float p, sqrt_p, q, c, s, phi;
	p = SQR(m) - 3.0f * c1;
	q = m * (p - (3.0f / 2.0f) * c1) - (27.0f / 2.0f) * c0;
	sqrt_p = sqrt(fabs(p));

	phi = 27.0f * (0.25f * SQR(c1) * (p - c1) + c0 * (q + 27.0f / 4.0f * c0));
	phi = (1.0f / 3.0f) * atan2(sqrt(fabs(phi)), q);

	c = sqrt_p * cos(phi);
	s = (1.0f / M_SQRT3) * sqrt_p * sin(phi);

	w[1] = (1.0f / 3.0f) * (m - c);
	w[2] = w[1] + s;
	w[0] = w[1] + c;
	w[1] -= s;

	return 0;
}

int EigenVals(Matrix3& A, Vector3& w)
{
	float3x3 a(A);
	int result = EigenVals(a, w);
	A.set(a);
	return result;
}

// ----------------------------------------------------------------------------
int EigenVectors(float3x3& A, float3x3& Q, Vector3& w)
// ----------------------------------------------------------------------------
// Calculates the eigenvalues and normalized eigenvectors of a symmetric 3x3
// matrix A using Cardano's method for the eigenvalues and an analytical
// method based on vector cross products for the eigenvectors.
// Only the diagonal and upper triangular parts of A need to contain meaningful
// values. However, all of A may be used as temporary storage and may hence be
// destroyed.
// ----------------------------------------------------------------------------
// Parameters:
//   A: The symmetric input matrix
//   Q: Storage buffer for eigenvectors
//   w: Storage buffer for eigenvalues
// ----------------------------------------------------------------------------
// Return value:
//   0: Success
//  -1: Error
// ----------------------------------------------------------------------------
// Dependencies:
//   dsyevc3()
// ----------------------------------------------------------------------------
// Version history:
//   v1.1 (12 Mar 2012): Removed access to lower triangular part of A
//     (according to the documentation, only the upper triangular part needs
//     to be filled)
//   v1.0: First released version
// ----------------------------------------------------------------------------
{
	float norm;          // Squared norm or inverse norm of current eigenvector
	float n0, n1;        // Norm of first and second columns of A
	float n0tmp, n1tmp;  // "Templates" for the calculation of n0/n1 - saves a few FLOPS
	float thresh;        // Small number used as threshold for floating point comparisons
	float error;         // Estimated maximum roundoff error in some steps
	float wmax;          // The eigenvalue of maximum modulus
	float f, t;          // Intermediate storage
	int i, j;             // Loop counters

	// Calculate eigenvalues
	EigenVals(A, w);

	wmax = fabs(w[0]);
	if ((t = fabs(w[1])) > wmax)
		wmax = t;
	if ((t = fabs(w[2])) > wmax)
		wmax = t;
	thresh = SQR(8.0 * FLT_EPSILON * wmax);

	// Prepare calculation of eigenvectors
	n0tmp = SQR(A[0][1]) + SQR(A[0][2]);
	n1tmp = SQR(A[0][1]) + SQR(A[1][2]);
	Q[0][1] = A[0][1] * A[1][2] - A[0][2] * A[1][1];
	Q[1][1] = A[0][2] * A[0][1] - A[1][2] * A[0][0];
	Q[2][1] = SQR(A[0][1]);

	// Calculate first eigenvector by the formula
	//   v[0] = (A - w[0]).e1 x (A - w[0]).e2
	A[0][0] -= w[0];
	A[1][1] -= w[0];
	Q[0][0] = Q[0][1] + A[0][2] * w[0];
	Q[1][0] = Q[1][1] + A[1][2] * w[0];
	Q[2][0] = A[0][0] * A[1][1] - Q[2][1];
	norm = SQR(Q[0][0]) + SQR(Q[1][0]) + SQR(Q[2][0]);
	n0 = n0tmp + SQR(A[0][0]);
	n1 = n1tmp + SQR(A[1][1]);
	error = n0 * n1;

	if (n0 <= thresh)  // If the first column is zero, then (1,0,0) is an eigenvector
	{
		Q[0][0] = 1.0f;
		Q[1][0] = 0.0f;
		Q[2][0] = 0.0f;
	}
	else if (n1 <= thresh)  // If the second column is zero, then (0,1,0) is an eigenvector
	{
		Q[0][0] = 0.0f;
		Q[1][0] = 1.0f;
		Q[2][0] = 0.0f;
	}
	else if (norm < SQR(64.0 * FLT_EPSILON) * error)
	{                      // If angle between A[0] and A[1] is too small, don't use
		t = SQR(A[0][1]);  // cross product, but calculate v ~ (1, -A0/A1, 0)
		f = -A[0][0] / A[0][1];
		if (SQR(A[1][1]) > t)
		{
			t = SQR(A[1][1]);
			f = -A[0][1] / A[1][1];
		}
		if (SQR(A[1][2]) > t)
			f = -A[0][2] / A[1][2];
		norm = 1.0f / sqrt(1 + SQR(f));
		Q[0][0] = norm;
		Q[1][0] = f * norm;
		Q[2][0] = 0.0f;
	}
	else  // This is the standard branch
	{
		norm = sqrt(1.0 / norm);
		for (j = 0; j < 3; j++) Q[j][0] = Q[j][0] * norm;
	}

	// Prepare calculation of second eigenvector
	t = w[0] - w[1];
	if (fabs(t) > 8.0f * FLT_EPSILON * wmax)
	{
		// For non-degenerate eigenvalue, calculate second eigenvector by the formula
		//   v[1] = (A - w[1]).e1 x (A - w[1]).e2
		A[0][0] += t;
		A[1][1] += t;
		Q[0][1] = Q[0][1] + A[0][2] * w[1];
		Q[1][1] = Q[1][1] + A[1][2] * w[1];
		Q[2][1] = A[0][0] * A[1][1] - Q[2][1];
		norm = SQR(Q[0][1]) + SQR(Q[1][1]) + SQR(Q[2][1]);
		n0 = n0tmp + SQR(A[0][0]);
		n1 = n1tmp + SQR(A[1][1]);
		error = n0 * n1;

		if (n0 <= thresh)  // If the first column is zero, then (1,0,0) is an eigenvector
		{
			Q[0][1] = 1.0f;
			Q[1][1] = 0.0f;
			Q[2][1] = 0.0f;
		}
		else if (n1 <= thresh)  // If the second column is zero, then (0,1,0) is an eigenvector
		{
			Q[0][1] = 0.0f;
			Q[1][1] = 1.0f;
			Q[2][1] = 0.0f;
		}
		else if (norm < SQR(64.0f * FLT_EPSILON) * error)
		{                      // If angle between A[0] and A[1] is too small, don't use
			t = SQR(A[0][1]);  // cross product, but calculate v ~ (1, -A0/A1, 0)
			f = -A[0][0] / A[0][1];
			if (SQR(A[1][1]) > t)
			{
				t = SQR(A[1][1]);
				f = -A[0][1] / A[1][1];
			}
			if (SQR(A[1][2]) > t)
				f = -A[0][2] / A[1][2];
			norm = 1.0 / sqrt(1 + SQR(f));
			Q[0][1] = norm;
			Q[1][1] = f * norm;
			Q[2][1] = 0.0f;
		}
		else
		{
			norm = sqrt(1.0 / norm);
			for (j = 0; j < 3; j++) Q[j][1] = Q[j][1] * norm;
		}
	}
	else
	{
		// For degenerate eigenvalue, calculate second eigenvector according to
		//   v[1] = v[0] x (A - w[1]).e[i]
		//
		// This would really get to complicated if we could not assume all of A to
		// contain meaningful values.
		A[1][0] = A[0][1];
		A[2][0] = A[0][2];
		A[2][1] = A[1][2];
		A[0][0] += w[0];
		A[1][1] += w[0];
		for (i = 0; i < 3; i++)
		{
			A[i][i] -= w[1];
			n0 = SQR(A[0][i]) + SQR(A[1][i]) + SQR(A[2][i]);
			if (n0 > thresh)
			{
				Q[0][1] = Q[1][0] * A[2][i] - Q[2][0] * A[1][i];
				Q[1][1] = Q[2][0] * A[0][i] - Q[0][0] * A[2][i];
				Q[2][1] = Q[0][0] * A[1][i] - Q[1][0] * A[0][i];
				norm = SQR(Q[0][1]) + SQR(Q[1][1]) + SQR(Q[2][1]);
				if (norm > SQR(256.0 * FLT_EPSILON) *
					n0)  // Accept cross product only if the angle between
				{                   // the two vectors was not too small
					norm = sqrt(1.0 / norm);
					for (j = 0; j < 3; j++) Q[j][1] = Q[j][1] * norm;
					break;
				}
			}
		}

		if (i == 3)  // This means that any vector orthogonal to v[0] is an EV.
		{
			for (j = 0; j < 3; j++)
				if (Q[j][0] != 0.0)  // Find nonzero element of v[0] ...
				{                    // ... and swap it with the next one
					norm = 1.0 / sqrt(SQR(Q[j][0]) + SQR(Q[(j + 1) % 3][0]));
					Q[j][1] = Q[(j + 1) % 3][0] * norm;
					Q[(j + 1) % 3][1] = -Q[j][0] * norm;
					Q[(j + 2) % 3][1] = 0.0;
					break;
				}
		}
	}

	// Calculate third eigenvector according to
	//   v[2] = v[0] x v[1]
	Q[0][2] = Q[1][0] * Q[2][1] - Q[2][0] * Q[1][1];
	Q[1][2] = Q[2][0] * Q[0][1] - Q[0][0] * Q[2][1];
	Q[2][2] = Q[0][0] * Q[1][1] - Q[1][0] * Q[0][1];

	return 0;
}

int EigenVectors(Matrix3& A, Matrix3& Q, Vector3& w)
{
	float3x3 a(A);
	float3x3 q(Q);
	int result = EigenVectors(a, q, w);
	A.set(a);
	Q.set(q);
	return result;
}

/*
 * Compute the Eigensystem of symmetric 3x3 matrix A.
 * Each entry in w is an eigenvalue.
 * Each column in V is an eigenvector.
 */
int EigenSys(Matrix3& A, Matrix3& V, Vector3& w)
{
	float3x3 a(A);
	float3x3 v(V);

	int res = EigenVals(a, w);
	res |= EigenVectors(a, v, w);

	A.set(a);
	V.set(v);
	return res;
}
