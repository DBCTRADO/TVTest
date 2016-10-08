#include "stdafx.h"
#include "EVRPresenterBase.h"
#include "EVRPresenter.h"
#include "../../Common/DebugDef.h"


namespace
{


inline float MFOffsetToFloat(const MFOffset& Offset)
{
	return static_cast<float>(Offset.value) + (static_cast<float>(Offset.fract) / 65536.0f);
}


RECT CorrectAspectRatio(const RECT &SrcRect, const MFRatio &SrcPAR, const MFRatio &DestPAR)
{
	RECT rc = {0, 0, SrcRect.right - SrcRect.left, SrcRect.bottom - SrcRect.top};

	if ((SrcPAR.Numerator != DestPAR.Numerator) || (SrcPAR.Denominator != DestPAR.Denominator)) {
		if (SrcPAR.Numerator > SrcPAR.Denominator) {
			rc.right = ::MulDiv(rc.right, SrcPAR.Numerator, SrcPAR.Denominator);
		} else if (SrcPAR.Numerator < SrcPAR.Denominator) {
			rc.bottom = ::MulDiv(rc.bottom, SrcPAR.Denominator, SrcPAR.Numerator);
		}

		if (DestPAR.Numerator > DestPAR.Denominator) {
			rc.bottom = ::MulDiv(rc.bottom, DestPAR.Numerator, DestPAR.Denominator);
		} else if (DestPAR.Numerator < DestPAR.Denominator) {
			rc.right = ::MulDiv(rc.right, DestPAR.Denominator, DestPAR.Numerator);
		}
	}

	return rc;
}


bool AreMediaTypesEqual(IMFMediaType *pType1, IMFMediaType *pType2)
{
	if (pType1 == pType2) {
		return true;
	}
	if ((pType1 == nullptr) || (pType2 == nullptr)) {
		return false;
	}

	DWORD Flags = 0;
	HRESULT hr = pType1->IsEqual(pType2, &Flags);

	return hr == S_OK;
}


HRESULT ValidateVideoArea(const MFVideoArea &Area, UINT32 Width, UINT32 Height)
{
	float OffsetX = MFOffsetToFloat(Area.OffsetX);
	float OffsetY = MFOffsetToFloat(Area.OffsetY);

	if ((static_cast<LONG>(OffsetX) + Area.Area.cx > static_cast<LONG>(Width)) ||
		(static_cast<LONG>(OffsetY) + Area.Area.cy > static_cast<LONG>(Height))) {
		return MF_E_INVALIDMEDIATYPE;
	}

	return S_OK;
}


HRESULT SetDesiredSampleTime(IMFSample *pSample, LONGLONG hnsSampleTime, LONGLONG hnsDuration)
{
	if (pSample == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;
	IMFDesiredSample *pDesired = nullptr;

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pDesired));
	if (SUCCEEDED(hr)) {
		pDesired->SetDesiredSampleTimeAndDuration(hnsSampleTime, hnsDuration);
		pDesired->Release();
	}

	return hr;
}


HRESULT ClearDesiredSampleTime(IMFSample *pSample)
{
	if (pSample == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;

	IUnknown *pUnkSwapChain = nullptr;
	IMFDesiredSample *pDesired = nullptr;

	UINT32 Counter = ::MFGetAttributeUINT32(pSample, SampleAttribute_Counter, (UINT32)-1);

	pSample->GetUnknown(SampleAttribute_SwapChain, IID_PPV_ARGS(&pUnkSwapChain));

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pDesired));
	if (SUCCEEDED(hr)) {
		pDesired->Clear();

		hr = pSample->SetUINT32(SampleAttribute_Counter, Counter);
		if (SUCCEEDED(hr)) {
			if (pUnkSwapChain != nullptr) {
				hr = pSample->SetUnknown(SampleAttribute_SwapChain, pUnkSwapChain);
			}
		}

		pDesired->Release();
	}

	SafeRelease(pUnkSwapChain);

	return hr;
}


bool IsSampleTimePassed(IMFClock *pClock, IMFSample *pSample)
{
	if ((pSample == nullptr) || (pClock == nullptr)) {
		return false;
	}

	HRESULT hr;
	MFTIME hnsTimeNow = 0;
	MFTIME hnsSystemTime = 0;
	MFTIME hnsSampleStart = 0;
	MFTIME hnsSampleDuration = 0;

	hr = pClock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

	if (SUCCEEDED(hr)) {
		hr = pSample->GetSampleTime(&hnsSampleStart);
	}
	if (SUCCEEDED(hr)) {
		hr = pSample->GetSampleDuration(&hnsSampleDuration);
	}

	if (SUCCEEDED(hr)) {
		if (hnsSampleStart + hnsSampleDuration < hnsTimeNow) {
			return true;
		}
	}

	return false;
}


HRESULT SetMixerSourceRect(IMFTransform *pMixer, const MFVideoNormalizedRect &nrcSource)
{
	if (pMixer == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;
	IMFAttributes *pAttributes = nullptr;

	hr = pMixer->GetAttributes(&pAttributes);
	if (SUCCEEDED(hr)) {
		hr = pAttributes->SetBlob(VIDEO_ZOOM_RECT, (const UINT8*)&nrcSource, sizeof(nrcSource));
		pAttributes->Release();
	}

	return hr;
}


}




