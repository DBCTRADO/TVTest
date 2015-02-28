// DtvEngine.h: CDtvEngine クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "BonBaseClass.h"
#include "BonSrcDecoder.h"
#include "TsPacketParser.h"
#include "TsAnalyzer.h"
#include "MediaViewer.h"
#include "MediaTee.h"
#include "TsRecorder.h"
#include "MediaGrabber.h"
#include "TsSelector.h"
#include "EventManager.h"
#include "CaptionDecoder.h"
#include "LogoDownloader.h"
#include "TsPacketCounter.h"

// ※この辺は全くの暫定です


//////////////////////////////////////////////////////////////////////
// デジタルTVエンジンクラス
//////////////////////////////////////////////////////////////////////

class CDtvEngine : protected CMediaDecoder::IEventHandler, public CBonBaseClass
{
public:
	enum {
		SID_INVALID     = 0xFFFF,
		SID_DEFAULT     = 0xFFFF,
		SERVICE_INVALID = 0xFFFF,
		SERVICE_DEFAULT = 0xFFFF
	};

	enum {
		DECODER_ID_BonSrcDecoder=1,
		DECODER_ID_TsPacketParser,
		DECODER_ID_TsAnalyzer,
		DECODER_ID_MediaViewer,
		DECODER_ID_MediaTee,
		DECODER_ID_TsRecorder,
		DECODER_ID_MediaGrabber,
		DECODER_ID_TsSelector,
		DECODER_ID_EventManager,
		DECODER_ID_CaptionDecoder,
		DECODER_ID_LogoDownloader,
		DECODER_ID_TsPacketCounter,
		DECODER_ID_External
	};

	struct DecoderConnectionInfo
	{
		int OutputDecoder;
		int InputDecoder;
		int OutputIndex;

		DecoderConnectionInfo(int Out, int In, int OutIndex = 0)
			: OutputDecoder(Out)
			, InputDecoder(In)
			, OutputIndex(OutIndex)
		{
		}
	};

	class ABSTRACT_CLASS_DECL CEventHandler
	{
		friend CDtvEngine;
	public:
		virtual ~CEventHandler() = 0 {}
	protected:
		CDtvEngine *m_pDtvEngine;
		virtual void OnServiceListUpdated(CTsAnalyzer *pTsAnalyzer, bool bStreamChanged) {}
		virtual void OnServiceInfoUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnServiceChanged(WORD ServiceID) {}
		virtual void OnFileWriteError(CTsRecorder *pTsRecorder) {}
		virtual void OnVideoStreamTypeChanged(BYTE StreamType) {}
		virtual void OnVideoSizeChanged(CMediaViewer *pMediaViewer) {}
		virtual void OnEventChanged(CTsAnalyzer *pTsAnalyzer, WORD EventID) {}
		virtual void OnEventUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnTotUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnFilterGraphInitialize(CMediaViewer *pMediaViewer, IGraphBuilder *pGraphBuilder) {}
		virtual void OnFilterGraphInitialized(CMediaViewer *pMediaViewer, IGraphBuilder *pGraphBuilder) {}
		virtual void OnFilterGraphFinalize(CMediaViewer *pMediaViewer, IGraphBuilder *pGraphBuilder) {}
		virtual void OnFilterGraphFinalized(CMediaViewer *pMediaViewer, IGraphBuilder *pGraphBuilder) {}
		virtual void OnSpdifPassthroughError(HRESULT hr) {}
	};

	enum OneSegSelectType {
		ONESEG_SELECT_LOWPRIORITY,
		ONESEG_SELECT_HIGHPRIORITY,
		ONESEG_SELECT_REFUSE
	};

	struct ServiceSelectInfo
	{
		WORD ServiceID;
		bool bFollowViewableService;
		OneSegSelectType OneSegSelect;
		WORD PreferredServiceIndex;

		ServiceSelectInfo()
			: ServiceID(SID_DEFAULT)
			, bFollowViewableService(false)
			, OneSegSelect(ONESEG_SELECT_LOWPRIORITY)
			, PreferredServiceIndex(SERVICE_INVALID)
		{
		}

		void Reset() { *this = ServiceSelectInfo(); }
	};

	CDtvEngine(void);
	~CDtvEngine(void);

	bool BuildEngine(const DecoderConnectionInfo *pDecoderConnectionList,
					 int DecoderConnectionCount,
					 CEventHandler *pEventHandler);
	bool IsEngineBuild() const { return m_bBuiled; };
	bool CloseEngine(void);
	bool ResetEngine(void);
	int RegisterDecoder(CMediaDecoder *pDecoder);

	bool OpenBonDriver(LPCTSTR pszFileName);
	bool ReleaseSrcFilter();
	bool IsSrcFilterOpen() const;

