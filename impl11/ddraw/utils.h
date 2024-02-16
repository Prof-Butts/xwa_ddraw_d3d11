// Copyright (c) 2014 Jérémy Ansel
// Licensed under the MIT license. See LICENSE.txt

#pragma once
#include "common.h"
#include "Vectors.h"
#include "Matrices.h"

void toupper(char *string);

std::string int_to_hex(int i);

std::string wchar_tostring(LPCWSTR text);
std::wstring string_towstring(const std::string& text);
std::wstring string_towstring(const char* text);

short ComputeMsgWidth(char* str, int font_size_index);
short DisplayText(char* str, int font_size_index, short x, short y, uint32_t color);
short DisplayCenteredText(char* str, int font_size_index, short y, uint32_t color);
// Only rows 0..2 are available
void DisplayTimedMessage(uint32_t seconds, int row, char* msg);

#if LOGGER

std::string tostr_IID(REFIID iid);

std::string tostr_DDSURFACEDESC(LPDDSURFACEDESC lpDDSurfaceDesc);

std::string tostr_RECT(LPRECT lpRect);

#endif

void copySurface(char* dest, DWORD destWidth, DWORD destHeight, DWORD destBpp, char* src, DWORD srcWidth, DWORD srcHeight, DWORD srcBpp, DWORD dwX, DWORD dwY, LPRECT lpSrcRect, bool useColorKey);

void scaleSurface(char* dest, DWORD destWidth, DWORD destHeight, DWORD destBpp, char* src, DWORD srcWidth, DWORD srcHeight, DWORD srcBpp);

class ColorConverterTables
{
public:
	ColorConverterTables();

	unsigned char X8toX5[0x100];
	unsigned char X8toX6[0x100];

	unsigned int B5G6R5toB8G8R8X8[0x10000];

	unsigned int B4G4R4A4toB8G8R8A8[0x10000];

	unsigned int B5G5R5A1toB8G8R8A8[0x10000];

	unsigned int B5G6R5toB8G8R8A8[0x10000];
};

extern ColorConverterTables g_colorConverterTables;

inline unsigned short convertColorB8G8R8X8toB5G6R5(unsigned int color32)
{
	unsigned char red = (unsigned char)((color32 & 0xFF0000) >> 16);
	unsigned char green = (unsigned char)((color32 & 0xFF00) >> 8);
	unsigned char blue = (unsigned char)(color32 & 0xFF);

	red = g_colorConverterTables.X8toX5[red];
	green = g_colorConverterTables.X8toX6[green];
	blue = g_colorConverterTables.X8toX5[blue];

	return (red << 11) | (green << 5) | blue;
}

inline unsigned int convertColorB5G6R5toB8G8R8X8(unsigned short color16)
{
	return g_colorConverterTables.B5G6R5toB8G8R8X8[color16];
}

inline unsigned int convertColorB4G4R4A4toB8G8R8A8(unsigned short color16)
{
	return g_colorConverterTables.B4G4R4A4toB8G8R8A8[color16];
}

inline unsigned int convertColorB5G5R5A1toB8G8R8A8(unsigned short color16)
{
	return g_colorConverterTables.B5G5R5A1toB8G8R8A8[color16];
}

inline unsigned int convertColorB5G6R5toB8G8R8A8(unsigned short color16)
{
	return g_colorConverterTables.B5G6R5toB8G8R8A8[color16];
}

void saveScreenshot(const std::wstring& filename, char* buffer, DWORD width, DWORD height, DWORD bpp);
void saveSurface(const std::wstring& name, char* buffer, DWORD width, DWORD height, DWORD bpp);

void log_debug(const char *format, ...);
void log_file(const char *format, ...);
short log_debug_vr(short y, int color, const char* format, ...);
void log_debug_vr(int color, const char* format, ...);
void log_debug_vr(const char* format, ...);
void log_debug_vr_reset();
void close_log_file();
char* stristr(const char* str1, const char* str2);
void DisplayTimedMessageV(uint32_t seconds, int row, const char *format, ...);

void ShowMatrix3(const Matrix3& mat, char* name);

int EigenVals(float3x3& A, Vector3& w);
int EigenVals(Matrix3& A, Vector3& w);

int EigenVectors(float3x3& A, float3x3& Q, Vector3& w);
int EigenVectors(Matrix3& A, Matrix3& Q, Vector3& w);

int EigenSys(Matrix3& A, Matrix3& V, Vector3& w);