HRESULT CEVRPresenter::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	if (ppv == nullptr) {
		return E_POINTER;
	}

	*ppv = nullptr;

	if (pUnkOuter != nullptr) {
		return CLASS_E_NOAGGREGATION;
	}

	HRESULT hr = S_OK;

	CEVRPresenter *pPresenter;
	try {
		pPresenter = new CEVRPresenter(hr);
	} catch (...) {
		return E_OUTOFMEMORY;
	}

	hr = pPresenter->QueryInterface(riid, ppv);

	pPresenter->Release();

	return hr;
}


#pragma warning(disable : 4355)	// 'this' used in base member initializer list

CEVRPresenter::CEVRPresenter(HRESULT &hr)
	: m_RenderState(RenderState_Shutdown)
	, m_pPresentEngine(nullptr)
	, m_pClock(nullptr)
	, m_pMixer(nullptr)
	, m_pMediaEventSink(nullptr)
	, m_pMediaType(nullptr)
	, m_bSampleNotify(false)
	, m_bRepaint(false)
	, m_bEndStreaming(false)
	, m_bPrerolled(false)
	, m_NativeVideoSize()
	, m_NativeAspectRatio()
	, m_fRate(1.0f)
	, m_AspectRatioMode(MFVideoARMode_PreservePicture)
	, m_RenderPrefs(0)
	, m_TokenCounter(0)
	, m_SampleFreeCB(this, &CEVRPresenter::OnSampleFree)
{
	hr = S_OK;

	m_nrcSource.top    = 0.0f;
	m_nrcSource.left   = 0.0f;
	m_nrcSource.bottom = 1.0f;
	m_nrcSource.right  = 1.0f;

	try {
		m_pPresentEngine = new CEVRPresentEngine(hr);

		m_Scheduler.SetCallback(m_pPresentEngine);
	} catch (...) {
		hr = E_OUTOFMEMORY;
	}
}


CEVRPresenter::~CEVRPresenter()
{
	SafeRelease(m_pClock);
	SafeRelease(m_pMixer);
	SafeRelease(m_pMediaEventSink);
	SafeRelease(m_pMediaType);

	SafeDelete(m_pPresentEngine);
}


HRESULT CEVRPresenter::QueryInterface(REFIID riid, void **ppv)
{
	if (ppv == nullptr) {
		return E_POINTER;
	}

	if (riid == __uuidof(IUnknown)) {
		*ppv = static_cast<IUnknown*>(static_cast<IMFVideoPresenter*>(this));
	} else if (riid == __uuidof(IMFVideoDeviceID)) {
		*ppv = static_cast<IMFVideoDeviceID*>(this);
	} else if (riid == __uuidof(IMFVideoPresenter)) {
		*ppv = static_cast<IMFVideoPresenter*>(this);
	} else if (riid == __uuidof(IMFClockStateSink)) {
		*ppv = static_cast<IMFClockStateSink*>(this);
	} else if (riid == __uuidof(IMFRateSupport)) {
		*ppv = static_cast<IMFRateSupport*>(this);
	} else if (riid == __uuidof(IMFGetService)) {
		*ppv = static_cast<IMFGetService*>(this);
	} else if (riid == __uuidof(IMFTopologyServiceLookupClient)) {
		*ppv = static_cast<IMFTopologyServiceLookupClient*>(this);
	} else if (riid == __uuidof(IMFVideoDisplayControl)) {
		*ppv = static_cast<IMFVideoDisplayControl*>(this);
	} else {
		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


ULONG CEVRPresenter::AddRef()
{
	return RefCountedObject::AddRef();
}


ULONG CEVRPresenter::Release()
{
	return RefCountedObject::Release();
}


HRESULT CEVRPresenter::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (ppvObject == nullptr) {
		return E_POINTER;
	}

	*ppvObject = nullptr;

	HRESULT hr = m_pPresentEngine->GetService(guidService, riid, ppvObject);

	if (FAILED(hr)) {
		if (guidService != MR_VIDEO_RENDER_SERVICE) {
			return MF_E_UNSUPPORTED_SERVICE;
		}

		hr = QueryInterface(riid, ppvObject);
	}

	return hr;
}


HRESULT CEVRPresenter::GetDeviceID(IID *pDeviceID)
{
	if (pDeviceID == nullptr) {
		return E_POINTER;
	}

	*pDeviceID = __uuidof(IDirect3DDevice9);

	return S_OK;
}


HRESULT CEVRPresenter::InitServicePointers(IMFTopologyServiceLookup *pLookup)
{
	if (pLookup == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	if (IsActive()) {
		return MF_E_INVALIDREQUEST;
	}

	SafeRelease(m_pClock);
	SafeRelease(m_pMixer);
	SafeRelease(m_pMediaEventSink);

	HRESULT hr;

	DWORD ObjectCount = 1;

	pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_RENDER_SERVICE,
		IID_PPV_ARGS(&m_pClock),
		&ObjectCount
	);

	ObjectCount = 1;

	hr = pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_MIXER_SERVICE,
		IID_PPV_ARGS(&m_pMixer),
		&ObjectCount
	);
	if (FAILED(hr)) {
		return hr;
	}

	hr = ConfigureMixer(m_pMixer);
	if (FAILED(hr)) {
		return hr;
	}

	ObjectCount = 1;

	hr = pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_RENDER_SERVICE,
		IID_PPV_ARGS(&m_pMediaEventSink),
		&ObjectCount
	);
	if (FAILED(hr)) {
		return hr;
	}

	m_RenderState = RenderState_Stopped;

	return S_OK;
}


