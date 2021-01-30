// From: https://docs.microsoft.com/en-us/previous-versions/windows/apps/jj207074(v=vs.105)?redirectedfrom=MSDN
#include "VideoPlayback.h"

class MediaEngineNotify : public IMFMediaEngineNotify
{
	long m_cRef;
	MediaEngineNotifyCallback* m_pCB;

public:
	MediaEngineNotify() :
		m_cRef(1),
		m_pCB(nullptr)
	{
	}

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		if (__uuidof(IMFMediaEngineNotify) == riid)
		{
			*ppv = static_cast<IMFMediaEngineNotify*>(this);
		}
		else
		{
			*ppv = nullptr;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		LONG cRef = InterlockedDecrement(&m_cRef);
		if (cRef == 0)
		{
			delete this;
		}
		return cRef;
	}

	void MediaEngineNotifyCallback(MediaEngineNotifyCallback* pCB)
	{
		m_pCB = pCB;
	}

	// EventNotify is called when the Media Engine sends an event.
	STDMETHODIMP EventNotify(DWORD meEvent, DWORD_PTR param1, DWORD param2)
	{
		if (meEvent == MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE)
		{
			SetEvent(reinterpret_cast<HANDLE>(param1));
		}
		else
		{
			m_pCB->OnMediaEngineEvent(meEvent);
		}

		return S_OK;
	}

};

MediaEnginePlayer::MediaEnginePlayer() :
	m_spMediaEngine(nullptr),
	m_spEngineEx(nullptr),
	m_isPlaying(false)
{
	memset(&m_bkgColor, 0, sizeof(MFARGB));
}

MediaEnginePlayer::~MediaEnginePlayer()
{
	Shutdown();
	MFShutdown();
}

void MediaEnginePlayer::Shutdown()
{
	if (m_spMediaEngine)
	{
		m_spMediaEngine->Shutdown();
	}
	return;
}

void MediaEnginePlayer::Initialize(
	ComPtr<ID3D11Device> device,
	DXGI_FORMAT d3dFormat)
{
	ComPtr<IMFMediaEngineClassFactory> spFactory;
	ComPtr<IMFAttributes> spAttributes;
	ComPtr<MediaEngineNotify> spNotify;

	if (FAILED(MFStartup(MF_VERSION)))
		return;

	UINT resetToken;
	ComPtr<IMFDXGIDeviceManager> DXGIManager;
	if (FAILED(MFCreateDXGIDeviceManager(&resetToken, &DXGIManager)))
		return;
	if (FAILED(DXGIManager->ResetDevice(device.Get(), resetToken)))
		return;

	// Create our event callback object.
	spNotify = new MediaEngineNotify();
	if (spNotify == nullptr)
	{
		//DX::ThrowIfFailed(E_OUTOFMEMORY);
		return;
	}

	spNotify->MediaEngineNotifyCallback(this);

	// Set configuration attribiutes.
	if (FAILED(MFCreateAttributes(&spAttributes, 1)))
		return;
	if (FAILED(spAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*)DXGIManager.Get())))
		return;
	if (FAILED(spAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)spNotify.Get())))
		return;
	if (FAILED(spAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, d3dFormat)))
		return;

	// Create MediaEngine.
	if (FAILED(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spFactory))))
		return;
	if (FAILED(spFactory->CreateInstance(0, spAttributes.Get(), &m_spMediaEngine)))
		return;

	// Create MediaEngineEx
	if (FAILED(m_spMediaEngine.Get()->QueryInterface(__uuidof(IMFMediaEngine), (void**)&m_spEngineEx)))
		return;
}

void MediaEnginePlayer::SetSource(wchar_t *szURL)
{
	BSTR bstrURL = nullptr;

	//if (nullptr != bstrURL)
	//{
	//	::CoTaskMemFree(bstrURL);
	//	bstrURL = nullptr;
	//}

	//size_t cchAllocationSize = 1 + ::wcslen(szURL->Data());
	//bstrURL = (LPWSTR)::CoTaskMemAlloc(sizeof(WCHAR)*(cchAllocationSize));

	//if (bstrURL == 0)
	//{
		//DX::ThrowIfFailed(E_OUTOFMEMORY);
		//return;
	//}

	//StringCchCopyW(bstrURL, cchAllocationSize, szURL);

	m_spMediaEngine->SetSource(szURL);
	return;
}

void MediaEnginePlayer::Play()
{
	if (m_spMediaEngine)
	{
		if (FAILED(m_spMediaEngine->Play()))
			return;
		m_isPlaying = true;
	}
}

void MediaEnginePlayer::Pause()
{
	if (m_spMediaEngine && IsPlaying())
	{
		if (FAILED(m_spMediaEngine->Pause()))
			return;
		m_isPlaying = false;
	}
}

bool MediaEnginePlayer::IsPlaying()
{
	return m_isPlaying;
}

void MediaEnginePlayer::GetNativeVideoSize(DWORD *cx, DWORD *cy)
{
	if (m_spMediaEngine)
	{
		m_spMediaEngine->GetNativeVideoSize(cx, cy);
	}
	return;
}

void MediaEnginePlayer::TransferFrame(ComPtr<ID3D11Texture2D> texture, MFVideoNormalizedRect rect, RECT rcTarget)
{
	if (m_spMediaEngine != nullptr && m_isPlaying)
	{
		LONGLONG pts;
		if (m_spMediaEngine->OnVideoStreamTick(&pts) == S_OK)
		{
			// new frame available at the media engine so get it 
			if (FAILED(m_spMediaEngine->TransferVideoFrame(texture.Get(), &rect, &rcTarget, &m_bkgColor)))
				return;
		}
	}
}

/*

	m_player = std::make_unique<MediaEnginePlayer>();
	m_player->Initialize(device, DXGI_FORMAT_B8G8R8A8_UNORM);
	m_player->SetSource(L"SampleVideo.mp4");

*/