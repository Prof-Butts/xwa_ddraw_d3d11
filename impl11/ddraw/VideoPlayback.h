#pragma once

//#include "DirectXHelper.h"
//#include <wrl.h>
#include <mfmediaengine.h>
//#include <strsafe.h>
#include <mfapi.h>
//#include <agile.h>

#include <ddraw.h>
#include <d3d.h>

#include <dxgi.h>
#include <d3d11.h>

#include "ComPtr.h"

// MediaEngineNotifyCallback - Defines the callback method to process media engine events.
struct MediaEngineNotifyCallback abstract
{
	virtual void OnMediaEngineEvent(DWORD meEvent) = 0;
};

class MediaEnginePlayer : public MediaEngineNotifyCallback
{
	IMFMediaEngine *m_spMediaEngine;
	IMFMediaEngineEx *m_spEngineEx;
	MFARGB                           m_bkgColor;

public:
	MediaEnginePlayer();
	~MediaEnginePlayer();

	// Media Info
	void GetNativeVideoSize(DWORD *cx, DWORD *cy);
	bool IsPlaying();

	// Initialize/Shutdown
	void Initialize(ComPtr<ID3D11Device> device, DXGI_FORMAT d3dFormat);
	void Shutdown();

	// Media Engine related
	void OnMediaEngineEvent(DWORD meEvent);

	// Media Engine Actions
	void Play();
	void Pause();
	//void SetMuted(bool muted);

	// Media Source
	void SetSource(wchar_t *sourceUri);
	//void SetBytestream(IRandomAccessStream^ streamHandle, Platform::String^ szURL);

	// Transfer Video Frame
	void TransferFrame(ComPtr<ID3D11Texture2D> texture, MFVideoNormalizedRect rect, RECT rcTarget);

private:
	bool m_isPlaying;
};