HRESULT CEVRPresenter::ReleaseServicePointers()
{
	{
		CAutoLock Lock(&m_ObjectLock);
		m_RenderState = RenderState_Shutdown;
	}

	Flush();

	SetMediaType(nullptr);

	SafeRelease(m_pClock);
	SafeRelease(m_pMixer);
	SafeRelease(m_pMediaEventSink);

	return S_OK;
}


HRESULT CEVRPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	switch (eMessage) {
	// 待機中のサンプルをフラッシュ
	case MFVP_MESSAGE_FLUSH:
		TRACE(TEXT("MFVP_MESSAGE_FLUSH\n"));
		hr = Flush();
		break;

	// メディアタイプの交渉
	case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
		TRACE(TEXT("MFVP_MESSAGE_INVALIDATEMEDIATYPE\n"));
		hr = RenegotiateMediaType();
		break;

	// ミキサーがサンプルを受け取った
	case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
		hr = ProcessInputNotify();
		break;

	// ストリーミング開始
	case MFVP_MESSAGE_BEGINSTREAMING:
		TRACE(TEXT("MFVP_MESSAGE_BEGINSTREAMING\n"));
		hr = BeginStreaming();
		break;

	// ストリームング終了(EVR停止)
	case MFVP_MESSAGE_ENDSTREAMING:
		TRACE(TEXT("MFVP_MESSAGE_ENDSTREAMING\n"));
		hr = EndStreaming();
		break;

	// 入力ストリーム終了
	case MFVP_MESSAGE_ENDOFSTREAM:
		TRACE(TEXT("MFVP_MESSAGE_ENDOFSTREAM\n"));
		m_bEndStreaming = true; 
		hr = CheckEndOfStream();
		break;

	// フレーム飛ばし
	case MFVP_MESSAGE_STEP:
		hr = PrepareFrameStep(LODWORD(ulParam));
		break;

	// フレーム飛ばしキャンセル
	case MFVP_MESSAGE_CANCELSTEP:
		hr = CancelFrameStep();
		break;

	default:
		hr = E_INVALIDARG;
		break;
	}

	return hr;
}


HRESULT CEVRPresenter::GetCurrentMediaType(IMFVideoMediaType **ppMediaType)
{
	if (ppMediaType == nullptr) {
		return E_POINTER;
	}

	*ppMediaType = nullptr;

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_pMediaType == nullptr) {
		return MF_E_NOT_INITIALIZED;
	}

	return m_pMediaType->QueryInterface(IID_PPV_ARGS(ppMediaType));
}


HRESULT CEVRPresenter::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (IsActive()) {
		m_RenderState = RenderState_Started;

		if (llClockStartOffset != PRESENTATION_CURRENT_POSITION) {
			Flush();
		}
	} else {
		m_RenderState = RenderState_Started;

		hr = StartFrameStep();
		if (FAILED(hr)) {
			return hr;
		}
	}

	ProcessOutputLoop();

	return S_OK;
}


HRESULT CEVRPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	_ASSERT(m_RenderState == RenderState_Paused);

	m_RenderState = RenderState_Started;

	hr = StartFrameStep();
	if (FAILED(hr)) {
		return hr;
	}

	ProcessOutputLoop();

	return S_OK;
}


HRESULT CEVRPresenter::OnClockStop(MFTIME hnsSystemTime)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_RenderState != RenderState_Stopped) {
		m_RenderState = RenderState_Stopped;
		Flush();

		if (m_FrameStep.State != FrameStep_None) {
			CancelFrameStep();
		}
	}

	return S_OK;
}


HRESULT CEVRPresenter::OnClockPause(MFTIME hnsSystemTime)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	m_RenderState = RenderState_Paused;

	return S_OK;
}


HRESULT CEVRPresenter::OnClockSetRate(MFTIME hnsSystemTime, float fRate)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if ((m_fRate == 0.0f) && (fRate != 0.0f)) {
		CancelFrameStep();
		m_FrameStep.Samples.Clear();
	}

	m_fRate = fRate;

	m_Scheduler.SetClockRate(fRate);

	return S_OK;
}


HRESULT CEVRPresenter::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pfRate)
{
	if (pfRate == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	*pfRate = 0;

	return S_OK;
}


HRESULT CEVRPresenter::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pfRate)
{
	if (pfRate == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	float fMaxRate = GetMaxRate(bThin);

	if (eDirection == MFRATE_REVERSE) {
		fMaxRate = -fMaxRate;
	}

	*pfRate = fMaxRate;

	return S_OK;
}


HRESULT CEVRPresenter::IsRateSupported(BOOL bThin, float fRate, float *pfNearestSupportedRate)
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	float fNearestRate = fRate;
	float fMaxRate = GetMaxRate(bThin);

	if (std::fabs(fRate) > fMaxRate) {
		hr = MF_E_UNSUPPORTED_RATE;

		fNearestRate = fMaxRate;
		if (fRate < 0.0) {
			fNearestRate = -fNearestRate;
		}
	}

	if (pfNearestSupportedRate != nullptr) {
		*pfNearestSupportedRate = fNearestRate;
	}

	return S_OK;
}


