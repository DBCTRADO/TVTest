#pragma once


#include "EVRPresentEngine.h"


class CEVRPresenter
	: private RefCountedObject

	, public IMFVideoDeviceID
	, public IMFVideoPresenter
	, public IMFRateSupport
	, public IMFGetService
	, public IMFTopologyServiceLookupClient
	, public IMFVideoDisplayControl
{
public:
	static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void **ppv) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

	// IMFGetService
	STDMETHOD(GetService)(REFGUID guidService, REFIID riid, LPVOID *ppvObject) override;

	// IMFVideoPresenter
	STDMETHOD(ProcessMessage)(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) override;
	STDMETHOD(GetCurrentMediaType)(IMFVideoMediaType **ppMediaType) override;

	// IMFClockStateSink
	STDMETHOD(OnClockStart)(MFTIME hnsSystemTime, LONGLONG llClockStartOffset) override;
	STDMETHOD(OnClockStop)(MFTIME hnsSystemTime) override;
	STDMETHOD(OnClockPause)(MFTIME hnsSystemTime) override;
	STDMETHOD(OnClockRestart)(MFTIME hnsSystemTime) override;
	STDMETHOD(OnClockSetRate)(MFTIME hnsSystemTime, float flRate) override;

	// IMFRateSupport
	STDMETHOD(GetSlowestRate)(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate) override;
	STDMETHOD(GetFastestRate)(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate) override;
	STDMETHOD(IsRateSupported)(BOOL bThin, float flRate, float *pflNearestSupportedRate) override;

	// IMFVideoDeviceID
	STDMETHOD(GetDeviceID)(IID* pDeviceID) override;

	// IMFTopologyServiceLookupClient
	STDMETHOD(InitServicePointers)(IMFTopologyServiceLookup *pLookup) override;
	STDMETHOD(ReleaseServicePointers)() override;

	// IMFVideoDisplayControl
	STDMETHOD(GetNativeVideoSize)(SIZE *pszVideo, SIZE *pszARVideo) override;
	STDMETHOD(GetIdealVideoSize)(SIZE *pszMin, SIZE *pszMax) override { return E_NOTIMPL; }
	STDMETHOD(SetVideoPosition)(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest) override;
	STDMETHOD(GetVideoPosition)(MFVideoNormalizedRect* pnrcSource, LPRECT prcDest) override;
	STDMETHOD(SetAspectRatioMode)(DWORD dwAspectRatioMode) override;
	STDMETHOD(GetAspectRatioMode)(DWORD *pdwAspectRatioMode) override;
	STDMETHOD(SetVideoWindow)(HWND hwndVideo) override;
	STDMETHOD(GetVideoWindow)(HWND *phwndVideo) override;
	STDMETHOD(RepaintVideo)() override;
	STDMETHOD(GetCurrentImage)(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp) override;
	STDMETHOD(SetBorderColor)(COLORREF Clr) override { return E_NOTIMPL; }
	STDMETHOD(GetBorderColor)(COLORREF *pClr) override { return E_NOTIMPL; }
	STDMETHOD(SetRenderingPrefs)(DWORD dwRenderFlags) override;
	STDMETHOD(GetRenderingPrefs)(DWORD *pdwRenderFlags) override;
	STDMETHOD(SetFullscreen)(BOOL bFullscreen) override { return E_NOTIMPL; }
	STDMETHOD(GetFullscreen)(BOOL *pbFullscreen) override { return E_NOTIMPL; }

protected:
	enum RenderState
	{
		RenderState_Started,
		RenderState_Stopped,
		RenderState_Paused,
		RenderState_Shutdown
	};

	enum FrameStepState
	{
		FrameStep_None,
		FrameStep_WaitingStart,
		FrameStep_Pending,
		FrameStep_Scheduled,
		FrameStep_Complete
	};

	CEVRPresenter(HRESULT &hr);
	virtual ~CEVRPresenter();

	inline HRESULT CheckShutdown() const
	{
		if (m_RenderState == RenderState_Shutdown) {
			return MF_E_SHUTDOWN;
		}
		return S_OK;
	}

	inline bool IsActive() const
	{
		return (m_RenderState == RenderState_Started) || (m_RenderState == RenderState_Paused);
	}

	inline bool IsScrubbing() const { return m_fRate == 0.0f; }

	void NotifyEvent(long EventCode, LONG_PTR Param1, LONG_PTR Param2)
	{
		if (m_pMediaEventSink) {
			m_pMediaEventSink->Notify(EventCode, Param1, Param2);
		}
	}

	float GetMaxRate(BOOL bThin);

	HRESULT ConfigureMixer(IMFTransform *pMixer);

	HRESULT CreateOptimalVideoType(IMFMediaType* pProposed, IMFMediaType **ppOptimal);
	HRESULT CalculateOutputRectangle(IMFMediaType *pProposed, RECT *prcOutput);
	HRESULT SetMediaType(IMFMediaType *pMediaType);
	HRESULT IsMediaTypeSupported(IMFMediaType *pMediaType);

	HRESULT Flush();
	HRESULT RenegotiateMediaType();
	HRESULT ProcessInputNotify();
	HRESULT BeginStreaming();
	HRESULT EndStreaming();
	HRESULT CheckEndOfStream();

	void ProcessOutputLoop();
	HRESULT ProcessOutput();
	HRESULT DeliverSample(IMFSample *pSample, BOOL bRepaint);
	HRESULT TrackSample(IMFSample *pSample);
	void ReleaseResources();

	HRESULT PrepareFrameStep(DWORD cSteps);
	HRESULT StartFrameStep();
	HRESULT DeliverFrameStepSample(IMFSample *pSample);
	HRESULT CompleteFrameStep(IMFSample *pSample);
	HRESULT CancelFrameStep();

	HRESULT OnSampleFree(IMFAsyncResult *pResult);

	AsyncCallback<CEVRPresenter> m_SampleFreeCB;

	struct FrameStep
	{
		FrameStep()
			: State(FrameStep_None)
			, Steps(0)
			, pSampleNoRef(nullptr)
		{
		}

		FrameStepState State;
		VideoSampleList Samples;
		DWORD Steps;
		IUnknown *pSampleNoRef;
	};

	RenderState m_RenderState;
	FrameStep m_FrameStep;

	CCritSec m_ObjectLock;

	CEVRScheduler m_Scheduler;
	CSamplePool m_SamplePool;
	DWORD m_TokenCounter;

	bool m_bSampleNotify;
	bool m_bRepaint;
	bool m_bPrerolled;
	bool m_bEndStreaming;

	SIZE m_NativeVideoSize;
	MFRatio m_NativeAspectRatio;

	MFVideoNormalizedRect m_nrcSource;
	float m_fRate;

	DWORD m_AspectRatioMode;
	DWORD m_RenderPrefs;

	CEVRPresentEngine *m_pPresentEngine;

	IMFClock *m_pClock;
	IMFTransform *m_pMixer;
	IMediaEventSink *m_pMediaEventSink;
	IMFMediaType *m_pMediaType;
};
