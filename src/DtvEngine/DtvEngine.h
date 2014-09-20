// DtvEngine.h: CDtvEngine クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "BonBaseClass.h"
#include "BonSrcDecoder.h"
#include "TsPacketParser.h"
#include "TsAnalyzer.h"
#include "CasProcessor.h"
#include "MediaViewer.h"
#include "MediaTee.h"
//#include "FileWriter.h"
#include "BufferedFileWriter.h"
#include "MediaGrabber.h"
#include "TsSelector.h"
#include "EventManager.h"
#include "CaptionDecoder.h"
#include "LogoDownloader.h"

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

	enum DecoderID {
		DECODER_ID_BonSrcDecoder=1,
		DECODER_ID_TsPacketParser,
		DECODER_ID_TsAnalyzer,
		DECODER_ID_CasProcessor,
		DECODER_ID_MediaViewer,
		DECODER_ID_MediaTee,
		DECODER_ID_FileWriter,
		DECODER_ID_MediaGrabber,
		DECODER_ID_TsSelector,
		DECODER_ID_EventManager,
		DECODER_ID_CaptionDecoder,
		DECODER_ID_LogoDownloader
	};

	struct DecoderConnectionInfo
	{
		DecoderID OutputDecoder;
		DecoderID InputDecoder;
		int OutputIndex;

		DecoderConnectionInfo(DecoderID Out, DecoderID In, int OutIndex = 0)
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
		virtual void OnFileWriteError(CBufferedFileWriter *pFileWriter) {}
		virtual void OnVideoStreamTypeChanged(BYTE StreamType) {}
		virtual void OnVideoSizeChanged(CMediaViewer *pMediaViewer) {}
		virtual void OnEmmProcessed() {}
		virtual void OnEmmError(LPCTSTR pszText) {}
		virtual void OnEcmError(LPCTSTR pszText) {}
		virtual void OnEcmRefused() {}
		virtual void OnCardReaderHung() {}
		virtual void OnEventChanged(CTsAnalyzer *pTsAnalyzer, WORD EventID) {}
		virtual void OnEventUpdated(CTsAnalyzer *pTsAnalyzer) {}
		virtual void OnTotUpdated(CTsAnalyzer *pTsAnalyzer) {}
	};

	struct ServiceSelectInfo
	{
		WORD ServiceID;
		bool bFollowViewableService;
		bool bPrefer1Seg;
		WORD PreferredServiceIndex;

		ServiceSelectInfo()
			: ServiceID(SID_DEFAULT)
			, bFollowViewableService(false)
			, bPrefer1Seg(false)
			, PreferredServiceIndex(SERVICE_INVALID)
		{
		}

		void Reset() { *this = ServiceSelectInfo(); }
	};

	CDtvEngine(void);
	~CDtvEngine(void);

	bool BuildEngine(const DecoderConnectionInfo *pDecoderConnectionList,
					 int DecoderConnectionCount,
					 CEventHandler *pEventHandler,
					 bool bDescramble = true);
	bool IsEngineBuild() const { return m_bBuiled; };
	bool IsBuildComplete() const;
	bool CloseEngine(void);
	bool ResetEngine(void);

	bool OpenBonDriver(LPCTSTR pszFileName);
	bool ReleaseSrcFilter();
	bool IsSrcFilterOpen() const;

	bool SetChannel(const BYTE byTuningSpace, const WORD wChannel,
					const ServiceSelectInfo *pServiceSelInfo = NULL);
	bool SetService(const ServiceSelectInfo *pServiceSelInfo, const bool bReserve = true);
	bool SetServiceByID(const WORD ServiceID, const bool bReserve = true);
	bool SetServiceByIndex(const WORD Service);
	bool GetServiceID(WORD *pServiceID) const;
	WORD GetServiceIndex() const;

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
	BYTE GetAudioComponentTag() const;
	BYTE GetAudioComponentType(const int AudioIndex = -1) const;
	int GetAudioComponentText(LPTSTR pszText, int MaxLength, const int AudioIndex = -1) const;

	bool LoadCasLibrary(LPCTSTR pszFileName);
	bool OpenCasCard(int Device, LPCTSTR pszReaderName = NULL);
	bool CloseCasCard();
	bool SetDescramble(bool bDescramble);
	bool SetDescrambleService(WORD ServiceID);
	bool SetDescrambleCurServiceOnly(bool bOnly);
	bool GetDescrambleCurServiceOnly() const { return m_bDescrambleCurServiceOnly; }
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
	CCasProcessor m_CasProcessor;
	CMediaViewer m_MediaViewer;				// メディアビューアー
	CMediaTee m_MediaTee;					// メディアティー
	//CFileWriter m_FileWriter;				// ファイルライター
	CBufferedFileWriter m_FileWriter;
	CMediaGrabber m_MediaGrabber;
	CTsSelector m_TsSelector;
	CEventManager m_EventManager;
	CCaptionDecoder m_CaptionDecoder;
	CLogoDownloader m_LogoDownloader;

protected:
	virtual const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam);

	bool SelectService(WORD ServiceID);
	CMediaDecoder *GetDecoderByID(DecoderID ID);
	const DecoderConnectionInfo *GetInputConnectionInfo(DecoderID ID) const;
	const DecoderConnectionInfo *GetOutputConnectionInfo(DecoderID ID) const;
	void ConnectDecoder(DecoderID ID);
	void DisconnectDecoder(DecoderID ID);

	mutable CCriticalLock m_EngineLock;
	CEventHandler *m_pEventHandler;
	std::vector<DecoderConnectionInfo> m_DecoderConnectionList;

	WORD m_wCurTransportStream;
	WORD m_CurServiceIndex;
	WORD m_CurServiceID;
	ServiceSelectInfo m_ServiceSel;
	BYTE m_VideoStreamType;
	int m_CurVideoStream;
	BYTE m_CurVideoComponentTag;
	int m_CurAudioStream;
	BYTE m_CurAudioComponentTag;
	WORD m_CurEventID;

	bool m_bBuiled;
	bool m_bDescramble;
	bool m_bStartStreamingOnDriverOpen;

	bool m_bDescrambleCurServiceOnly;
	bool m_bWriteCurServiceOnly;
	DWORD m_WriteStream;

	void ResetStatus();
};