HRESULT CEVRPresenter::SetVideoWindow(HWND hwndVideo)
{
	if (!::IsWindow(hwndVideo)) {
		return E_INVALIDARG;
	}

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr = S_OK;
	HWND hwndOld = m_pPresentEngine->GetVideoWindow();

	if (hwndOld != hwndVideo) {
		hr = m_pPresentEngine->SetVideoWindow(hwndVideo);

		NotifyEvent(EC_DISPLAY_CHANGED, 0, 0);
	}

	return hr;
}


HRESULT CEVRPresenter::GetVideoWindow(HWND* phwndVideo)
{
	if (phwndVideo == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	*phwndVideo = m_pPresentEngine->GetVideoWindow();

	return S_OK;
}


HRESULT CEVRPresenter::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
{
	if ((pszVideo == nullptr) || (pszARVideo == nullptr)) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (pszVideo != nullptr) {
		*pszVideo = m_NativeVideoSize;
	}

	if (pszARVideo != nullptr) {
		pszARVideo->cx = m_NativeAspectRatio.Numerator;
		pszARVideo->cy = m_NativeAspectRatio.Denominator;
	}

	return S_OK;
}


HRESULT CEVRPresenter::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
	if ((pnrcSource == nullptr) && (prcDest == nullptr)) {
		return E_POINTER;
	}

	if (pnrcSource != nullptr) {
		if ((pnrcSource->left > pnrcSource->right) ||
			(pnrcSource->top > pnrcSource->bottom)) {
			return E_INVALIDARG;
		}

		if ((pnrcSource->left < 0.0f) || (pnrcSource->right > 1.0f) ||
			(pnrcSource->top < 0.0f) || (pnrcSource->bottom > 1.0f)) {
			return E_INVALIDARG;
		}
	}

	if (prcDest != nullptr) {
		if ((prcDest->left > prcDest->right) ||
			(prcDest->top > prcDest->bottom)) {
			return E_INVALIDARG;
		}
	}

	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr;

	bool bChanged = false;

	if (pnrcSource != nullptr) {
		if ((pnrcSource->left != m_nrcSource.left) ||
			(pnrcSource->top != m_nrcSource.top) ||
			(pnrcSource->right != m_nrcSource.right) ||
			(pnrcSource->bottom != m_nrcSource.bottom)) {
			m_nrcSource = *pnrcSource;

			if (m_pMixer != nullptr) {
				hr = SetMixerSourceRect(m_pMixer, m_nrcSource);
				if (FAILED(hr)) {
					return hr;
				}
			}

			bChanged = true;
		}
	}

	if (prcDest != nullptr) {
		RECT rcOldDest = m_pPresentEngine->GetDestinationRect();

		if (!::EqualRect(prcDest, &rcOldDest)) {
			hr = m_pPresentEngine->SetDestinationRect(*prcDest);
			if (FAILED(hr)) {
				return hr;
			}

			bChanged = true;
		}
	}

	if (bChanged && (m_pMixer != nullptr)) {
		hr = RenegotiateMediaType();
		if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
			hr = S_OK;
		} else {
			if (FAILED(hr)) {
				return hr;
			}

			m_bRepaint = true;
			ProcessOutput();
		}
	}

	return S_OK;
}


HRESULT CEVRPresenter::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
	if ((pnrcSource == nullptr) || (prcDest == nullptr)) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	*pnrcSource = m_nrcSource;
	*prcDest = m_pPresentEngine->GetDestinationRect();

	return S_OK;
}


HRESULT CEVRPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	if (dwAspectRatioMode & ~MFVideoARMode_Mask) {
		return E_INVALIDARG;
	}

	CAutoLock Lock(&m_ObjectLock);

	m_AspectRatioMode = dwAspectRatioMode;

	return S_OK;
}


HRESULT CEVRPresenter::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
	if (pdwAspectRatioMode == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	*pdwAspectRatioMode = m_AspectRatioMode;

	return S_OK;
}


HRESULT CEVRPresenter::RepaintVideo()
{
	CAutoLock Lock(&m_ObjectLock);

	HRESULT hr = S_OK;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_bPrerolled) {
		m_bRepaint = true;
		ProcessOutput();
	}

	return S_OK;
}


HRESULT CEVRPresenter::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	return m_pPresentEngine->GetCurrentImage(pBih, pDib, pcbDib, pTimeStamp);
}


HRESULT CEVRPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
	if (dwRenderFlags & ~MFVideoRenderPrefs_Mask) {
		return E_INVALIDARG;
	}

	CAutoLock Lock(&m_ObjectLock);

	m_RenderPrefs = dwRenderFlags;

	return S_OK;
}


HRESULT CEVRPresenter::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
	if (pdwRenderFlags == nullptr) {
		return E_POINTER;
	}

	CAutoLock Lock(&m_ObjectLock);

	*pdwRenderFlags = m_RenderPrefs;

	return S_OK;
}


HRESULT CEVRPresenter::ConfigureMixer(IMFTransform *pMixer)
{
	HRESULT hr;

	IMFVideoDeviceID *pDeviceID = nullptr;

	hr = pMixer->QueryInterface(IID_PPV_ARGS(&pDeviceID));

	if (SUCCEEDED(hr)) {
		IID DeviceID = GUID_NULL;

		hr = pDeviceID->GetDeviceID(&DeviceID);
		if (SUCCEEDED(hr)) {
			if (DeviceID == __uuidof(IDirect3DDevice9)) {
				SetMixerSourceRect(pMixer, m_nrcSource);
			} else {
				hr = MF_E_INVALIDREQUEST;
			}
		}

		pDeviceID->Release();
	}

	return hr;
}


