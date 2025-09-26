#pragma once
#include <string>

extern HWND g_hookexe;

void ExeShowMessage(const std::string& message);

HRESULT ExeLoadTexture(const std::wstring& name, HANDLE* handle);
void ExeLoadTextureRelease();

HRESULT ExeDATLoadTexture(const char* sDATFileName, int GroupId, int ImageId, HANDLE* handle);