	bool SetChannel(const BYTE byTuningSpace, const WORD wChannel,
					const ServiceSelectInfo *pServiceSelInfo = NULL);
	bool SetService(const ServiceSelectInfo *pServiceSelInfo, const bool bReserve = true);
	bool GetServiceID(WORD *pServiceID) const;
	WORD GetServiceIndex() const;
	bool SetOneSegSelectType(OneSegSelectType Type);

	WORD GetEventID(bool bNext = false);
	int GetEventName(LPTSTR pszName, int MaxLength, bool bNext = false);
	int GetEventText(LPTSTR pszText, int MaxLength, bool bNext = false);
	int GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext = false);
	bool GetEventTime(SYSTEMTIME *pStartTime, DWORD *pDuration, bool bNext = false);
	bool GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext = false);
	bool GetEventInfo(CEventInfo *pInfo, bool bNext = false);
	bool GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex = -1, bool bNext = false);
	int GetCurComponentGroup() const;

	unsigned __int64 GetPcrTimeStamp();

	bool BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer = CVideoRenderer::RENDERER_DEFAULT,
		BYTE VideoStreamType = STREAM_TYPE_INVALID,
		LPCWSTR pszVideoDecoder = NULL,
		LPCWSTR pszAudioDevice = NULL);
	bool RebuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer = CVideoRenderer::RENDERER_DEFAULT,
		BYTE VideoStreamType = STREAM_TYPE_INVALID,
		LPCWSTR pszVideoDecoder = NULL,
		LPCWSTR pszAudioDevice = NULL);
	bool CloseMediaViewer();
	bool ResetMediaViewer();
	bool EnableMediaViewer(const bool bEnable = true);

	BYTE GetVideoStreamType() const;
	int GetVideoStreamNum(const int Service = -1) const;
	bool SetVideoStream(const int StreamIndex);
	int GetVideoStream() const;
	BYTE GetVideoComponentTag() const;

	BYTE GetAudioChannelNum();
	int GetAudioStreamNum(const int Service = -1) const;
	bool SetAudioStream(const int StreamIndex);
	int GetAudioStream() const;
	BYTE GetAudioComponentTag(const int AudioIndex = -1) const;
	BYTE GetAudioComponentType(const int AudioIndex = -1) const;
	int GetAudioComponentText(LPTSTR pszText, int MaxLength, const int AudioIndex = -1) const;

	bool SetWriteStream(WORD ServiceID, DWORD Stream=CTsSelector::STREAM_ALL);
	bool GetWriteStream(WORD *pServiceID, DWORD *pStream = NULL);
	bool SetWriteCurServiceOnly(bool bOnly, DWORD Stream=CTsSelector::STREAM_ALL);
	bool GetWriteCurServiceOnly() const { return m_bWriteCurServiceOnly; }
	void SetStartStreamingOnDriverOpen(bool bStart);

// CBonBaseClass
	void SetTracer(CTracer *pTracer);

//protected:
	// CMediaDecoder から派生したメディアデコーダクラス
	CBonSrcDecoder m_BonSrcDecoder;			// TSソースチューナー(HAL化すべき)
	CTsPacketParser m_TsPacketParser;		// TSパケッタイザー
	CTsAnalyzer m_TsAnalyzer;
	CMediaViewer m_MediaViewer;				// メディアビューアー
	CMediaTee m_MediaTee;					// メディアティー
	CTsRecorder m_TsRecorder;
	CMediaGrabber m_MediaGrabber;
	CTsSelector m_TsSelector;
	CEventManager m_EventManager;
	CCaptionDecoder m_CaptionDecoder;
	CLogoDownloader m_LogoDownloader;
	CTsPacketCounter m_TsPacketCounter;

protected:
	struct DecoderInfo
	{
		int ID;
		CMediaDecoder *pDecoder;
	};

	virtual const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam);

	bool SelectService(WORD ServiceID, bool bNo1Seg = false);
	CMediaDecoder *GetDecoderByID(int ID);
	const DecoderConnectionInfo *GetInputConnectionInfo(int ID) const;
	const DecoderConnectionInfo *GetOutputConnectionInfo(int ID) const;
	void ConnectDecoder(int ID);
	void DisconnectDecoder(int ID);

	mutable CCriticalLock m_EngineLock;
	CEventHandler *m_pEventHandler;
	std::vector<DecoderInfo> m_DecoderList;
	std::vector<DecoderConnectionInfo> m_DecoderConnectionList;

	WORD m_wCurTransportStream;
	WORD m_CurServiceIndex;
	WORD m_CurServiceID;
	ServiceSelectInfo m_ServiceSel;
	ServiceSelectInfo m_SetChannelServiceSel;
	BYTE m_VideoStreamType;
	int m_CurVideoStream;
	BYTE m_CurVideoComponentTag;
	int m_CurAudioStream;
	BYTE m_CurAudioComponentTag;
	WORD m_CurEventID;

	bool m_bBuiled;
	bool m_bStartStreamingOnDriverOpen;

	bool m_bWriteCurServiceOnly;
	DWORD m_WriteStream;

	void ResetStatus();
};