HRESULT CEVRPresenter::RenegotiateMediaType()
{
	if (m_pMixer == nullptr) {
		return MF_E_INVALIDREQUEST;
	}

	HRESULT hr;
	bool bFoundMediaType = false;

	DWORD TypeIndex = 0;

	do {
		IMFMediaType *pMixerType = nullptr;

		hr = m_pMixer->GetOutputAvailableType(0, TypeIndex++, &pMixerType);
		if (FAILED(hr)) {
			break;
		}

		hr = IsMediaTypeSupported(pMixerType);

		if (SUCCEEDED(hr)) {
			IMFMediaType *pOptimalType = nullptr;

			hr = CreateOptimalVideoType(pMixerType, &pOptimalType);

			if (SUCCEEDED(hr)) {
				hr = m_pMixer->SetOutputType(0, pOptimalType, MFT_SET_TYPE_TEST_ONLY);

				if (SUCCEEDED(hr)) {
					hr = SetMediaType(pOptimalType);

					if (SUCCEEDED(hr)) {
						hr = m_pMixer->SetOutputType(0, pOptimalType, 0);

						if (SUCCEEDED(hr)) {
							VideoType mt(pMixerType);

							mt.GetFrameDimensions((UINT32*)&m_NativeVideoSize.cx, (UINT32*)&m_NativeVideoSize.cy);
							m_NativeAspectRatio = mt.GetPixelAspectRatio();

							bFoundMediaType = true;
						} else {
							SetMediaType(nullptr);
						}
					}
				}

				pOptimalType->Release();
			}
		}

		pMixerType->Release();
	} while (!bFoundMediaType);

	return hr;
}


HRESULT CEVRPresenter::Flush()
{
	m_bPrerolled = false;

	m_Scheduler.Flush();

	m_FrameStep.Samples.Clear();

	if (m_RenderState == RenderState_Stopped) {
		m_pPresentEngine->PresentSample(nullptr, 0);
	}

	return S_OK;
}


HRESULT CEVRPresenter::ProcessInputNotify()
{
	HRESULT hr = S_OK;

	m_bSampleNotify = true;

	if (m_pMediaType == nullptr) {
		hr = MF_E_TRANSFORM_TYPE_NOT_SET;
	} else {
		ProcessOutputLoop();
	}

	return hr;
}


HRESULT CEVRPresenter::BeginStreaming()
{
	return m_Scheduler.StartScheduler(m_pClock);
}


HRESULT CEVRPresenter::EndStreaming()
{
	return m_Scheduler.StopScheduler();
}


HRESULT CEVRPresenter::CheckEndOfStream()
{
	if (!m_bEndStreaming) {
		return S_OK;
	}

	if (m_bSampleNotify) {
		return S_OK;
	}

	if (m_SamplePool.AreSamplesPending()) {
		return S_OK;
	}

	NotifyEvent(EC_COMPLETE, S_OK, 0);

	m_bEndStreaming = false;

	return S_OK;
}


HRESULT CEVRPresenter::PrepareFrameStep(DWORD cSteps)
{
	m_FrameStep.Steps += cSteps;
	m_FrameStep.State = FrameStep_WaitingStart;

	HRESULT hr = S_OK;

	if (m_RenderState == RenderState_Started) {
		hr = StartFrameStep();
	}

	return hr;
}


HRESULT CEVRPresenter::StartFrameStep()
{
	_ASSERT(m_RenderState == RenderState_Started);

	HRESULT hr = S_OK;

	if (m_FrameStep.State == FrameStep_WaitingStart) {
		m_FrameStep.State = FrameStep_Pending;

		while (!m_FrameStep.Samples.IsEmpty() && (m_FrameStep.State == FrameStep_Pending)) {
			IMFSample *pSample = nullptr;

			hr = m_FrameStep.Samples.RemoveFront(&pSample);

			if (SUCCEEDED(hr)) {
				hr = DeliverFrameStepSample(pSample);

				pSample->Release();
			}

			if (FAILED(hr)) {
				break;
			}
		}
	} else if (m_FrameStep.State == FrameStep_None) {
		while (!m_FrameStep.Samples.IsEmpty()) {
			IMFSample *pSample = nullptr;

			hr = m_FrameStep.Samples.RemoveFront(&pSample);

			if (SUCCEEDED(hr)) {
				hr = DeliverSample(pSample, FALSE);

				pSample->Release();
			}

			if (FAILED(hr)) {
				break;
			}
		}
	}

	return hr;
}


HRESULT CEVRPresenter::CompleteFrameStep(IMFSample *pSample)
{
	HRESULT hr = S_OK;

	m_FrameStep.State = FrameStep_Complete;
	m_FrameStep.pSampleNoRef = nullptr;

	NotifyEvent(EC_STEP_COMPLETE, FALSE, 0);

	if (IsScrubbing()) {
		MFTIME hnsSampleTime = 0;
		MFTIME hnsSystemTime = 0;

		hr = pSample->GetSampleTime(&hnsSampleTime);
		if (FAILED(hr)) {
			if (m_pClock != nullptr) {
				m_pClock->GetCorrelatedTime(0, &hnsSampleTime, &hnsSystemTime);
			}
			hr = S_OK;
		}

		NotifyEvent(EC_SCRUB_TIME, LODWORD(hnsSampleTime), HIDWORD(hnsSampleTime));
	}

	return hr;
}


