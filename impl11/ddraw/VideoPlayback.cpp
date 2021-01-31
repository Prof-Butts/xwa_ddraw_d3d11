// From: https://docs.microsoft.com/en-us/previous-versions/windows/apps/jj207074(v=vs.105)?redirectedfrom=MSDN
#include "VideoPlayback.h"

void log_debug(const char *format, ...);

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
	IMFMediaEngineClassFactory *spFactory = nullptr;
	IMFAttributes *spAttributes = nullptr;
	MediaEngineNotify *spNotify = nullptr;
	IMFDXGIDeviceManager *DXGIManager = nullptr;
	HRESULT hr = S_OK;
	UINT resetToken;

	if (FAILED(MFStartup(MF_VERSION))) {
		log_debug("[DBG] Failed at MFStartup");
		return;
	}
	
	if (FAILED(MFCreateDXGIDeviceManager(&resetToken, &DXGIManager))) {
		log_debug("[DBG] Failed at MFCreateDXGIDeviceManager");
		return;
	}
	if (FAILED(DXGIManager->ResetDevice(device.Get(), resetToken))) {
		log_debug("[DBG] Failed at ResetDevice");
		return;
	}

	// Create our event callback object.
	spNotify = new MediaEngineNotify();
	if (spNotify == nullptr)
	{
		//DX::ThrowIfFailed(E_OUTOFMEMORY);
		log_debug("[DBG] Failed at MediaEngineNotify (E_OUTOFMEMORY)");
		return;
	}
	//log_debug("[DBG] MediaEngineNotify created");

	spNotify->MediaEngineNotifyCallback(this);

	// Set configuration attribiutes.
	if (FAILED(MFCreateAttributes(&spAttributes, 1))) {
		log_debug("[DBG] Failed at MFCreateAttributes");
		return;
	}
	//log_debug("[DBG] CHECK 2");
	if (FAILED(spAttributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, (IUnknown*)DXGIManager))) {
		log_debug("[DBG] Failed at SetUnknown 1");
		return;
	}
	if (FAILED(spAttributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, (IUnknown*)spNotify))) {
		log_debug("[DBG] Failed at SetUnknown 2");
		return;
	}
	//log_debug("[DBG] CHECK 3");
	if (FAILED(spAttributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, d3dFormat))) {
		log_debug("[DBG] Failed at SetUINT32");
		return;
	}

	// Create MediaEngine.
	if (FAILED(hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&spFactory)))) {
		log_debug("[DBG] Failed at CoCreateInstance, hr: 0x%x", hr);
		return;
	}
	if (spFactory == nullptr) {
		log_debug("[DBG] spFactory == NULL");
		return;
	}
	//log_debug("[DBG] CHECK 4, spFactory: 0x%x", spFactory);
	//log_debug("[DBG] spAttributes: 0x%x", spAttributes);
	//log_debug("[DBG] m_spMediaEngine: 0x%x", m_spMediaEngine);

	if (FAILED(hr = spFactory->CreateInstance(0, spAttributes, &m_spMediaEngine))) {
		log_debug("[DBG] Failed at CreateInstance, hr: 0x%x", hr);
		return;
	}
	//log_debug("[DBG] CHECK 5, m_spMediaEngine: 0x%x", m_spMediaEngine);
	//log_debug("[DBG] m_spEngineEx: 0x%x", m_spEngineEx);
	// Create MediaEngineEx
	//if (FAILED(m_spMediaEngine.Get()->QueryInterface(__uuidof(IMFMediaEngine), (void**)&m_spEngineEx))) {
	if (FAILED(m_spMediaEngine->QueryInterface(__uuidof(IMFMediaEngine), (void**)&m_spEngineEx))) {
		log_debug("[DBG] Failed at QueryInterface");
		return;
	}
	
	//if (FAILED(hr = m_spMediaEngine.As(&m_spEngineEx))) {
	//	log_debug("[DBG] Failed at m_spEngineEx, hr: 0x%x", hr);
	//	return;
	//}
	//log_debug("[DBG] CHECK 6 m_spEngineEx: 0x%x", m_spEngineEx);
	spFactory->Release();
	spAttributes->Release();
	spNotify->Release();
	DXGIManager->Release();
	log_debug("[DBG] MediaEnginePlayer Successfully Initialized");
}

void MediaEnginePlayer::SetSource(wchar_t *szURL)
{
	BSTR bstrURL = nullptr;
	HRESULT hr = S_OK;

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

	if (FAILED(hr = m_spMediaEngine->SetSource(szURL))) {
		log_debug("[DBG] Failed to load: %ls, hr: 0x%x", szURL, hr);
	}
	log_debug("[DBG] Successfully loaded: %ls", szURL);
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

void MediaEnginePlayer::OnMediaEngineEvent(DWORD meEvent)
{
	switch (meEvent)
	{
	case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
		break;

	case MF_MEDIA_ENGINE_EVENT_CANPLAY:
		Play();
		break;

	case MF_MEDIA_ENGINE_EVENT_PLAY:
		break;

	case MF_MEDIA_ENGINE_EVENT_PAUSE:
		break;

	case MF_MEDIA_ENGINE_EVENT_ENDED:
		break;

	case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
		break;

	case MF_MEDIA_ENGINE_EVENT_ERROR:
		if (m_spMediaEngine)
		{
			ComPtr<IMFMediaError> error;
			m_spMediaEngine->GetError(&error);
			USHORT errorCode = error->GetErrorCode();
		}
		break;
	}

}

/*

	m_player = std::make_unique<MediaEnginePlayer>();
	m_player->Initialize(device, DXGI_FORMAT_B8G8R8A8_UNORM);
	m_player->SetSource(L"SampleVideo.mp4");

*/