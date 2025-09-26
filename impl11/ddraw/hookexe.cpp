#include "targetver.h"
#include <Windows.h>
#include "hookexe.h"

HWND g_hookexe = NULL;

enum ExeDataType
{
	ExeDataType_None,
	ExeDataType_ShowMessage,
	ExeDataType_LoadTexture_Result,
	ExeDataType_LoadTexture_Handle,
	ExeDataType_LoadTexture_Release,

	ExeDataType_DATLoadTexture_Result,
};

void ExeShowMessage(const std::string& message)
{
	if (!g_hookexe)
	{
		return;
	}

	COPYDATASTRUCT cds{};
	cds.dwData = ExeDataType_ShowMessage;
	cds.cbData = message.length() + 1;
	cds.lpData = (void*)message.data();

	SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);
}

HRESULT ExeLoadTexture(const std::wstring& name, HANDLE* handle)
{
	*handle = 0;

	if (!g_hookexe)
	{
		return E_ABORT;
	}

	COPYDATASTRUCT cds{};
	cds.dwData = ExeDataType_LoadTexture_Result;
	cds.cbData = (name.length() + 1) * sizeof(wchar_t);
	cds.lpData = (void*)name.data();
	HRESULT hr = (HRESULT)SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);

	if (SUCCEEDED(hr))
	{
		cds.dwData = ExeDataType_LoadTexture_Handle;
		cds.cbData = 0;
		cds.lpData = nullptr;
		*handle    = (HANDLE)SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);
	}

	return hr;
}

void ExeLoadTextureRelease()
{
	if (!g_hookexe)
	{
		return;
	}

	COPYDATASTRUCT cds{};
	cds.dwData = ExeDataType_LoadTexture_Release;
	cds.cbData = 0;
	cds.lpData = nullptr;
	SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);
}

constexpr int MAX_DAT_FILE_NAME = 128;
struct DATLoadTextureArgs
{
	char  sDATFileName[MAX_DAT_FILE_NAME];
	int   GroupId   = -1;
	int   ImageId   = -1;
	bool  flipImage = false;
};

HRESULT ExeDATLoadTexture(const char* sDATFileName, int GroupId, int ImageId, HANDLE* handle)
{
	*handle = 0;

	if (!g_hookexe)
	{
		return E_ABORT;
	}

	DATLoadTextureArgs args;
	strcpy_s(args.sDATFileName, MAX_DAT_FILE_NAME, sDATFileName);
	args.GroupId   = GroupId;
	args.ImageId   = ImageId;
	args.flipImage = false;

	COPYDATASTRUCT cds{};
	cds.dwData = ExeDataType_DATLoadTexture_Result;
	cds.cbData = sizeof(args);
	cds.lpData = (void*)&args;
	HRESULT hr = (HRESULT)SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);

	/*if (SUCCEEDED(hr))
	{
		cds.dwData = ExeDataType_LoadTexture_Handle;
		cds.cbData = 0;
		cds.lpData = nullptr;
		*handle    = (HANDLE)SendMessage(g_hookexe, WM_COPYDATA, NULL, (LPARAM)&cds);
	}*/

	return hr;
}