HRESULT CEVRPresenter::CancelFrameStep()
{
	FrameStepState OldState = m_FrameStep.State;

	m_FrameStep.State = FrameStep_None;
	m_FrameStep.Steps = 0;
	m_FrameStep.pSampleNoRef = nullptr;

	if ((OldState > FrameStep_None) && (OldState < FrameStep_Complete)) {
		NotifyEvent(EC_STEP_COMPLETE, TRUE, 0);
	}

	return S_OK;
}


HRESULT CEVRPresenter::CreateOptimalVideoType(IMFMediaType *pProposedType, IMFMediaType **ppOptimalType)
{
	if (ppOptimalType == nullptr) {
		return E_POINTER;
	}

	*ppOptimalType = nullptr;

	HRESULT hr;

	VideoType mtOptimal;

	hr = mtOptimal.CopyFrom(pProposedType);
	if (FAILED(hr)) {
		return hr;
	}

	RECT rcOutput = m_pPresentEngine->GetDestinationRect();
	if (::IsRectEmpty(&rcOutput)) {
		hr = CalculateOutputRectangle(pProposedType, &rcOutput);
		if (FAILED(hr)) {
			return hr;
		}
	}

	if (m_AspectRatioMode & MFVideoARMode_PreservePicture) {
		hr = mtOptimal.SetPixelAspectRatio(1, 1);
		if (FAILED(hr)) {
			return hr;
		}
	} else {
		UINT32 SrcWidth = 0, SrcHeight = 0;
		hr = mtOptimal.GetFrameDimensions(&SrcWidth, &SrcHeight);
		if (FAILED(hr)) {
			return hr;
		}
		MFRatio AspectRatio = mtOptimal.GetPixelAspectRatio();
		SrcWidth = ::MulDiv(SrcWidth, AspectRatio.Numerator, AspectRatio.Denominator);

		hr = mtOptimal.SetPixelAspectRatio(
			static_cast<UINT32>(
				static_cast<float>(rcOutput.bottom - rcOutput.top) *
				static_cast<float>(SrcWidth) * (m_nrcSource.right - m_nrcSource.left) + 0.5f),
			static_cast<UINT32>(
				static_cast<float>(rcOutput.right - rcOutput.left) *
				static_cast<float>(SrcHeight) * (m_nrcSource.bottom - m_nrcSource.top) + 0.5f));
		if (FAILED(hr)) {
			return hr;
		}
	}

	hr = mtOptimal.SetFrameDimensions(rcOutput.right, rcOutput.bottom);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea DisplayArea = MakeArea(0, 0, rcOutput.right, rcOutput.bottom);

	hr = mtOptimal.SetPanScanEnabled(FALSE);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetGeometricAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetPanScanAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetMinDisplayAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	mtOptimal.SetYUVMatrix(MFVideoTransferMatrix_BT709);
	mtOptimal.SetTransferFunction(MFVideoTransFunc_709);
	mtOptimal.SetVideoPrimaries(MFVideoPrimaries_BT709);
	//mtOptimal.SetVideoNominalRange(MFNominalRange_16_235);
	mtOptimal.SetVideoNominalRange(MFNominalRange_0_255);
	mtOptimal.SetVideoLighting(MFVideoLighting_dim);

	*ppOptimalType = mtOptimal.Detach();

	return S_OK;
}


HRESULT CEVRPresenter::CalculateOutputRectangle(IMFMediaType *pProposedType, RECT *prcOutput)
{
	HRESULT hr;

	VideoType mtProposed(pProposedType);

	UINT32 SrcWidth = 0, SrcHeight = 0;

	hr = mtProposed.GetFrameDimensions(&SrcWidth, &SrcHeight);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea DisplayArea = {};

	hr = mtProposed.GetVideoDisplayArea(&DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	LONG OffsetX = static_cast<LONG>(MFOffsetToFloat(DisplayArea.OffsetX));
	LONG OffsetY = static_cast<LONG>(MFOffsetToFloat(DisplayArea.OffsetY));
	RECT rcOutput;

	if ((DisplayArea.Area.cx != 0) &&
		(DisplayArea.Area.cy != 0) &&
		(OffsetX + DisplayArea.Area.cx <= static_cast<LONG>(SrcWidth)) &&
		(OffsetY + DisplayArea.Area.cy <= static_cast<LONG>(SrcHeight))) {
		rcOutput.left   = OffsetX;
		rcOutput.right  = OffsetX + DisplayArea.Area.cx;
		rcOutput.top    = OffsetY;
		rcOutput.bottom = OffsetY + DisplayArea.Area.cy;
	} else {
		rcOutput.left   = 0;
		rcOutput.top    = 0;
		rcOutput.right  = SrcWidth;
		rcOutput.bottom = SrcHeight;
	}

	MFRatio InputPAR = mtProposed.GetPixelAspectRatio();
	MFRatio OutputPAR = {1, 1};

	*prcOutput = CorrectAspectRatio(rcOutput, InputPAR, OutputPAR);

	return S_OK;
}


HRESULT CEVRPresenter::SetMediaType(IMFMediaType *pMediaType)
{
	if (pMediaType == nullptr) {
		SafeRelease(m_pMediaType);
		ReleaseResources();
		return S_OK;
	}

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (AreMediaTypesEqual(m_pMediaType, pMediaType)) {
		return S_OK;
	}

	SafeRelease(m_pMediaType);
	ReleaseResources();

	VideoSampleList SampleQueue;
	MFRatio fps = {0, 0};

	hr = m_pPresentEngine->CreateVideoSamples(pMediaType, SampleQueue);
	if (FAILED(hr)) {
		goto Done;
	}

	for (VideoSampleList::Position Pos = SampleQueue.FrontPosition();
			Pos != SampleQueue.EndPosition();
			Pos = SampleQueue.Next(Pos)) {
		IMFSample *pSample = nullptr;

		hr = SampleQueue.GetItemPos(Pos, &pSample);
		if (FAILED(hr)) {
			goto Done;
		}

		hr = pSample->SetUINT32(SampleAttribute_Counter, m_TokenCounter);

		pSample->Release();

		if (FAILED(hr)) {
			goto Done;
		}
	}

	hr = m_SamplePool.Initialize(SampleQueue);
	if (FAILED(hr)) {
		goto Done;
	}

	if (SUCCEEDED(GetFrameRate(pMediaType, &fps)) &&
		(fps.Numerator != 0) && (fps.Denominator != 0)) {
		m_Scheduler.SetFrameRate(fps);
	} else {
		static const MFRatio DefaultFrameRate = {30, 1};
		m_Scheduler.SetFrameRate(DefaultFrameRate);
	}

	m_pMediaType = pMediaType;
	m_pMediaType->AddRef();

Done:
	if (FAILED(hr)) {
		ReleaseResources();
	}

	return hr;
}


HRESULT CEVRPresenter::IsMediaTypeSupported(IMFMediaType *pMediaType)
{
	HRESULT hr;

	VideoType mtProposed(pMediaType);

	BOOL bCompressed = FALSE;

	hr = mtProposed.IsCompressedFormat(&bCompressed);
	if (FAILED(hr)) {
		return hr;
	}

	if (bCompressed) {
		return MF_E_INVALIDMEDIATYPE;
	}

	D3DFORMAT Format = D3DFMT_UNKNOWN;

	hr = mtProposed.GetFourCC((DWORD*)&Format);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_pPresentEngine->CheckFormat(Format);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoInterlaceMode InterlaceMode = MFVideoInterlace_Unknown;

	hr = mtProposed.GetInterlaceMode(&InterlaceMode);
	if (InterlaceMode != MFVideoInterlace_Progressive) {
		return MF_E_INVALIDMEDIATYPE;
	}

	UINT32 Width = 0, Height = 0;

	hr = mtProposed.GetFrameDimensions(&Width, &Height);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea VideoCropArea;

	if (SUCCEEDED(mtProposed.GetPanScanAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}
	if (SUCCEEDED(mtProposed.GetGeometricAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}
	if (SUCCEEDED(mtProposed.GetMinDisplayAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}

	return S_OK;
}


void CEVRPresenter::ProcessOutputLoop()
{
	HRESULT hr;

	do {
		if (!m_bSampleNotify) {
			hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
			break;
		}

		hr = ProcessOutput();

	} while (hr == S_OK);

	if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
		CheckEndOfStream();
	}
}


HRESULT CEVRPresenter::ProcessOutput()
{
	_ASSERT(m_bSampleNotify || m_bRepaint);

	HRESULT hr = S_OK;
	DWORD Status = 0;
	LONGLONG MixerStartTime = 0, MixerEndTime = 0;
	MFTIME SystemTime = 0;
	bool bRepaint = m_bRepaint;

	MFT_OUTPUT_DATA_BUFFER DataBuffer = {};

	IMFSample *pSample = nullptr;

	if ((m_RenderState != RenderState_Started) && !m_bRepaint && m_bPrerolled) {
		return S_FALSE;
	}

	if (m_pMixer == nullptr) {
		return MF_E_INVALIDREQUEST;
	}

	hr = m_SamplePool.GetSample(&pSample);
	if (hr == MF_E_SAMPLEALLOCATOR_EMPTY) {
		return S_FALSE;
	}
	if (FAILED(hr)) {
		return hr;
	}

	if (m_bRepaint) {
		SetDesiredSampleTime(pSample, m_Scheduler.LastSampleTime(), m_Scheduler.FrameDuration());
		m_bRepaint = false;
	} else {
		ClearDesiredSampleTime(pSample);

		if (m_pClock) {
			m_pClock->GetCorrelatedTime(0, &MixerStartTime, &SystemTime);
		}
	}

	DataBuffer.dwStreamID = 0;
	DataBuffer.pSample = pSample;
	DataBuffer.dwStatus = 0;

	hr = m_pMixer->ProcessOutput(0, 1, &DataBuffer, &Status);

	if (FAILED(hr)) {
		HRESULT hr2 = m_SamplePool.ReturnSample(pSample);
		if (FAILED(hr2)) {
			hr = hr2;
		} else {
			if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
				hr = RenegotiateMediaType();
			} else if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
				SetMediaType(nullptr);
			} else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
				m_bSampleNotify = false;
			}
		}
	} else {
		if (m_pClock && !bRepaint) {
			m_pClock->GetCorrelatedTime(0, &MixerEndTime, &SystemTime);

			LONGLONG LatencyTime = MixerEndTime - MixerStartTime;
			NotifyEvent(EC_PROCESSING_LATENCY, (LONG_PTR)&LatencyTime, 0);
		}

		hr = TrackSample(pSample);
		if (SUCCEEDED(hr)) {
			if ((m_FrameStep.State == FrameStep_None) || bRepaint) {
				hr = DeliverSample(pSample, bRepaint);
			} else {
				hr = DeliverFrameStepSample(pSample);
			}
			if (SUCCEEDED(hr)) {
				m_bPrerolled = true;
			}
		}
	}

	SafeRelease(DataBuffer.pEvents);
	SafeRelease(pSample);

	return hr;
}


HRESULT CEVRPresenter::DeliverSample(IMFSample *pSample, BOOL bRepaint)
{
	_ASSERT(pSample != nullptr);

	HRESULT hr;
	CEVRPresentEngine::DeviceState State = CEVRPresentEngine::DeviceState_OK;

	bool bPresentNow = ((m_RenderState != RenderState_Started) || IsScrubbing() || bRepaint);

	hr = m_pPresentEngine->CheckDeviceState(&State);

	if (SUCCEEDED(hr)) {
		hr = m_Scheduler.ScheduleSample(pSample, bPresentNow);
	}

	if (FAILED(hr)) {
		NotifyEvent(EC_ERRORABORT, hr, 0);
	} else if (State == CEVRPresentEngine::DeviceState_Reset) {
		NotifyEvent(EC_DISPLAY_CHANGED, S_OK, 0);
	}

	return hr;
}


HRESULT CEVRPresenter::DeliverFrameStepSample(IMFSample *pSample)
{
	HRESULT hr = S_OK;

	if (IsScrubbing() && m_pClock && IsSampleTimePassed(m_pClock, pSample)) {
		// サンプル破棄
	} else if (m_FrameStep.State >= FrameStep_Scheduled) {
		hr = m_FrameStep.Samples.InsertBack(pSample);
	} else {
		if (m_FrameStep.Steps > 0) {
			m_FrameStep.Steps--;
		}

		if (m_FrameStep.Steps > 0) {
			// サンプル破棄
		} else if (m_FrameStep.State == FrameStep_WaitingStart) {
			hr = m_FrameStep.Samples.InsertBack(pSample);
		} else {
			hr = DeliverSample(pSample, FALSE);
			if (SUCCEEDED(hr)) {
				IUnknown *pUnk = nullptr;

				hr = pSample->QueryInterface(IID_PPV_ARGS(&pUnk));
				if (SUCCEEDED(hr)) {
					m_FrameStep.pSampleNoRef = pUnk;
					m_FrameStep.State = FrameStep_Scheduled;
					pUnk->Release();
				}
			}
		}
	}

	return hr;
}


HRESULT CEVRPresenter::TrackSample(IMFSample *pSample)
{
	HRESULT hr;
	IMFTrackedSample *pTracked = nullptr;

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pTracked));
	if (SUCCEEDED(hr)) {
		hr = pTracked->SetAllocator(&m_SampleFreeCB, nullptr);
		pTracked->Release();
	}

	return hr;
}


void CEVRPresenter::ReleaseResources()
{
	m_TokenCounter++;

	Flush();

	m_SamplePool.Clear();

	m_pPresentEngine->ReleaseResources();
}


HRESULT CEVRPresenter::OnSampleFree(IMFAsyncResult *pResult)
{
	HRESULT hr;
	IUnknown *pObject = nullptr;
	IMFSample *pSample = nullptr;

	hr = pResult->GetObject(&pObject);

	if (SUCCEEDED(hr)) {
		hr = pObject->QueryInterface(IID_PPV_ARGS(&pSample));

		if (SUCCEEDED(hr)) {
			if (m_FrameStep.State == FrameStep_Scheduled)  {
				IUnknown *pUnk = nullptr;

				hr = pSample->QueryInterface(IID_PPV_ARGS(&pUnk));

				if (SUCCEEDED(hr)) {
					if (m_FrameStep.pSampleNoRef == pUnk) {
						hr = CompleteFrameStep(pSample);
					}

					pUnk->Release();
				}
			}
		}
	}

	if (SUCCEEDED(hr)) {
		CAutoLock Lock(&m_ObjectLock);

		if (::MFGetAttributeUINT32(pSample, SampleAttribute_Counter, (UINT32)-1) == m_TokenCounter) {
			hr = m_SamplePool.ReturnSample(pSample);

			if (SUCCEEDED(hr)) {
				ProcessOutputLoop();
			}
		}
	}

	if (FAILED(hr)) {
		NotifyEvent(EC_ERRORABORT, hr, 0);
	}

	SafeRelease(pObject);
	SafeRelease(pSample);

	return hr;
}


float CEVRPresenter::GetMaxRate(BOOL bThin)
{
	float fMaxRate = FLT_MAX;

	if (!bThin && (m_pMediaType != nullptr)) {
		MFRatio fps = {0, 0};

		GetFrameRate(m_pMediaType, &fps);

		UINT MonitorRateHz = m_pPresentEngine->RefreshRate();

		if (fps.Denominator && fps.Numerator && MonitorRateHz) {
			fMaxRate = static_cast<float>(::MulDiv(MonitorRateHz, fps.Denominator, fps.Numerator));
		}
	}

	return fMaxRate;
}
